#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
extern int errno;
#include "sigsched.h"
#include "ralloc.h"
#include "config/ttyopts.h"
#include "config/ptylongname.h"
#include "sesslog.h"
#include "sessconnlog.h"
#include "ptyget.h"
#include "ptymisc.h"
#include "ptyerr.h"
#include "ptytty.h"
#include "ptycomm.h"
#include "ptymaster.h"

/*
If you don't believe that sigsched makes this incredibly easy to write,
try writing it without sigsched. Then try understanding what you wrote.

Apologies in advance to Billy Joel.
*/

#define PTYBUFSIZE 16384

char inbuf[PTYBUFSIZE];
static int inbufwrite = 0;
static int inbufread = 0;
char outbuf[PTYBUFSIZE];
static int outbufwrite = 0;
static int outbufread = 0;

static char longname[PTYLONGNAMELEN] = { 0 };

static struct sessconnlog scl;
static int remotelen;
static char remote[SESSCONNLOG_REMOTELEN];
static char *musername;

static int mflagreading;
static int mflagsession;
static int mflagxflowctl;
static int mfdcomm;
static int mfdmty;
static int mfdsty;
static int muid = -1;
static char mext[2];
static int mslavepid = -1;

static int fdctrlr = -1;
static int fdsigler;
/* to insure yourself you got to provide communication constantly... */
static int fdsig2us;
static int fdi;
static int fdo;
static int flagwatchchld;
static int flagstopped;

static char recoext[2];
/* this is a violation of modularity, but even after the sigler */
/* knows what recoext is, we may have to report it to ctrlr, so we can't */
/* just forget about it XXX: or should we ask sigler dynamically? no! */

static int mfdpreco;
static int flagpreco;

/* This is a kludge to deal with fd passing bugs (particularly under SunOS). */
/* It's also a sanity check. */
void closeallbut()
{
 int i;

 i = getdtablesize();
 while (i--)
  {
   if (
   (i != mfdcomm)
&& (i != mfdmty)
&& (i != mfdsty)
&& (i != fdctrlr)
&& (i != fdsigler)
&& (i != fdsig2us)
&& (i != fdi)
&& (i != fdo)
&& (i != mfdpreco)
      )
    {
     close(i);
    }
  }
}

#define verbose 0,

static void saytasig(c0,c1,c2,c3) int c0; int c1; int c2; int c3;
{
 char c[4]; c[0] = c0; c[1] = c1; c[2] = c2; c[3] = c3;
 verbose("master telling sigler %c %d %d %d",c0,c1,c2,c3);
 if (fdsigler == -1)
   return;
 /* tell her all your crazy dreams... */
 bwrite(fdsigler,c,4); /* XXX: error checks? */
}

static void childdead()
{
 verbose("master entering childdead");
 /* We could check specially for final output here, but since */
 /* there's no way we can know when it's all done, we don't bother. */
 if (fdsigler != -1)
   ; /* no need for further notification or explicit EOF */
 if (mslavepid != -1)
  {
   struct sesslog sl;
   long t;
   t = now();
   sessconnlog_fill(&scl,mext,"",0,t);
   sesslog_fill(&sl,mext,musername,muid,0,t);
   if (fdsigler != -1)
     sessconnlog(&scl); /* XXX: failure? */
   sesslog(&sl); /* XXX: failure? */
   utmp_off(mext,"",t); /* XXX: failure? */
   wtmp_off(mext,"",t); /* XXX: failure? */
   ungetpty(mfdmty,mfdsty,mext); /* XXX: failure? */
   mslavepid = -1; /* JIC */
   if (mfdcomm != -1)
    {
     if (comm_unlink(mext,muid) == -1)
       ; /* if it fails, who cares? */
     close(mfdcomm); /* goodbye, cruel world */
     mfdcomm = -1;
    }
   ss_unforcewait();
  }
}

static void contchild()
{
 if (mslavepid != -1)
  {
   if (kill(mslavepid,SIGCONT) == -1) /* XXX: when would this work? */
    {
     int pgrp;
     pgrp = getpgrp(mslavepid);
     if (pgrp > 0)
       killpg(pgrp,SIGCONT);
       /* XXX: errors? */
    }
  }
 flagstopped = 0;
 disp_flagstopped();
}

