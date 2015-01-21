#include <stdio.h>
#include "fmt.h"
#include "getopt.h"
#include "sessconnlog.h"
#include "config/sessconnfile.h"

main(argc,argv)
int argc;
char *argv[];
{
 FILE *fisf;
 struct sessconnlog sl;
 int opt;
 char *file;
 int flaglogouts;
 int flagreverse;
 int revnum;

 file = SESSCONNNOW_FILE;
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
   perror("sesswhere: fatal: cannot open current session-connection file");
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
   static char outbuf[SESSCONNLOG_REMOTELEN + 100];
   if (sl.ext[0] && (sl.siglerpid || flaglogouts))
    {
     char *t; t = outbuf;
     *t++ = sl.ext[0]; *t++ = sl.ext[1];
     t += fmt_strncpy(t,"  ",0);
     t += fmt_strncpy(t,asctime(localtime(&sl.date)) + 4,12);
     switch(sl.siglerpid)
      {
       case -1:
	 t += fmt_strncpy(t,"  connect ",0);
	 t += fmt_vis(t,sl.remote,strlen(sl.remote));
	 break;
       case 1:
	 t += fmt_strncpy(t,"  disconnect",0);
	 break;
       case 0:
	 t += fmt_strncpy(t,"  cleanup",0);
	 break;
      }
     *t++ = '\n';
     *t = 0;
     fwrite(outbuf,1,t - outbuf,stdout);
    }
   if (flagreverse)
    {
     if (fseek(fisf,-2 * sizeof(sl),1) == -1) /*XXX*/
       break;
     --revnum; /*XXX: why do we care? */
    }
  }
 exit(0);
}
