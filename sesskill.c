#include <stdio.h>
#include "ptycomm.h"
#include "config/ptydir.h"
#include "getopt.h"
#include "env.h"

main(argc,argv)
int argc;
char *argv[];
{
 int opt;
 int uid;
 char *ext;
 int fdcomm;
 char resp6[6];
 int yes;
 int me;

 yes = 0;
 me = 0;

 uid = getuid();
 ext = env_get("PTY");
 while ((opt = getopt(argc,argv,"s:ym")) != opteof)
   switch(opt)
    {
     case 'm':
       me = 1;
       break;
     case 'y':
       yes = 1;
       break;
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
 if (!yes)
  {
   fprintf(stderr,"%s: fatal: -y not specified, so no action taken\n",optprogname);
   exit(2);
  }
 if (!me && env_get("PTY") && (!strcmp(ext,env_get("PTY"))))
  {
   fprintf(stderr,"%s: fatal: that session is us, and -m not specified, so no action taken\n",optprogname);
   exit(2);
  }

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
 if (write(fdcomm,"k",1) < 1)
  {
   close(fdcomm);
   fprintf(stderr,"%s: weird: session %s refuses to listen\n",optprogname,ext);
   exit(2);
  }
 if (read(fdcomm,resp6,6) < 6)
  {
   close(fdcomm);
   fprintf(stderr,"%s: weird: session %s refuses to respond\n",optprogname,ext);
   exit(2);
  }
 close(fdcomm);

 if (!respeq(resp6,"<poof>"))
   ; /* unrecognized reply code */

 exit(0);
}
