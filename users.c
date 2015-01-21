/* users.c: clone of users program
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on radixsort.h, sod.h, ralloc.h.
Requires BSD: /etc/utmp, <utmp.h>.
7/23/91: Baseline. users 2.0, public domain.
No known patent problems.

Documentation in users.1.
*/

#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>
#include "config/utmpfile.h"
#include "radixsort.h"
#include "sod.h"
#include "ralloc.h"

struct utmp ut;
SODdecl(namestack,struct { char name[sizeof(ut.ut_name) + 1]; } );

main()
{
 register FILE *fi;
 namestack namehead;
 namestack name;
 int numnames;
 char **base;
 int i;

 numnames = 0;
 namehead = 0;
 if (!(fi = fopen(UTMP_FILE,"r")))
   exit(1); /*XXX*/
 while (fread((char *) &ut,sizeof(ut),1,fi))
   if (ut.ut_name[0])
    {
     name = SODalloc(namestack,name,ralloc);
     if (!name)
       exit(1); /*XXX*/
     strncpy(SODdata(name).name,ut.ut_name,sizeof(ut.ut_name));
     SODpush(namehead,name);
     ++numnames;
    }
 fclose(fi);

 base = RALLOC(char *,numnames);
 if (!base)
   exit(1); /*XXX*/

 i = 0;
 for (name = namehead;name;name = SODnext(name))
   base[i++] = SODdata(name).name;
 
 if (radixsort7(base,numnames,0,(unsigned char *) 0,0,ralloc,rfree) == -1)
   exit(1); /*XXX*/

 i = 0;
 for (;;)
  {
   if (i)
     putchar(' ');
   if (i == numnames)
     break;
   fputs(base[i],stdout);
   ++i;
  }
 putchar('\n');
 exit(0);
}
