/* tty.c: clone of tty program
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on getopt.h.
Requires ttyname().
7/22/91: Baseline. tty 1.0, public domain.
No known patent problems.

Documentation in tty.1.
*/

#include "getopt.h"

extern char *ttyname(); /* XXX: should have library for this */

main(argc,argv)
int argc;
char *argv[];
{
 char *s;
 int opt;
 int flagsilent;

 flagsilent = 0;
 while ((opt = getopt(argc,argv,"s")) != opteof)
   switch(opt)
    {
     case 's':
       flagsilent = 1;
       break;
     case '?':
     default:
       exit(1);
    }

 s = ttyname(0);

 if (!flagsilent)
   if (s)
     (void) puts(s);
   else
     (void) puts("not a tty");
 exit(!s);
}
