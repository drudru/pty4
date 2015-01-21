/* Public domain. */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#define BIFFBIT 0100

main(argc,argv)
int argc;
char *argv[];
{
 struct stat st;

 if (fstat(0,&st) == -1)
   exit(2); /* XXX */

 if (argc == 1)
   if (st.st_mode & BIFFBIT)
     printf("is y\n");
   else
     printf("is n\n");
 else
   switch(argv[1][0])
    {
     case 'y':
       fchmod(0,(int) (st.st_mode | BIFFBIT));
       break;
     case 'n':
       fchmod(0,(int) (st.st_mode & ~BIFFBIT));
       break;
     default:
       fprintf(stderr,"usage: biff [y] [n]\n");
       exit(2);
    }
 exit(0);
}
