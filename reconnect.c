#include <stdio.h>
#include "ptymisc.h"
#include "ptycomm.h"
#include "config/ptydir.h"
#include "getopt.h"
#include "env.h"

char noreco[2] = { 0, 0 };

main(argc,argv)
int argc;
char *argv[];
{
 int opt;
 int uid;
 char *ext;
 char *reco;
 int fdcomm;
 char resp6[6];

 uid = getuid();
 ext = env_get("PTY");
 while ((opt = getopt(argc,argv,"s:")) != opteof)
   switch(opt)
    {
     case 's':
       ext = optarg;
       break;
     case '?':
     default:
       exit(1);
    }
 argc -= optind;
 argv += optind;

 if (!ext)
  {
   fprintf(stderr,"%s: fatal: no -s specified, and PTY not set; are we under a session?\n",optprogname);
   exit(2);
  }

 reco = *argv;
 if (!reco)
   reco = noreco;

 if (chdir(PTYDIR) == -1)
  {
   fprintf(stderr,"%s: fatal: cannot change to session directory %s\n",optprogname,PTYDIR);
   exit(2);
  }

 fdcomm = comm_write(ext,uid);
 if (fdcomm == -1)
  {
   fprintf(stderr,"%s: fatal: cannot find session %s; if it exists, do you own it?\n",optprogname,ext);
   exit(2);
  }
 if (bwrite(fdcomm,"s",1) < 1)
  {
   close(fdcomm);
   fprintf(stderr,"%s: weird: session %s refuses to listen\n",optprogname,ext);
   exit(2);
  }
 if (bwrite(fdcomm,reco,2) < 2)
  {
   close(fdcomm);
   fprintf(stderr,"%s: weird: session %s refuses to listen\n",optprogname,ext);
   exit(2);
  }
 if (bread(fdcomm,resp6,6) < 6)
  {
   close(fdcomm);
   fprintf(stderr,"%s: weird: session %s refuses to respond\n",optprogname,ext);
   exit(2);
  }
 close(fdcomm);

 if (respeq(resp6,"nosglr"))
  {
   fprintf(stderr,"%s: fatal: session %s not connected\n",optprogname,ext);
   exit(2);
  }

 if (!respeq(resp6,"sglrok"))
   ; /* unrecognized reply code */

 fprintf(stderr,"%s: will connect to session %c%c when session %c%c is done\n"
   ,optprogname,reco[0],reco[1],ext[0],ext[1]);
 exit(0);
}
