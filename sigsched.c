/* sigsched.c, sigsched.h: signal-schedule thread library
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on ralloc.h, sod.h, config/fdsettrouble.h.
Requires BSDish environment: reliable signals, sig{vec,block,setmask}, select.
9/1/91: Added worst-case fdset, FD_ZERO, etc. definitions.
8/25/91: sigsched 1.1, public domain.
8/25/91: Fixed bug that sigs[sched->blah].r didn't force instant timeout.
8/25/91: Fixed bug that if select() returned -1 then fds were still checked.
7/21/91: Changed forever to a 1-hour wakeup.
7/19/91: Added isopen() to fix bug in case of bad descriptor.
7/18/91: Baseline. sigsched 1.0, public domain.
No known patent problems.

Documentation in sigsched.3.

XXX: how well do we clean up upon ss_exec() exit?

*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
extern int errno;
#include "config/fdsettrouble.h"
#include "sigsched.h"
#include "ralloc.h"
#include "sod.h"

/* XXX: should restore signal set exactly after ss_exec returns */

typedef int sigc_set; /*XXX */

#define sigc_ismember(x,i) (*(x) & (1 << ((i) - 1)))
#define sigc_addset(x,i) (*(x) |= (1 << ((i) - 1)))
#define sigc_emptyset(x) (*(x) = 0)

/*       sigprocmask(SIG_UNBLOCK,xxxx,(sigc_set *) 0); */
#define sigc_unblock(x) (sigsetmask(sigblock(0) & ~*(x)))
/*       sigprocmask(SIG_BLOCK,xxxx,(sigc_set *) 0); */
#define sigc_block(x) (sigblock(*(x)))

#ifndef NSIG
#define NSIG 64 /* it's not as if any sane system has more than 32 */
#endif

#define NUMSIGS NSIG

#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif

#define NUMFDS FD_SETSIZE /* if select() can't handle it, we can't either */

#ifdef LACKING_FD_ZERO
#define NFDBITS	(sizeof(fd_mask) * NBBY)
#define	FD_SET(n,p) ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_ISSET(n,p) ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p) bzero((caddr_t)(p),sizeof(*(p)))
#endif

#ifdef DESPERATION_FD_SET
#undef NFDBITS
#undef FD_SET
#undef FD_ISSET
#undef FD_ZERO
#undef fd_set
#define fd_set long
#define FD_SET(n,p) ((*p) |= (1 << (n)))
#define FD_ISSET(n,p) ((*p) & (1 << (n)))
#define FD_ZERO(p) (*p = 0L)
#endif

#define ASAP 1
#define SIGNAL 2
#define READ 3
#define WRITE 4
#define EXCEPT 5
#define JUNK 6
#define EXTERN 7

typedef struct { ss_sig s; int r; } ss_sigplus;

static ss_sigplus asap;
static ss_sigplus sigs[NUMSIGS];
static ss_sigplus reads[NUMFDS];
static ss_sigplus writes[NUMFDS];
static ss_sigplus excepts[NUMFDS];
static ss_sigplus junk; /* special case for internal use */

static void initsigs()
{
 int i;
 asap.s.type = ASAP; asap.s.u.n = 0; asap.r = 0;
 for (i = 0;i < NUMSIGS;++i)
  { sigs[i].s.type = SIGNAL; sigs[i].s.u.n = i; sigs[i].r = 0; }
 for (i = 0;i < NUMFDS;++i)
  { reads[i].s.type = READ; reads[i].s.u.n = i; reads[i].r = 0; }
 for (i = 0;i < NUMFDS;++i)
  { writes[i].s.type = WRITE; writes[i].s.u.n = i; writes[i].r = 0; }
 for (i = 0;i < NUMFDS;++i)
  { excepts[i].s.type = EXCEPT; excepts[i].s.u.n = i; excepts[i].r = 0; }
 junk.s.type = JUNK; junk.s.u.n = 0; junk.r = 0;
}

ss_sig *ss_asap()
{ return &(asap.s); }
#define OKsig(i) ((i >= 0) && (i < NUMSIGS))
ss_sig *ss_signal(i) int i;
{ if (!OKsig(i)) return 0; return &(sigs[i].s); }
#define OKfd(fd) ((fd >= 0) && (fd < NUMFDS))
ss_sig *ss_sigread(fd) int fd;
{ if (!OKfd(fd)) return 0; return &(reads[fd].s); }
ss_sig *ss_sigwrite(fd) int fd;
{ if (!OKfd(fd)) return 0; return &(writes[fd].s); }
ss_sig *ss_sigexcept(fd) int fd;
{ if (!OKfd(fd)) return 0; return &(excepts[fd].s); }

