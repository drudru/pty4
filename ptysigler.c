#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include "sigsched.h"
#include "sigdfl.h"
#include "fmt.h"
#include "sessconnlog.h"
#include "ptytty.h"
#include "ptycomm.h"
#include "ptymisc.h"
#include "ptyerr.h"
#include "config/ttyopts.h"
#include "ptysigler.h"
extern void doconnect(); /* XXX: gaargh */
extern void ckobey();

#define verbose 0, /*XXX*/

static int firsttime;

static char resp6[6];
static char *sremote;
static int sremotelen;

static int sflagttymodes; /* must be 0 if !sflagreading */
static struct ttymodes stmotty; /* only valid if sflagttymodes */
static struct ttymodes stmottyzero; /* only valid if sflagttymodes */
static int sflagreading;
static int sflagjobctrl;
static int fdmaster = -1;
static int fdus2master = -1;

static int fdtty = -1;

static int suid = -1;
static char recoext[2]; /* if null, then unset */

void lastmoment(n)
int n;
{
 /* clean up and die! */
 if (sflagttymodes)
   tty_setmodes(fdtty,&stmotty);
}

void stop(sig)
int sig;
{
 sigdfl(sig);
}

void byebye(sig)
int sig;
{
 lastmoment(0);
 sigdfl(sig);
 die(DIE_IMPOSSIBLE);
}

void obey(n)
int n;
{
 int r;
 char c[4];

 verbose("sigler entering obey");
 r = bread(fdmaster,c,4);
 verbose("sigler obeyread %d %c %d %d %d",r,c[0],c[1],c[2],c[3]);

 if (r <= 0)
   c[0] = 'd'; /* kludge alert! */

 switch(c[0])
  {
   case 'z': /* slave stopped, c[1] is stop signal */
     if (sflagjobctrl)
      {
       /* XXX: error checks! */
       if (sflagttymodes)
         tty_setmodes(fdtty,&stmotty);
       sigdfl(c[1]); /* my, we are so trusting... */
       if (sflagreading)
	 tty_forcefg(fdtty);
       if (sflagttymodes)
	 tty_setmodes(fdtty,&stmottyzero);
       bwrite(fdus2master,"C",1); /* XXX: error checks? */
      }
     break;
   case 'k': /* sesskill */
   case 'd': /* disconnect */
   case 'e': /* slave exited, c[1] is exit status */
   case 's': /* slave terminated by signal, c[1] is signum, c[2] is coredump */
     if (recoext[0])
       doconnect();
     else
      {
       close(fdmaster); fdmaster = -1;
       close(fdus2master); fdus2master = -1;
       ckobey();
       lastmoment(0);
      }
     break;
   case 'r': /* recoext, c[1] and c[2] */
     recoext[0] = c[1];
     recoext[1] = c[2];
     break;
  }
}
static ss_sig *sigobey = 0;
void ckobey()
{
 if (!sigobey && (fdmaster != -1))
  { ss_schedwait(sigobey = ss_sigread(fdmaster),obey,0,1); return; }
 if (sigobey && (fdmaster == -1))
  { ss_unsched(sigobey,obey,0); sigobey = 0; }
}

