#include <stdio.h>
#include "ptycomm.h"
#include "config/ptydir.h"
#include "config/ptylongname.h"
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
 char longname[PTYLONGNAMELEN];

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

 if (*argv)
  {
   strncpy(longname,*argv,sizeof(longname));
   longname[sizeof(longname) - 1] = 0;
  }

 if (!ext)
  {
   fprintf(stderr,"%s: fatal: no -s specified, and PTY not set; are we under a session?\n",optprogname);
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
 if (!*argv)
  {
   if (write(fdcomm,"l",1) < 1)
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
   if (!respeq(resp6,"longnm"))
    {
     close(fdcomm);
     fprintf(stderr,"%s: weird: session %s is being quiet\n",optprogname,ext);
     exit(2);
    }
   if (read(fdcomm,longname,sizeof(longname)) < sizeof(longname))
    {
     close(fdcomm);
     fprintf(stderr,"%s: weird: session %s is being evasive\n",optprogname,ext);
     exit(2);
    }
   close(fdcomm);
   printf("session %c%c%s%s\n",ext[0],ext[1],longname[0] ? ": " : "",longname);
  }
 else
  {
   if (write(fdcomm,"L",1) < 1)
    {
     close(fdcomm);
     fprintf(stderr,"%s: weird: session %s refuses to listen\n",optprogname,ext);
     exit(2);
    }
   if (write(fdcomm,longname,sizeof(longname)) < sizeof(longname))
    {
     close(fdcomm);
     fprintf(stderr,"%s: fatal: session %s is deaf\n",optprogname,ext);
     exit(2);
    }
   if (read(fdcomm,resp6,6) < 6)
    {
     close(fdcomm);
     fprintf(stderr,"%s: weird: session %s refuses to respond\n",optprogname,ext);
     exit(2);
    }
   if (!respeq(resp6,"thanks"))
    {
     close(fdcomm);
     fprintf(stderr,"%s: fatal: session %s refuses to change\n",optprogname,ext);
     exit(2);
    }
   close(fdcomm);
  }

 exit(0);
}
