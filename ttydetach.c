#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <errno.h>
extern int errno;
#include "env.h"
#include "config/posix.h"

main(argc,argv)
int argc;
char *argv[];
{
 int fd;
 int dummy;
 if (argc < 2)
  {
   fputs("Usage: ttydetach program [ arg ... ]\n",stderr);
   exit(1);
  }
 if (env_unset("PTY") == -1)
  {
   fputs("ttydetach: fatal: out of memory\n",stderr);
   exit(2);
  }
 fd = open("/dev/tty",O_RDWR);
 if (fd == -1)
  {
   if (errno == EBUSY) /* damn! */
    {
     if ((ioctl(0,TIOCNOTTY,0) == -1)
       &&(ioctl(1,TIOCNOTTY,0) == -1)
       &&(ioctl(2,TIOCNOTTY,0) == -1)
       &&(ioctl(3,TIOCNOTTY,0) == -1)
       )
     fputs("ttydetach: warning: unable to detach from tty: exclusive-use set\n",stderr);
       /* but it's not as if we didn't try... */
    }
  }
 else
  {
   if (ioctl(fd,TIOCNOTTY,0) == -1)
     perror("ttydetach: warning: unable to detach from tty");
   close(fd);
  }
 if (ioctl(3,TIOCGPGRP,&dummy) == -1)
   close(3);
#ifdef POSIX_SILLINESS /* XXX: sigh... */
 setsid();
#endif
 execvp(argv[1],argv + 1);
 perror("ttydetach: fatal: cannot execute");
 exit(4);
}