static void disconnect()
{
 verbose("master disconnecting");
 /* oh listen boy */
 /* i'm sure you think you got it all under control... */
 saytasig('d',0,0,0);

 if (fdsigler != -1)
  {
   long t;
   t = now();
   sessconnlog_fill(&scl,mext,"",1/*XXX*/,t);
   sessconnlog(&scl); /* XXX: failure? */
  }

 if (fdsigler != -1) { close(fdsigler); fdsigler = -1; }
 if (fdsig2us != -1) { close(fdsig2us); fdsig2us = -1; }
 if (fdi != -1) { close(fdi); fdi = -1; }
 if (fdo != -1) { close(fdo); fdo = -1; }
 flagwatchchld = 0;
 recoext[0] = recoext[1] = 0;
 disp_fdsigler();
 disp_fdsig2us();
 disp_flagwatchchld();
 if (!mflagsession)
   childdead();
   /* ... and though you may not have done anything */
   /* will that be consolation when she's gone? ... */
 closeallbut();
}

static int reconnect()
{
 verbose("master starting reconnect");

 closeallbut();
 /* assumes fdsigler, fdsig2us, fdi, fdo all -1 */
 if ((fdsigler = comm_getfd(fdctrlr)) == -1)
  {
   verbose("master getting sigler failed");
   return -1; }
 if ((fdsig2us = comm_getfd(fdctrlr)) == -1)
  {
   verbose("master getting fdsig2us failed");
   close(fdsigler); fdsigler = -1; return -1; }
 if ((fdi = comm_getfd(fdctrlr)) == -1)
  {
   verbose("master getting fdi failed");
   close(fdsigler); close(fdsig2us); fdsigler = fdsig2us = -1; return -1; }
 if ((fdo = comm_getfd(fdctrlr)) == -1)
  {
   verbose("master getting fdo failed");
   close(fdsigler); close(fdsig2us); close(fdi);
   fdsigler = fdsig2us = fdi = -1; return -1; }

 verbose("master okay fds on reconnect %d %d %d %d",fdsigler,fdsig2us,fdi,fdo);
 if (bread(fdctrlr,(char *) &mflagreading,sizeof(int)) < sizeof(int))
  { close(fdsigler); close(fdsig2us); close(fdi); close(fdo);
   fdsigler = fdsig2us = fdi = fdo = -1; return -1; }
 if (bread(fdctrlr,(char *) &remotelen,sizeof(int)) < sizeof(int)
    || (remotelen > SESSCONNLOG_REMOTELEN))
  { close(fdsigler); close(fdsig2us); close(fdi); close(fdo);
   fdsigler = fdsig2us = fdi = fdo = -1; return -1; }
 if (bread(fdctrlr,remote,remotelen) < remotelen)
  { close(fdsigler); close(fdsig2us); close(fdi); close(fdo);
   fdsigler = fdsig2us = fdi = fdo = -1; return -1; }

 /* log stuff... */
 sessconnlog_fill(&scl,mext,remote,-1/*XXX*/,now());
 if (sessconnlog(&scl) == -1)
   ; /*XXX: sessconn write failed; do we care?*/

 closeallbut();

 verbose("master succeeded on reconnect %d",mflagreading);
 contchild(); /* XXX */
 flagwatchchld = 1;
 disp_fdsigler();
 disp_fdsig2us();
 disp_flagwatchchld();
 disp_mflagreading();
 if (flagpreco) /* talk to me if you don't understand this */
  {
   flagpreco = 0;
   /* cause now and then she'll get to worrying */
   /* just because you haven't spoken for so long... */
   if (write(mfdpreco,"k",1) == -1)
     ; /* pipe might be broken if child has somehow been killed; */
       /* that's okay, we'll get SIGCHLD in a few moments */
   close(mfdpreco);
   mfdpreco = -1;
  }
 return 0;
}

