#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include "fmt.h"
#include "ptyerr.h"
#include "ptyslave.h"
#include "ralloc.h"

void slave(fdsty,ext,program,flagxerrwo,flagsameerr,uid,flagverbose,flagxexcl)
int fdsty;
char *ext;
char **program;
int flagxerrwo;
int flagsameerr;
int uid;
int flagverbose;
int flagxexcl;
{
 char ptyext[10];
 int fdout;
 int fdtty;
 char *t;

 /* we're already dissociated and reassociated */
 close(0);
 close(1);
 if (flagsameerr < 2)
   close(2);
 if (flagsameerr < 1)
  {
   close(3);
   for (fdout = getdtablesize();fdout > 3;--fdout)
     if (fdout != fdsty)
       close(fdout);
  }
 if (dup(fdsty) != 0)
  {
   warn("fatal","cannot dup slave tty descriptor");
   die(1);
  }
 if (dup(fdsty) != 1)
  {
   warn("fatal","cannot dup slave tty descriptor");
   die(1);
  }
 if (flagsameerr < 2)
   if (dup(fdsty) != 2) /* XXX: what about flagxerrwo? */
    {
     warn("fatal","cannot dup slave tty descriptor");
     die(1);
    }
 if (flagsameerr < 1)
   if (open("/dev/tty",O_RDWR) != 3)
    {
     warn("fatal","cannot open /dev/tty under pseudo-tty");
     die(1);
    }
 close(fdsty);

 if ((fdtty = open("/dev/tty",O_RDWR)) == -1)
  {
   warn("fatal","cannot open /dev/tty second time under pseudo-tty");
   die(1);
  }
 if (flagxexcl)
   if (tty_setexcl(0) == -1)
     warn("warning","cannot set exclusive use on pseudo-tty");
 if (setpgrp(0,getpid()) == -1)
  {
   warn("fatal","cannot setpgrp");
   die(1);
  }
 signal(SIGTTOU,SIG_IGN);
 if (tctpgrp(fdtty,getpid()) == -1)
  {
   warn("warning","cannot set pseudo-tty pgrp");
   /* XXX: this always seems to happen on an IRIS. why? */
  }
 signal(SIGTTOU,SIG_DFL);
 /* pty modes are already set */
 /* pty inode protection, including chmod and chown, is done */
 close(fdtty);

 /* logs, including utmp and wtmp, are already set up */

 /* master's comm file is already set up. end of that race! */

 if (setreuid(uid,uid) == -1)
   /* This syscall shouldn't ever fail, so most programs don't check it. */
   /* But we absolutely refuse to exec while setuid. */
  {
   warn("fatal","cannot setreuid");
   die(1);
  }

 t = ptyext;
 t += fmt_strncpy(ptyext,"PTY=",0);
 *t++ = ext[0];
 *t++ = ext[1];
 *t = 0;
 if (env_put(ptyext) == -1)
  {
   warn("fatal","cannot set up PTY in environment");
   die(1);
  }

 sigsetmask(0); /*XXX: restore exactly? */

 if (flagverbose > 1)
   warn("executing program",program[0]);
 execvp(program[0],program);
  
  {
   char *buf;
   buf = ralloc(strlen(program) + 30);
   if (!buf)
     warn("fatal","cannot exec");
   else
    {
     t = buf;
     t += fmt_strncpy(t,"cannot exec ",0);
     t += fmt_strncpy(t,program[0],0);
     *t = 0;
     warn("fatal",buf);
     rfree(buf);
    }
  }
 die(1);
 /*NOTREACHED*/
}
