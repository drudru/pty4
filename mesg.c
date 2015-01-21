/* Public domain. */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#define MESGBIT 0020

main(argc,argv)
int argc;
char *argv[];
{
 struct stat st;

 if (fstat(0,&st) == -1)
   exit(2); /* XXX */

 if (argc == 1)
   if (st.st_mode & MESGBIT)
     printf("is y\n");
   else
     printf("is n\n");
 else
   switch(argv[1][0])
    {
     case 'y':
       fchmod(0,(int) (st.st_mode | MESGBIT));
       break;
     case 'n':
       fchmod(0,(int) (st.st_mode & ~MESGBIT));
       break;
     default:
       fprintf(stderr,"usage: mesg [y] [n]\n");
       exit(2);
    }
 exit(0);
}
