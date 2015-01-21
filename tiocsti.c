/* Public domain. */
#include <sys/ioctl.h>

main(argc,argv)
int argc;
char *argv[];
{
 int j;
 char *s;

 if (ioctl(3,TIOCGPGRP,(char *) &j) == -1)
   (void) dup2(0,3);

 for (j = 1;j < argc;j++)
  {
   for (s = argv[j];*s;s++)
     (void) ioctl(3,TIOCSTI,s);
   if (j < argc - 1)
     (void) ioctl(3,TIOCSTI," ");
  }
}
