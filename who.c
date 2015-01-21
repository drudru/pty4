/* who.c: clone of who program
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on getopt.h, fmt.h.
Requires BSD-style utmp and gethostname.
7/22/91: Baseline. who 2.0, public domain.
No known patent problems.

Documentation in who.1.

This is based on the who.c that came with pty 3.0 but is much cleaner.
*/

#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>
#include <time.h>
extern char *asctime();
extern char *ttyname();
#include "config/utmpfile.h"
#include "getopt.h"
#include "fmt.h"

main(argc,argv)
int argc;
char *argv[];
{
 int opt;
 int flagloggedin;
 int flagwhoami;
 char *ttyn;
 static struct utmp ut;
 static char hostname[120];
 char *fn;
 FILE *fi;
 static char output[500];
 char *t;
 unsigned int len;

 flagloggedin = 2;
 flagwhoami = 0;
 while ((opt = getopt(argc,argv,"lL")) != opteof)
   switch(opt)
    {
     case 'L':
       flagloggedin = 1;
       break;
     case 'l':
       flagloggedin = 0;
       break;
     case '?':
     default:
       exit(1);
    }
 argc -= optind;
 argv += optind;

 if (argc > 1)
  {
   flagwhoami = 1;
   if (!isatty(0))
    {
     fprintf(stderr,"who: fatal: stdin not a tty\n");
     exit(1);
    }
   ttyn = ttyname(0) + 5; /* XXX: /dev/ */
   gethostname(hostname,sizeof(hostname));
  }

 if (flagloggedin == 2)
   flagloggedin = !argc;
 fn = ((argc == 1) ? argv[0] : UTMP_FILE);

 if (!(fi = fopen(fn,"r")))
  {
   t = output;
   t += fmt_strncpy(t,"who: cannot open ",0);
   t += fmt_strncpy(t,fn,(output + sizeof(output)) - t - 3);
   *t = 0;
   perror(output);
   exit(1);
  }

 while (fread((char *) &ut,sizeof(ut),1,fi))
  {
   if (ut.ut_name[0] || !flagloggedin)
     if (!flagwhoami || !strncmp(ut.ut_line,ttyn,8))
      {
       t = output;
       if (flagwhoami)
	{
	 t += fmt_strncpy(t,hostname,0);
	 *t++ = '!';
	}
       len = fmt_strncpy(t,"        ",0);
       t[fmt_strncpy(t,ut.ut_name,len)] = ' '; t += len; *t++ = ' ';
       len = fmt_strncpy(t,"        ",0);
       t[fmt_strncpy(t,ut.ut_line,len)] = ' '; t += len;
       len = fmt_strncpy(t,"            ",0);
       t[fmt_strncpy(t,asctime(localtime(&ut.ut_time)) + 4,len)] = ' ';
       t += len;
       if (ut.ut_host[0])
	{
	 *t++ = '\t';
	 *t++ = '(';
	 t += fmt_strncpy(t,ut.ut_host,16);
	 *t++ = ')';
	}
       *t = 0;
       puts(output);
      }
  }
 exit(0);
}
