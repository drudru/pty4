/* sigdfl.c, sigdfl.h: default signal library
Daniel J. Bernstein, brnstnd@nyu.edu.
No dependencies.
Requires BSD signal syscalls.
1/27/92: Added ABRT->IOT ifndef.
1/27/92: Switched from setting sv_flags to manually zeroing sigvecs.
7/18/91: Baseline. sigdfl 1.0, public domain.
No known patent problems.

Documentation in sigdfl.3.

If you port this to System V, you don't have to worry about stop
signals; you can just have sigdfl_tstp() and friends return -1 with
ENOTTY. You are, however, faced with an incomplete, nearly prehistoric
signal interface. Have fun.
*/

#include <signal.h>
#include "sigdfl.h"

static int cont = 0;

static sigcont() /* XXX: should declare with right signal type */
{
 cont = 1;
}

int sigdfl(sig)
int sig;
{
 int oldmask;
 struct sigvec oldvec;
 struct sigvec vec;
 struct sigvec contvec;
 char *x;

 if (sig == SIGCONT)
   return 0; /* strategy below simply cannot work for CONT */
 if ((sig == SIGURG) || (sig == SIGCHLD) || (sig == SIGIO)
#ifdef SIGWINCH
   || (sig == SIGWINCH)
#endif
    )
   return 0; /* the above signals are ignored */
 /* XXX: If we're still going now, and sig can be delivered without */
 /* killing the process and without stopping the process so that it'll */
 /* receive CONT later, then we will enter an infinite loop. [sigh] */
 /* XXX: put maximum time wastage on this? */
 oldmask = sigblock(0);
 sigblock(~0);
 /* now we won't receive any signals */
 x = (char *) &vec; while (x < sizeof(vec) + (char *) &vec) *x++ = 0;
 vec.sv_handler = SIG_DFL;
 vec.sv_mask = ~0;
 if (sigvec(sig,&vec,&oldvec) == -1)
   if ((sig != SIGSTOP) && (sig != SIGKILL))
     return -1;
 x = (char *) &vec; while (x < sizeof(vec) + (char *) &vec) *x++ = 0;
 vec.sv_handler = sigcont;
 vec.sv_mask = ~0;
 if (sigvec(SIGCONT,&vec,&contvec) == -1)
   return -1;
 cont = 0;
 if (kill(getpid(),sig) == -1)
   return -1;
 /* now a sig should be queued, and we have control over sig and CONT */
 /* exception: SIGSTOP and SIGKILL can't be blocked, so those signals
    might already have been delivered. in the SIGSTOP case, if we've
    reached this point, sigcont() might already have been run. that's
    why cont must be set to 0 before the kill(). */
 /* after this next bit we may receive sig and/or CONT */
 sigsetmask(~(sigmask(sig) | sigmask(SIGCONT)));
 /* in the near future, sig will in fact be received */
 while (!cont) /* dead loop until we receive CONT */
   ; /* XXX: there should be a syscall so we don't have to loop here */
 sigblock(~0);
 /* now we won't receive any signals */
 (void) sigvec(sig,&oldvec,&vec); /* we don't care if it fails */
 (void) sigvec(SIGCONT,&contvec,&vec);
 /* now signal handlers are back to normal */
 (void) sigsetmask(oldmask);
 return 0;
}

int sigdfl_tstp()
{
 return sigdfl(SIGTSTP);
}

int sigdfl_stop()
{
 return sigdfl(SIGSTOP);
}

int sigdfl_ttin()
{
 return sigdfl(SIGTTIN);
}

int sigdfl_ttou()
{
 return sigdfl(SIGTTOU);
}

int sigdfl_abrt()
{
#ifndef SIGABRT
#define SIGABRT SIGIOT
#endif
 return sigdfl(SIGABRT);
}
