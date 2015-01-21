#include <stdio.h>
#include "getopt.h"
#include "sesslog.h"
#include "config/sessfile.h" /* XXX: maybe sesslog should have an iterator? */

main(argc,argv)
int argc;
char *argv[];
{
 FILE *fisf;
 struct sesslog sl;
 int opt;
 char *file;
 int flaglogouts;
 int flagreverse;
 int revnum;

 file = SESSNOW_FILE;
 flaglogouts = 0;
 flagreverse = 0;

 while ((opt = getopt(argc,argv,"rRlLf:")) != opteof)
   switch(opt)
    {
     case 'R':
       flagreverse = 1;
       break;
     case 'r':
       flagreverse = 1;
       break;
     case 'l':
       flaglogouts = 1;
       break;
     case 'L':
       flaglogouts = 0;
       break;
     case 'f':
       file = optarg;
       break;
     case '?':
     default:
       exit(1);
    }

 fisf = fopen(file,"r");
 if (!fisf)
  {
   perror("sesswho: fatal: cannot open current session file");
   exit(2);
  }
 if (flagreverse)
  {
   fseek(fisf,0,2);
   revnum = ftell(fisf) / sizeof(sl);
   fseek(fisf,sizeof(sl) * --revnum,0);
  }

 while (fread(&sl,sizeof(sl),1,fisf) == 1)
  {
   if (sl.ext[0] && (sl.masterpid || flaglogouts))
     /* XXX: requires printf with %* */
     printf(sl.masterpid ? "%c%c  %12.12s  %-*.*s  %d\n"
			 : "%c%c  %12.12s  %-*.*s  exit\n"
       ,sl.ext[0],sl.ext[1]
       ,asctime(localtime(&sl.date)) + 4
       ,SESSLOG_USERLEN
       ,SESSLOG_USERLEN
       ,sl.username
       ,sl.masterpid
      );
   if (flagreverse)
    {
     if (fseek(fisf,-2 * sizeof(sl),1) == -1) /*XXX*/
       break;
     --revnum; /*XXX: why do we care? */
    }
  }
 exit(0);
}