static void sigchld(n)
int n;
{
 int w;
 int pid;

 verbose("master entering sigchld");
 while ((pid = wait3(&w,WNOHANG | WUNTRACED,(struct rusage *) 0)) > 0)
   /* it'd be very weird if wait3 equalled 0 */ 
   if (pid == mslavepid)
    {
     verbose("master sees pid %d",pid);
#define PWIFSTOPPED(s) (((s) & 0177) == 0177)
#define PWSTOPSIG(s) ((s) >> 8) /* only defined if PWIFSTOPPED */
     if (PWIFSTOPPED(w))
      {
       flagstopped = 1;
       /* just a word or two that she gets from you */
       /* could be the difference that it makes... */
       saytasig('z',PWSTOPSIG(w),0,0);
       disp_flagstopped();
      }
     else
      {
#define PWIFEXITED(s) (!((s) & 0177))
#define PWEXITSTATUS(s) ((s) >> 8)
#define PWTERMSIG(s) ((s) & 0177)
#define PWCOREDUMP(s) ((s) & 0200)
       if (PWIFEXITED(w))
	 /* tell her about it... */
         saytasig('e',PWEXITSTATUS(w),0,0);
       else
	 /* tell her everything you feel... */
	 saytasig('s',PWTERMSIG(w),PWCOREDUMP(w),0);
       childdead();
      }
    }
}
static int schedsigchld = 0;
static void cksigchld()
{
 if (!schedsigchld && flagwatchchld && !outbufread)
  { ss_sched(ss_signal(SIGCHLD),sigchld,0); schedsigchld = 1; return; }
 if (schedsigchld && (!flagwatchchld || outbufread))
  { ss_unsched(ss_signal(SIGCHLD),sigchld,0); schedsigchld = 0; }
}

static void doacceptfdctrlr(n)
int n;
{
 fdctrlr = comm_accept(mfdcomm);
 disp_fdctrlr();
}
static int schedacceptfdctrlr = 0;
static void ckacceptfdctrlr()
{
 if (!schedacceptfdctrlr && (fdctrlr == -1))
  {
   ss_sched(ss_sigread(mfdcomm),doacceptfdctrlr,0); schedacceptfdctrlr = 1;
   return;
  }
 if (schedacceptfdctrlr && (fdctrlr != -1))
  { ss_unsched(ss_sigread(mfdcomm),doacceptfdctrlr,0); schedacceptfdctrlr = 0; }
}

static void doreadfdctrlr(n)
int n;
{
 int r;
 char mess[1];
 int foo;

 verbose("master entering readfdctrlr");
 r = read(fdctrlr,mess,1);
 verbose("master readfdctrlr %d %c",r,mess[0]);
 if (r <= 0) /* bye bye */
  {
   close(fdctrlr); fdctrlr = -1; disp_fdctrlr(); return;
  }
 switch(mess[0]) /* XXX: we tacitly assume these writes will be atomic */
  {
   case 'a': /* are you there? */
     switch(mslavepid % 3)
      {
       case 0: bwrite(fdctrlr,"(nope)",6); break;
       case 1: bwrite(fdctrlr,"a_y_t?",6); break;
       default: bwrite(fdctrlr,"maybe.",6); break;
      }
     break;
   case 'e': /* short name? */
     bwrite(fdctrlr,mext,2);
     break;
   case 'l': /* long name? */
     bwrite(fdctrlr,"longnm",6);
     bwrite(fdctrlr,longname,sizeof(longname));
     break;
   case 'L': /* set long name to this: */
     bread(fdctrlr,longname,sizeof(longname)); /* XXX: error checking? */
     bwrite(fdctrlr,"thanks",6);
     break;
   case 'C': /* are you connected? */
     if (fdsigler == -1)
       bwrite(fdctrlr,"noaose",6);
     else
       bwrite(fdctrlr,"owuno?",6);
     break;
   case 'p': /* what's your pid? */
     foo = getpid();
     bwrite(fdctrlr,(char *) &foo,sizeof(foo));
     break;
   case 'P': /* what's your slave pid? */
     bwrite(fdctrlr,(char *) &mslavepid,sizeof(mslavepid));
     break;
   case 'D': /* do you allow disconnects? */
     bwrite(fdctrlr,(char *) &mflagsession,sizeof(mflagsession));
     break;
   case 'k': /* sesskill */
     saytasig('k',0,0,0);
     bwrite(fdctrlr,"<poof>",6);
     childdead();
     break;
   case 'd': /* disconnect */
     if (mflagsession)
       if (fdsigler != -1)
	{
	 disconnect();
	 bwrite(fdctrlr,"yessir",6);
	}
       else
	 bwrite(fdctrlr,"no-op!",6);
     else
       /* you're a big boy now and you'll never let her go */
       /* but that's just the kind of thing she ought to know... */
       bwrite(fdctrlr,"kinky!",6);
     break;
   case 'r': /* reconnect---including initial connection */
     if (fdsigler == -1)
      {
       /* let her know you need her */
       /* let her know how much she means... */
       bwrite(fdctrlr,"shrtng",6);
       if (reconnect() == -1)
	{
	 bwrite(fdctrlr,"damn! ",6); /*XXXXXXX*/
	 closeallbut();
	}
       else
         bwrite(fdctrlr,"phew! ",6);
      }
     else
       bwrite(fdctrlr,"no-go!",6);
     break;
   case 's': /* recoset */
     if (fdsigler != -1)
      {
       /* XXX: should ack first */
       bread(fdctrlr,recoext,2); /* XXX: error checks? */
       /* when she can't be with you tell her you wish you were there... */
       saytasig('r',recoext[0],recoext[1],0);
       bwrite(fdctrlr,"sglrok",6);
      }
     else
       bwrite(fdctrlr,"nosglr",6);
     break;
   case 'S': /* report latest recoset */
     bwrite(fdctrlr,"latest",6);
     bwrite(fdctrlr,recoext,2);
     break;
   case 'Z': /* suspend */
     /* XXXX: not supported yet; fall through */
   /* XXX: resume? */
   /* XXX: ioctl? particular ioctls? */
   default:
     bwrite(fdctrlr,"nosupp",6);
     break;
  }
 verbose("master exiting readfdctrlr");
}
static ss_sig *sigreadfdctrlr;
static void ckreadfdctrlr()
{
 if (!sigreadfdctrlr && (fdctrlr != -1))
  { ss_sched(sigreadfdctrlr = ss_sigread(fdctrlr),doreadfdctrlr,0); return; }
 if (sigreadfdctrlr && (fdctrlr == -1))
  { ss_unsched(sigreadfdctrlr,doreadfdctrlr,0); sigreadfdctrlr = 0; }
}