void ss_externsetsig(sig,x)
ss_sig *sig;
ss_extern *x;
{
 sig->type = EXTERN;
 sig->u.c = (char *) x;
}

struct sched
 {
  ss_sig *sig;
  ss_thread *t;
  union { ss_id i; ss_idptr p; } id;
  int flagi;
  int wait;
 }
;

SODdecl(schedlist,struct sched);

static schedlist schedhead = 0;
static int schednum = 0;
static int schedjunked = 0;
static int numwait = 0;

void ss_forcewait()
{
 ++numwait;
}

void ss_unforcewait()
{
 --numwait;
}

int ss_schedvwait(sig,t,flagi,i,p,wait)
ss_sig *sig;
ss_thread *t;
int flagi;
ss_id i;
ss_idptr p;
int wait;
{
 schedlist s;

 if (sig->type == EXTERN)
  {
   ss_extern *x;
   x = (ss_extern *) sig->u.c;
   return x->sched(x,t,flagi,i,p,wait);
  }
 s = SODalloc(schedlist,s,ralloc);
 if (!s)
   return -1;
 SODdata(s).sig = sig;
 SODdata(s).t = t;
 if (SODdata(s).flagi = flagi)
   SODdata(s).id.i = i;
 else
   SODdata(s).id.p = p;
 SODdata(s).wait = wait;
 SODpush(schedhead,s);
 ++schednum;
 if (wait)
   ++numwait;
 return 0;
}

int ss_schedwait(sig,t,i,wait)
ss_sig *sig;
ss_thread *t;
ss_id i;
int wait;
{
 return ss_schedvwait(sig,t,1,i,(ss_idptr) 0,wait);
}

int ss_sched(sig,t,i)
ss_sig *sig;
ss_thread *t;
ss_id i;
{
 return ss_schedvwait(sig,t,1,i,(ss_idptr) 0,0);
}

struct oncestuff { ss_sig *sig; ss_thread *t; ss_id i; } ;
/* XXX: this is the same as some other struct */

static void once(p)
ss_idptr p;
{
 struct oncestuff *os;
 os = (struct oncestuff *) p;
 if (ss_unschedv(os->sig,once,0,0,p) == -1)
   ; /* impossible */
 os->t(os->i);
 RFREE(os);
}

int ss_schedonce(sig,t,i)
ss_sig *sig;
ss_thread *t;
ss_id i;
{
 struct oncestuff *os;

 os = (struct oncestuff *) ralloc(sizeof(struct oncestuff));
 if (!os)
   return -1;
 os->sig = sig; os->t = t; os->i = i;
 return ss_schedvwait(sig,once,0,0,(ss_idptr) os,1);
}

/* XXX: could rallocinstall() this, if it has the recvhead() test */

static int schedcleanup()
{
 schedlist s;
 schedlist t;
 schedlist sprev;

/*  if (recvhead) return 0;  XXX: needs recvhead in scope */

 if (!schedjunked)
   return 0;

 sprev = 0;
 s = schedhead;
 while (s)
  {
   if (SODdata(s).sig == &(junk.s))
    {
     if (sprev)
      {
       SODpop(SODnext(sprev),t); /* XXX: not part of official sod interface */
       s = SODnext(sprev);
      }
     else
      {
       SODpop(s,t);
       schedhead = s;
      }
     SODfree(t,rfree);
     --schednum;
     --schedjunked;
    }
   else
    {
     sprev = s;
     s = SODnext(s);
    }
  }

/* schednum -= schedjunked; now done dynamically inside loop */
/* schedjunked = 0; */
 return 1;
}

static void nothing(id)
ss_id id;
{
 ;
}

int ss_unschedv(sig,t,flagi,i,p)
ss_sig *sig;
ss_thread *t;
int flagi;
ss_id i;
ss_idptr p;
{
 schedlist s;

 if (sig->type == EXTERN)
  {
   ss_extern *x;
   x = (ss_extern *) sig->u.c;
   return x->unsched(x,t,flagi,i,p);
  }
 for (s = schedhead;s;s = SODnext(s))
   if (SODdata(s).sig == sig && SODdata(s).t == t)
     if (SODdata(s).flagi == flagi)
       if (flagi ? (SODdata(s).id.i == i) : (SODdata(s).id.p == p))
        {
         SODdata(s).sig = &(junk.s);
         SODdata(s).t = nothing; /* just in case */
         if (SODdata(s).wait)
           --numwait;
         SODdata(s).wait = 0;
         ++schedjunked;
         return 0;
        }
 return 1;
}

