/* timer.c, timer.h: timer libraries
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on sigsched.h, ralloc.h, sod.h.
Requires BSD (interval timers, PROF and VTALRM signals, gettimeofday, etc.).
7/27/91: Baseline. timer 1.0, public domain.
No known patent problems.

All signals defined here are thread-lowered signals.

Note that we use timer_clock instead of struct timeval since
timer_clock has at least a prayer of being portable.
This implementation, however, is BSD-only.
*/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <signal.h>
#include "sigsched.h"
#include "sod.h"
#include "timer.h"
#include "ralloc.h"
#ifndef HZ
#define HZ 60 /*XXX*/
#endif

int timer_now(t,result)
timer_type t;
timer_clock *result;
{
 struct tms tms;
 struct timeval tv;

 switch(t)
  {
   case TIMER_REAL:
     if (gettimeofday(&tv,(struct timezone *) 0) == -1)
       return -1;
     result->sec = tv.tv_sec;
     result->usec = tv.tv_usec;
     break;
   case TIMER_VIRTUAL:
     times(&tms);
     result->sec = tms.tms_utime / HZ;
     result->usec = ((tms.tms_utime % HZ) * 1000000) / HZ;
     break;
   case TIMER_PROF:
     times(&tms);
     result->sec = (tms.tms_utime + tms.tms_stime) / HZ;
     result->usec = (((tms.tms_utime + tms.tms_stime) % HZ) * 1000000) / HZ;
     break;
   default:
     return -1;
  }
 return 0;
}

void timer_sum(one,two,result)
timer_clock *one;
timer_clock *two;
timer_clock *result;
{
 result->sec = one->sec + two->sec;
 if ((result->usec = one->usec + two->usec) >= 1000000)
  {
   result->sec += 1;
   result->usec -= 1000000;
  }
}

int timer_diff(one,two,result)
timer_clock *one;
timer_clock *two;
timer_clock *result;
{
 if (one->sec > two->sec)
  {
   result->sec = one->sec - two->sec;
   if (one->usec >= two->usec)
     result->usec = one->usec - two->usec;
   else
    { --result->sec; result->usec = 1000000 - (two->usec - one->usec); }
   return 1;
  }
 if (one->sec < two->sec)
  {
   result->sec = two->sec - one->sec;
   if (two->usec >= one->usec)
     result->usec = two->usec - one->usec;
   else
    { --result->sec; result->usec = 1000000 - (one->usec - two->usec); }
   return -1;
  }
 if (one->usec > two->usec)
  {
   result->sec = 0;
   result->usec = one->usec - two->usec;
   return 1;
  }
 if (one->usec < two->usec)
  {
   result->sec = 0;
   result->usec = two->usec - one->usec;
   return 1;
  }
 result->sec = 0;
 result->usec = 0;
 return 0;
}

/* Basic idea: For each kind of timer, keep a list of all scheduled */
/* events. Set the interval timers to go off at the first events. */

struct kaboom { timer_clock when; ss_thread *t; int flagi; ss_id i; ss_idptr p; int wait; } ;

SODdecl(kaboomlist,struct kaboom);

/* XXX: should use priority queues here */

static int numwait = 0;

/* XXX: all these will have to change if TIMER_NUM changes */
static kaboomlist thead[TIMER_NUM] = { 0, 0, 0 };
static int tgoing[TIMER_NUM] = { 0, 0, 0 };
static timer_clock twhen[TIMER_NUM];
static int t2sig[TIMER_NUM] = { SIGALRM, SIGVTALRM, SIGPROF };
static int t2it[TIMER_NUM] = { ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF };

static void kaboomcleanup()
{
 kaboomlist newhead;
 kaboomlist k;
 timer_type i;

 for (i = 0;i < TIMER_NUM;++i)
  {
   newhead = 0;
   while (thead[i])
    {
     SODpop(thead[i],k);
     if (SODdata(k).t) SODpush(newhead,k); else SODfree(k,rfree);
    }
   thead[i] = newhead;
  }
}

static void kaboom(t)
timer_type t;
{
 kaboomlist k;
 timer_clock dummy;
 ss_thread *thread;

 ss_unsched(ss_signal(t2sig[t]),kaboom,t);
 tgoing[t] = 0; /* timer's out */
 for (k = thead[t];k;k = SODnext(k))
   if (timer_diff(&(SODdata(k).when),twhen + t,&dummy) <= 0)
    {
     thread = SODdata(k).t; SODdata(k).t = 0;
     if (SODdata(k).wait) --numwait; SODdata(k).wait = 0;
     if (thread)
       if (SODdata(k).flagi)
	 thread(SODdata(k).i);
       else
	 thread(SODdata(k).p);
     /* k may now be invalid. alternative: ss_schedonce(ss_asap(),...) */
     break; /* important! */
    }
 kaboomcleanup();
 if (kaboomresched(t) == -1)
   ; /* XXXXXX: uh-oh */
}