static void doreadsig2us(n)
int n;
{
 int r;
 char sigsez[1];
#ifdef TTY_WINDOWS
 struct ttywin twi;
 struct ttymodes tmoold;
 struct ttymodes tmonew;
#endif

 verbose("master entering readsig2us");
 /* every day before you leave */
 /* pay her some attention */
 /* give her something to believe... */
 r = read(fdsig2us,sigsez,1);
 verbose("master readsig2us %d %c",r,sigsez[0]);
 if (r == -1)
   disconnect(); /*XXX: should we try to handle errors better? */
 else if (!r)
   disconnect(); /* if it weren't for this, we'd never see sigler die! */
 else
   switch(sigsez[0])
    {
     case 'H': /* hangup */
       disconnect();
       break;
     case 'C': /* continue */
       contchild();
       break;
     case 'W': /* WINCH */
#ifdef TTY_WINDOWS
       bread(fdsig2us,(char *) &twi,sizeof(twi)); /* XXX: error checks? */
       if (tty_getmodes(mfdsty,&tmoold) == 0)
	{
         /* XXX: race! race! race! */
         tty_copymodes(&tmonew,&tmoold);
         tty_win2modes(&twi,&tmonew);
         if (tty_modifymodes(mfdsty,&tmonew,&tmoold) == -1)
	   ; /* who cares? */
	}
#endif
       break;
     default:
       break; /* XXX: nothing we can reasonably do */
    }
 verbose("master exiting readsig2us");
}
static ss_sig *sigreadsig2us;
static void ckreadsig2us()
{
 if (!sigreadsig2us && (fdsig2us != -1))
  { ss_sched(sigreadsig2us = ss_sigread(fdsig2us),doreadsig2us,0); return; }
 if (sigreadsig2us && (fdsig2us == -1))
  { ss_unsched(sigreadsig2us,doreadsig2us,0); sigreadsig2us = 0; }
}

