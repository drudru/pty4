/* trecord.c: tape-record the input
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on timer.h.
Requires read()/write(), i.e., UNIX.
7/27/91: Baseline. trecord 1.0, public domain.
No known patent problems.

Documentation in trecord.1.
*/

#include "timer.h"

static char buf[8 + 256];

main()
{
 timer_clock latest;
 timer_clock now;
 timer_clock diff;
 int r;
 int n;
 int w;

 if (timer_now(TIMER_REAL,&latest) == -1)
   ; /*XXX*/
 do
  {
   r = read(0,buf + 8,sizeof(buf) - 8);
   if (r == -1)
     ; /*XXX*/
   if (timer_now(TIMER_REAL,&now) == -1)
     ; /*XXX*/
   if (timer_diff(&now,&latest,&diff) < 0)
     ; /* time warp! */
   latest = now;
   buf[0] = diff.usec & 255;
   buf[1] = (diff.usec / 256) & 255;
   buf[2] = (diff.usec / 65536) & 255;
   buf[3] = diff.sec & 255;
   buf[4] = (diff.sec / 256) & 255;
   buf[5] = (diff.sec / 65536) & 255;
   buf[6] = (diff.sec / 16777216) & 255;
   buf[7] = r - 1;
   r += 8;
   if (r == 8)
     r = 7;
   n = 0;
   while (n < r)
    {
     w = write(1,buf + n,r - n);
     if (w < 0)
       ; /*XXX*/
     n += w;
    }
  }
 while (r != 7);
 exit(0);
}