void doconnect()
{
 /* assumptions: */
 /* we are in the foreground if sflagreading */
 /* fd 0 is input, fd 1 is output, fd 2 is error---all open */
 /* tty is in mode smottyzero if sflagttymodes */

 int fdcomm;
 int pi[2];
 int sp[2];

 if (!firsttime)
  {
   char buf[50]; char *t; t = buf;
   t += fmt_strncpy(t,"reconnecting to ",0);
   *t++ = recoext[0]; *t++ = recoext[1]; *t = 0;
   warn("info",buf);
  }
 verbose("sigler entering doconnect");
 if (fdmaster != -1) { close(fdmaster); fdmaster = -1; }
 if (fdus2master != -1) { close(fdus2master); fdus2master = -1; }

 if ((pipe(pi) == -1) || (pipe(sp) == -1))
  {
   lastmoment(0);
   if (firsttime) /* hope not */
     warn("fatal","signaller cannot create internal pipe; master may still be running; use sesslist to check, then try sesskill or reconnect later");
   else
     warn("fatal","signaller cannot create internal pipe");
   die(DIE_IMPOSSIBLE);
  }

 fdcomm = comm_write(recoext,suid);
 if (fdcomm == -1) /* hope this isn't the first connect */
  {
   lastmoment(0);
   if (firsttime)
     warn("fatal","signaller cannot connect; master may still be running; use sesslist to check, then try sesskill or reconnect later");
   else
    {
     char buf[100]; char *t; t = buf;
     /* this is a typical case so we print a nice error message */
     t += fmt_strncpy(t,"signaller cannot reconnect to session ",0);
     t += fmt_strncpy(t,recoext,2);
     t += fmt_strncpy(t,"; does it exist?",0);
     *t = 0;
     warn("fatal",buf);
    }
   die(DIE_EXIST);
  }
 verbose("sigler opened connection");
 if (
     (bwrite(fdcomm,"r",1) != 1)
   ||(bread(fdcomm,resp6,6) != 6)
   ||(!respeq(resp6,"shrtng"))
   ||(comm_putfd(fdcomm,sp[1]) == -1)
   ||(comm_putfd(fdcomm,pi[0]) == -1)
   ||(comm_putfd(fdcomm,0) == -1)
   ||(comm_putfd(fdcomm,1) == -1)
   ||(bwrite(fdcomm,(char *) &sflagreading,sizeof(int)) != sizeof(int))
   ||(bwrite(fdcomm,(char *) &sremotelen,sizeof(int)) != sizeof(int))
   ||(bwrite(fdcomm,(char *) sremote,sremotelen) != sremotelen)
   ||(bread(fdcomm,resp6,6) != 6)
   ||(!respeq(resp6,"phew! "))
    )
  {
   lastmoment(0);
   if (firsttime)
     warn("fatal","signaller having trouble; master may still be running; use sesslist to check, then try sesskill or reconnect later");
   else
     if (respeq(resp6,"no-go!")) /* master's already connected */
      {
       warn("fatal","session already connected somewhere else");
       die(DIE_ELSE);
      }
     else
       warn("fatal","signaller having trouble");
   die(DIE_COMM);
  }
 close(sp[1]); fdmaster = sp[0];
 close(pi[0]); fdus2master = pi[1];

#ifdef TTY_WINDOWS
 /* Obviously there's a race here between the master reconnecting */
 /* and us telling the pty to change sizes. But will the slave */
 /* ever care? I'm not sure... */
 kill(getpid(),SIGWINCH); /* oh, what a royal kludge */
#endif

 close(fdcomm);

 verbose("sigler successful doconnect");
 if (!firsttime)
  {
   char buf[50]; char *t; t = buf;
   t += fmt_strncpy(t,"successfully connected to ",0);
   *t++ = recoext[0]; *t++ = recoext[1]; *t = 0;
   warn("info",buf);
  }
 recoext[0] = recoext[1] = 0;
 firsttime = 0;

 ckobey();
}

static void sigchld(n)
int n;
{
 int w;

 while (wait3(&w,WNOHANG | WUNTRACED,(struct rusage *) 0) > 0)
   ; /* [yawn] */
}

static void sigwinch(n)
int n;
{
#ifdef TTY_WINDOWS
 struct ttywin twi;
 struct ttymodes tmo;

 if (tty_getmodes(fdtty,&tmo) == 0)
  {
   tty_modes2win(&tmo,&twi);
   bwrite(fdus2master,"W",1);
   bwrite(fdus2master,(char *) &twi,sizeof(twi));
   /* XXX: error checks? */
  }
#endif
 ;
}

/*
Signal handling:

TTIN, TTOU: will never happen, as we don't do I/O; default if they do happen
PIPE: ditto; default if it does happen
HUP, INT, QUIT: default. we'll die, master will see socket close.
  XXX: There's a race here to reconnect to the master initially...
TSTP: default. master won't see it.

CHLD: could be master, or a child from before we were execed; ignore both
  XXX: There's a race upon exiting to avoid making zombies...
WINCH, and after every manual continue: tell master to winch

when fdmaster != -1 and it's readable: do obey(). This keeps us going.
A side effect of this strategy is that if we ever rest for a moment
without a master to obey, we die. How poetic. Should this be called the
slave process?
*/

void sigler(ext,uid,pid,flagttymodes,tmotty,tmottyzero,flagreading,flagjobctrl,remote)
char *ext;
int uid;
int pid; /* process id of master---not that we care */
int flagttymodes;
struct ttymodes *tmotty;
struct ttymodes *tmottyzero;
int flagreading;
int flagjobctrl;
char *remote;
{
 /* TTOU, TTIN, PIPE are already SIG_DFL */
 ss_sched(ss_signal(SIGINT),byebye,SIGINT);
 ss_sched(ss_signal(SIGHUP),byebye,SIGHUP);
 ss_sched(ss_signal(SIGTSTP),stop,SIGTSTP);
 ss_sched(ss_signal(SIGQUIT),byebye,SIGQUIT);
 ss_sched(ss_signal(SIGCHLD),sigchld,0);
#ifdef TTY_WINDOWS
 ss_sched(ss_signal(SIGWINCH),sigwinch,0);
#endif

 suid = uid;

 sremote = remote;
 sremotelen = strlen(remote) + 1;
 if (sremotelen > SESSCONNLOG_REMOTELEN)
   sremotelen = SESSCONNLOG_REMOTELEN;
 sflagttymodes = flagttymodes;
 sflagreading = flagreading;
 sflagjobctrl = flagjobctrl;
 tty_copymodes(&stmotty,tmotty);
 tty_copymodes(&stmottyzero,tmottyzero);

 if (flagttymodes)
   fdtty = tty_getctrl();
   /* XXX: what if it's -1? is this even remotely possible? */

 firsttime = 1;

 recoext[0] = ext[0]; recoext[1] = ext[1];
 doconnect();
}
