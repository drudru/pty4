#include <stdio.h>

main(argc,argv)
int argc;
char *argv[];
{
 if (argc < 3)
  {
   fputs("Usage: argv0 realname program [ arg ... ]\n",stderr);
   exit(1);
  }
 execvp(argv[1],argv + 2);
 perror("argv0: fatal: cannot execute");
 exit(4);
}
