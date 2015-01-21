/* tplay.c: play back a tape
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on sigsched.h, timer.h.
Requires read()/write(), i.e., UNIX.
7/27/91: Baseline. tplay 1.0, public domain.
No known patent problems.

Documentation in tplay.1.

*/

#include "sigsched.h"
#include "timer.h"

static char hdr[8];
static char buf[256];
static timer_clock latest;
static timer_sig tsig;
static int flageof = 0;

void doit(n)
int n; /* if nonzero, there's data waiting to be written */
{
 int r;
 timer_clock now;
 timer_clock diff;
 int x;

 while (!flageof)
  {
   x = 1 + (unsigned int) (unsigned char) hdr[7];
   if (n)
    {
     n = 0;
     while (n < x)
      {
       r = write(1,buf + n,x - n);
       if (r <= 0) ; /*XXX*/
       n += r;
      }
    }
   r = read(0,hdr,8); /* eighth byte is length of data */
   if (r == -1)
     ; /*XXX*/
   if (r < 7)
     ; /*XXX*/
   if (r == 7)
     flageof = 1;
   else
    {
     x = 1 + (unsigned int) (unsigned char) hdr[7];
     n = 0;
     while (n < x)
      {
       r = read(0,buf + n,x - n);
       if (r == -1) ; /*XXX*/
       if (r == 0) ; /*XXX*/
       n += r;
      }
    }
   diff.usec = (hdr[2] * 256 + hdr[1]) * 256 + hdr[0];
   diff.sec = ((hdr[6] * 256 + hdr[5]) * 256 + hdr[4]) * 256 + hdr[3];
   if (diff.sec || diff.usec)
    {
     now = latest; /*XXX: structure copying */
     timer_sum(&now,&diff,&latest);
     timer_setsig(&tsig,TIMER_REAL,&latest);
     ss_schedonce(&tsig.sig,doit,1);
     break;
    }
  }
}

main()
{
 timer_now(TIMER_REAL,&latest);
 ss_schedonce(ss_asap(),doit,0);
 timer_init();
 ss_exec();
 exit(0);
}