static void doreadin(n)
int n;
{
 int r;

 verbose("master entering readin");
 /* XXX: sanity checks, like !mflagreading? */

 /* XXX: Warning! We're manipulating an outside descriptor! */
 r = read(fdi,inbuf + inbufread,sizeof(inbuf) - inbufread);
 /* but a girl like that won't tell you what you should do... */
 if (r == -1)
   switch(errno) { case EINTR: case EWOULDBLOCK: break;
     default: disconnect(); /*XXXX*/ }
 else if (!r)
  {
   /* XXX: this EOF handling is all rather slimy */
   if (mflagsession)
     disconnect();
   else
    {
     mflagreading = 0;
     disp_mflagreading();
    }
  }
 else { inbufread += r; disp_inbufread(); }
 verbose("master exiting readin");
}
static ss_sig *sigreadin = 0;
static void ckreadin()
{
 if (!sigreadin && (fdsigler != -1) && (inbufread < sizeof(inbuf))
   && mflagreading && !flagstopped)
  { ss_sched(sigreadin = ss_sigread(fdi),doreadin,0); return; }
 if (sigreadin && ((fdsigler == -1) || (inbufread == sizeof(inbuf))
   || !mflagreading || flagstopped))
  { ss_unsched(sigreadin,doreadin,0); sigreadin = 0; }
}

static void dowritein(n)
int n;
{
 int w;

 verbose("master entering writein");
 if (mflagxflowctl)
  {
   if (tty_spaceleft(mfdsty) < 128)
     return;
   /* XXX: this busy-loops! */
  }
 if (mflagxflowctl && (inbufread - inbufwrite > 128))
   w = write(mfdmty,inbuf + inbufwrite,128);
 else
   w = write(mfdmty,inbuf + inbufwrite,inbufread - inbufwrite);
 if (w == -1)
   switch(errno) { case EINTR: case EWOULDBLOCK: break;
     default: ; /*XXXX*/ }
 else
  {
   inbufwrite += w;
   if (inbufwrite == inbufread)
    { inbufwrite = inbufread = 0; disp_inbufread(); }
   disp_inbufwrite();
  }
 verbose("master exiting writein");
}
static ss_sig *sigwritein = 0;
static void ckwritein()
{
 if (!sigwritein && (mfdmty != -1) && (inbufwrite < inbufread))
  { ss_sched(sigwritein = ss_sigwrite(mfdmty),dowritein,0); return; }
 if (sigwritein && ((mfdmty == -1) || (inbufwrite == inbufread)))
  { ss_unsched(sigwritein,dowritein,0); sigwritein = 0; }
}

static void doreadout(n)
int n;
{
 int r;
 verbose("master entering doreadout");

 r = read(mfdmty,outbuf + outbufread,sizeof(outbuf) - outbufread);
 verbose("master doreadout: %d",r);
 if (r == -1)
   switch(errno) { case EINTR: case EWOULDBLOCK: break;
     default: ; /*XXXX*/ }
 else if (!r)
   ; /* XXX: EOF on a pty? utterly impossible */
 else { outbufread += r; disp_outbufread(); }
}
static ss_sig *sigreadout = 0;
static void ckreadout()
{
 if (!sigreadout && (mfdmty != -1) && (outbufread < sizeof(outbuf)))
  { ss_sched(sigreadout = ss_sigread(mfdmty),doreadout,0); return; }
 if (sigreadout && ((mfdmty == -1) || (outbufread == sizeof(outbuf))))
  { ss_unsched(sigreadout,doreadout,0); sigreadout = 0; }
}

static void dowriteout(n)
int n;
{
 int w;
 verbose("master entering writeout");
 /* XXX: Warning! We're manipulating an outside descriptor! */
 w = write(fdo,outbuf + outbufwrite,outbufread - outbufwrite);
 if (w == -1)
   switch(errno) { case EINTR: case EWOULDBLOCK: break;
   case EPIPE: disconnect(); /* ``the master treats PIPE like EOF'' */
   default: disconnect(); /*XXXX*/ }
 else
  {
   outbufwrite += w;
   if (outbufwrite == outbufread)
    { outbufwrite = outbufread = 0; disp_outbufread(); }
   disp_outbufwrite();
  }
 verbose("master exiting writeout");
}
static ss_sig *sigwriteout = 0;
static void ckwriteout()
{
 if (!sigwriteout && (fdsigler != -1) && (outbufwrite < outbufread) && !flagstopped)
  { ss_sched(sigwriteout = ss_sigwrite(fdo),dowriteout,0); return; }
 if (sigwriteout && ((fdsigler == -1) || (outbufwrite == outbufread) || flagstopped))
  { ss_unsched(sigwriteout,dowriteout,0); sigwriteout = 0; }
}