int ss_unsched(sig,t,i)
ss_sig *sig;
ss_thread *t;
ss_id i;
{
 return ss_unschedv(sig,t,1,i,(ss_idptr) 0);
}

static struct timeval timeout;
static struct timeval instant = { 0, 0 };
static struct timeval forever = { 3600, 0 };
  /* XXX: talk to me */

static void handle(i)
int i;
{
 timeout = instant; /* XXX: structure copying */
 sigs[i].r = 1;
}

static sigc_set sigstorage;
static sigc_set *xxxx = 0;

int ss_addsig(i)
int i;
{
 if (!OKsig(i))
   return -1;
 if (!xxxx)
  {
   xxxx = &sigstorage;
   sigc_emptyset(xxxx);
  }
 sigc_addset(xxxx,i);
 return 0;
}

static int isopen(fd)
int fd;
{
 /* XXX: should call this only if select() fails */
 return fcntl(fd,F_GETFL,0) != -1;
}

SODdecl(recvlist,schedlist);

int ss_exec()
{
 int i;
 struct sigvec sv;
 recvlist recvhead;
 recvlist temp;
 schedlist sch;

 initsigs();

 if (xxxx)
  {
   sigc_block(xxxx);

   sv.sv_handler = handle;
   sv.sv_mask = *xxxx; /* so handle won't interrupt itself */
   sv.sv_flags = 0;

   /* XXX: Does anyone but me find it absolutely idiotic that POSIX
      doesn't provide a way to get each member of a signal set in turn? */
   for (i = 0;i < NUMSIGS;i++)
    {
     if (sigc_ismember(xxxx,i))
       if (sigvec(i,&sv,(struct sigvec *) 0) == -1) /*XXX: really trash orig? */
	 ; /* not our problem */
    }
  }

 recvhead = 0;

 while (numwait)
  {
   if (recvhead)
    {
     int w;
     SODpop(recvhead,temp);
     sch = SODdata(temp);

/* This is the only section where we call user code. */
#define DOIT \
if (SODdata(sch).flagi) \
  SODdata(sch).t(SODdata(sch).id.i); \
else \
  SODdata(sch).t(SODdata(sch).id.p);

     switch(SODdata(sch).sig->type)
      {
       case JUNK:
	 break; /* has been unscheduled while waiting on the receive list */
       case ASAP:
	 DOIT
	 break;
       case READ:
	 if (reads[w = SODdata(sch).sig->u.n].r)
	   DOIT
	 reads[w].r = 0;
	 break;
       case WRITE:
	 if (writes[w = SODdata(sch).sig->u.n].r)
	   DOIT
	 writes[w].r = 0;
	 break;
       case EXCEPT:
	 if (excepts[w = SODdata(sch).sig->u.n].r)
	   DOIT
	 excepts[w].r = 0;
	 break;
       case SIGNAL:
	 if (sigs[w = SODdata(sch).sig->u.n].r)
	   DOIT
	 sigs[w].r = 0;
	   /* ``after the end of the last...'' */
	 break;
       case EXTERN:
	 /* by definition, an external library handles this */
	 break;
       default: /* XXX: huh? */
	 ;
      }
     SODfree(temp,rfree);
    }
   else
    {
     schedlist sp;
     static fd_set rfds;
     static fd_set wfds;
     static fd_set efds;
     static int maxfd;
     int r;

     if (schedjunked > 100)
       if (schednum / schedjunked < 3)
         (void) schedcleanup(); /* now's as good a time as any */

     timeout = forever;
     FD_ZERO(&rfds);
     FD_ZERO(&wfds);
     FD_ZERO(&efds);
     maxfd = -1;

     for (sp = schedhead;sp;sp = SODnext(sp))
      {
       switch(SODdata(sp).sig->type)
	{
	 case JUNK:
	   break;
	 case ASAP:
	   timeout = instant;
	   break;
	 case SIGNAL:
	   if (sigs[SODdata(sp).sig->u.n].r)
	     timeout = instant;
	   break;
	 case READ:
	   if (isopen(SODdata(sp).sig->u.n))
	     FD_SET(SODdata(sp).sig->u.n,&rfds);
	   if (SODdata(sp).sig->u.n > maxfd)
	     maxfd = SODdata(sp).sig->u.n;
	   break;
	 case WRITE:
	   if (isopen(SODdata(sp).sig->u.n))
	     FD_SET(SODdata(sp).sig->u.n,&wfds);
	   if (SODdata(sp).sig->u.n > maxfd)
	     maxfd = SODdata(sp).sig->u.n;
	   break;
	 case EXCEPT:
	   if (isopen(SODdata(sp).sig->u.n))
	     FD_SET(SODdata(sp).sig->u.n,&efds);
	   if (SODdata(sp).sig->u.n > maxfd)
	     maxfd = SODdata(sp).sig->u.n;
	   break;
	 case EXTERN:
	   break;
	 default: /*XXX: huh? */
	   break;
	}
      }

     if (xxxx)
       sigc_unblock(xxxx);
     /* This is the only section where handle() can be called. */
     /* XXX: If maxfd == -1, this select functions as a pause. */
     /* XXX: If maxfd == -1 and timeout is instant, should skip select. */
     /* XXX: Random bug of note: Real BSD systems will say that the
        fd is writable as soon as a network connect() fails. The first
	I/O will show the error (though it's rather stupid that you
	can't find out the error without doing I/O). What does Ultrix
	4.1 do? It pauses for 75 seconds. Dolts. */
     r = select(maxfd + 1,&rfds,&wfds,&efds,&timeout);
       /* XXX: does this necessarily prevent timeout race conditions? */
     if (xxxx)
       sigc_block(xxxx);

     if (r == -1)
      {
       FD_ZERO(&rfds);
       FD_ZERO(&wfds);
       FD_ZERO(&efds);
       switch(errno)
        {
         case EINTR: /* fine, this will happen on any signal */ break;
         case EBADF: /* who knows? */ break;
         case EINVAL: /* simply impossible */ break;
         default: /*XXX*/ ;
	 /* well, that was real useful */
        }
      }

     for (sp = schedhead;sp;sp = SODnext(sp))
      {
       switch(SODdata(sp).sig->type) 
	{
	 case JUNK:
	   break;
	 case ASAP:
           temp = SODalloc(recvlist,temp,ralloc);
           if (!temp)
             return -1; /*XXX*/
	   SODdata(temp) = sp;
           SODpush(recvhead,temp);
	   break;
	 case SIGNAL:
	   if (sigs[SODdata(sp).sig->u.n].r)
	    {
             temp = SODalloc(recvlist,temp,ralloc);
             if (!temp)
               return -1; /*XXX*/
	     SODdata(temp) = sp;
             SODpush(recvhead,temp);
	    }
	   break;
	 case READ:
	   if (FD_ISSET(SODdata(sp).sig->u.n,&rfds))
	    {
	     FD_CLR(SODdata(sp).sig->u.n,&rfds);
	     reads[SODdata(sp).sig->u.n].r = 1;
             temp = SODalloc(recvlist,temp,ralloc);
             if (!temp)
               return -1; /*XXX*/
	     SODdata(temp) = sp;
             SODpush(recvhead,temp);
	    }
	   break;
	 case WRITE:
	   if (FD_ISSET(SODdata(sp).sig->u.n,&wfds))
	    {
	     FD_CLR(SODdata(sp).sig->u.n,&wfds);
	     writes[SODdata(sp).sig->u.n].r = 1;
             temp = SODalloc(recvlist,temp,ralloc);
             if (!temp)
               return -1; /*XXX*/
	     SODdata(temp) = sp;
             SODpush(recvhead,temp);
	    }
	   break;
	 case EXCEPT:
	   if (FD_ISSET(SODdata(sp).sig->u.n,&efds))
	    {
	     FD_CLR(SODdata(sp).sig->u.n,&efds);
	     excepts[SODdata(sp).sig->u.n].r = 1;
             temp = SODalloc(recvlist,temp,ralloc);
             if (!temp)
               return -1; /*XXX*/
	     SODdata(temp) = sp;
             SODpush(recvhead,temp);
	    }
	   break;
	 case EXTERN:
	   break;
	 default:
	   break;
	}
      }
    }
  }
 if (xxxx)
   sigc_unblock(xxxx);
   /* XXX: should put this at other returns as well */
 return 0;
}