static int set_it(t,when)
timer_type t;
timer_clock *when;
{
 struct itimerval it;
 timer_clock now;
 timer_clock diff;

 if (timer_now(t,&now) == -1)
   return -1;
 if (timer_diff(when,&now,&diff) <= 0)
  {
   diff.sec = 0;
   diff.usec = 1;
  }
 it.it_value.tv_sec = diff.sec; it.it_interval.tv_sec = 0;
 it.it_value.tv_usec = diff.usec; it.it_interval.tv_usec = 0;
 if (setitimer(t2it[t],&it,(struct itimerval *) 0) == -1)
   return -1;
 return 0;
}

static int kaboomresched(t) /* XXX: implicit-static */
timer_type t;
{
 timer_clock dummy;
 kaboomlist k;
 int resched;

 if (thead[t])
  {
   resched = 0;
   for (k = thead[t];k;k = SODnext(k))
     if (SODdata(k).t)
       if (!tgoing[t] || timer_diff(&SODdata(k).when,twhen + t,&dummy) < 0)
	{
	 resched = 1;
	 twhen[t] = SODdata(k).when; /*XXX: structure copying*/
	}
   if (resched)
    {
     if (tgoing[t])
       ss_unsched(ss_signal(t2sig[t]),kaboom,t);
     tgoing[t] = 1;
     if (ss_schedwait(ss_signal(t2sig[t]),kaboom,t,numwait) == -1)
       return -1; /*XXX*/
     if (set_it(t,twhen + t) == -1)
       return -1; /*XXX*/
    }
  }
 return 0;
}

static int timer_sched(x,t,flagi,i,p,wait)
ss_extern *x;
ss_thread *t;
int flagi;
ss_id i;
ss_idptr p;
int wait;
{
 timer_sig *tsig;
 kaboomlist k;
 int resched;
 timer_clock dummy;

 k = SODalloc(kaboomlist,k,ralloc);
 if (!k)
   return -1;
 tsig = (timer_sig *) x->u.c;
 SODdata(k).when.sec = tsig->when.sec;
 SODdata(k).when.usec = tsig->when.usec;
 SODdata(k).t = t;
 SODdata(k).flagi = flagi;
 SODdata(k).i = i;
 SODdata(k).p = p;
 SODdata(k).wait = wait;

 resched = 0;
 if (wait)
   if (!numwait++)
     resched = 1;
 SODpush(thead[tsig->t],k);
 if (!tgoing[tsig->t] || (timer_diff(&(SODdata(k).when),twhen + tsig->t,&dummy) < 0))
   resched |= 2;
 if (resched)
  {
   if (tgoing[tsig->t])
     ss_unsched(ss_signal(t2sig[tsig->t]),kaboom,tsig->t);
   if (ss_schedwait(ss_signal(t2sig[tsig->t]),kaboom,tsig->t,numwait) == -1)
     return -1;
   tgoing[tsig->t] = 1;
   if (resched & 2) twhen[tsig->t] = SODdata(k).when; /*XXX: struct copying*/
   if (set_it(tsig->t,twhen + tsig->t) == -1)
     return -1;
  }
 return 0;
}

static int timer_unsched(x,t,flagi,i,p)
ss_extern *x;
ss_thread *t;
int flagi;
ss_id i;
ss_idptr p;
{
 timer_sig *tsig;
 kaboomlist k;
 struct kaboom *sk;

 tsig = (timer_sig *) x->u.c;
 for (k = thead[tsig->t];k;k = SODnext(k))
  {
   sk = &(SODdata(k));
   if ((sk->t == t) && (sk->flagi == flagi) && (sk->i == i) && (sk->p == p))
     if ((sk->when.usec == tsig->when.usec) && (sk->when.sec == tsig->when.usec))
      {
       sk->t = 0;
       if (sk->wait)
	 --numwait;
       sk->wait = 0;
      }
  }
 kaboomcleanup();
 if (kaboomresched(tsig->t) == -1)
   return -1;
 return 0;
}

void timer_setsig(tsig,t,when)
timer_sig *tsig;
timer_type t;
timer_clock *when;
{
 tsig->x.sched = timer_sched;
 tsig->x.unsched = timer_unsched;
 tsig->x.u.c = (char *) tsig; /* my, aren't we the clever ones today */
 tsig->t = t;
 tsig->when.sec = when->sec;
 tsig->when.usec = when->usec;
 ss_externsetsig(&(tsig->sig),&(tsig->x));
}

static int init = 0;

int timer_init()
{
 struct itimerval it;
 it.it_value.tv_sec = 0; it.it_value.tv_usec = 0;
 it.it_interval.tv_sec = 0; it.it_interval.tv_usec = 0;
 if (init)
   return 0;
 init = 1;
 if (ss_addsig(SIGALRM) == -1)
   return -1;
 if (setitimer(ITIMER_REAL,&it,(struct itimerval *) 0) == -1)
   return -1;
 if (ss_addsig(SIGPROF) == -1)
   return -1;
 if (setitimer(ITIMER_PROF,&it,(struct itimerval *) 0) == -1)
   return -1;
 if (ss_addsig(SIGVTALRM) == -1)
   return -1;
 if (setitimer(ITIMER_VIRTUAL,&it,(struct itimerval *) 0) == -1)
   return -1;
 /* we may end up receiving up to one of each signal, but that's okay */
 return 0;
}