/*
We have ck{acceptfdctrlr,readfdctrlr,readin,writein,readout,writeout,
readsig2us,sigchld}.
When flagstopped changes, which ck*() have to be called?
The dispatchers know. disp_flagstopped, for instance.
It'd be interesting to see a programming language where these could
be generated automatically---it was semi-automatic by hand.
Of course, you could also look at this as an optimization problem...
*/

/* these aren't void 'cause forward static declarations are unportable */
static disp_mflagreading() { ckreadin(); }
static disp_flagstopped() { ckreadin(); ckwriteout(); }
static disp_flagwatchchld() { cksigchld(); }
static disp_mfdmty() { ckwritein(); ckreadout(); } /* ever? */
static disp_fdctrlr() { ckacceptfdctrlr(); ckreadfdctrlr(); }
static disp_fdsigler() { ckreadin(); ckwriteout(); }
static disp_fdsig2us() { ckreadsig2us(); }
static disp_inbufread() { ckreadin(); ckwritein(); }
static disp_inbufwrite() { ckwritein(); }
static disp_outbufread() { ckreadout(); ckwriteout(); cksigchld(); }
static disp_outbufwrite() { ckwriteout(); }

static void disp() { ckreadin(); ckwriteout(); ckreadout(); cksigchld();
  ckacceptfdctrlr(); ckreadfdctrlr(); ckwritein(); ckreadsig2us(); }

/*
Signal handling:

fdctrlr == -1 and mfdcomm readable: accept fdctrlr
fdctrlr != -1 and fdctrlr readable: read command (or close)
fdsig2us != -1 and fdsig2us readable: read stuff from sigler
fdsigler != -1 and fdi readable and ibuf okay and reading and !stopped: read
fdsigler != -1 and fdo readable and obuf nonempty and !stopped: write
mfdmty != -1 and mfdmty readable and obuf okay: read from mfdmty into buffer
mfdmty != -1 and mfdmty writable and ibuf nonempty: write

CHLD, while flagwatchchld: if exited, tell sigler exit status, and exit
  if stopped: if sigler, tell sigler to stop, else remember stop
HUP, TSTP, TTIN, TTOU, WINCH, INT, QUIT: ignore
PIPE: ignore (we handle EPIPE on write())

*/

void master(fdcomm,fdmty,fdsty,ext,uid,pid,flagsession,fdpreco,username,flagxflowctl)
int fdcomm;
int fdmty;
int fdsty;
char *ext;
int uid;
int pid; /* pid of child */
int flagsession;
int fdpreco;
char *username;
int flagxflowctl;
{
 musername = username;
 mflagsession = flagsession;
 mflagxflowctl = flagxflowctl;
 mflagreading = 0; /* will change with flagsigler */
 mfdcomm = fdcomm;
 mfdmty = fdmty;
 mfdsty = fdsty;
 muid = uid;
 mext[0] = ext[0];
 mext[1] = ext[1];
 mslavepid = pid;
 flagstopped = 0;
 fdsigler = -1;
 fdsig2us = -1;
 flagwatchchld = 0;
 fdi = -1;
 fdo = -1;
 flagpreco = 1;
 mfdpreco = fdpreco;

 recoext[0] = recoext[1] = 0;

 ss_forcewait(); /* to be turned off exactly once, namely when child exits */

 rallocneverfail(childdead); /* XXX: this is, arguably, suboptimal */

 disp(); /* starts up slave I/O and CHLD handler */
/*
We're already ignoring HUP, INT, TSTP, TTOU, TTIN, WINCH, QUIT, PIPE.

Other signals to worry about: URG and IO are ignored by default; fine.
USR1 and USR2 should never be generated.
TERM should never be generated.

The user can arrange for XCPU, XFSZ, PROF, VTALRM, and ALRM to be sent:
*/
 signal(SIGXCPU,SIG_IGN); /* XXX */
 signal(SIGXFSZ,SIG_IGN); /* we'll get notice on write() */
 signal(SIGALRM,SIG_IGN);
 signal(SIGVTALRM,SIG_IGN);
 signal(SIGPROF,SIG_IGN); /* and if we're being profiled, too bad! */

 signal(SIGTERM,SIG_IGN); /* XXX */
 signal(SIGUSR1,SIG_IGN); /* XXX */
 signal(SIGUSR2,SIG_IGN); /* XXX */

 closeallbut();
}
