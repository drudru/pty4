/* lock.c: clone of lock program
Daniel J. Bernstein, brnstnd@nyu.edu.
No dependencies.
Requires curses and signal, i.e., UNIX.
7/27/91: Baseline. lock 2.0, public domain.
No known patent problems.

Documentation in lock.1.

Derived from version of lock included with pty 3.0.
*/

#include <curses.h>
#include <signal.h>

main()
{
 char key[100];
 char key2[100];
 int i;

 for (i = 1;i < 32;++i) /*XXX*/
   signal(i,SIG_IGN);
 savetty();
 crmode();
 noecho();
 printf("Key: "); fflush(stdout);
 if (fgets(key,sizeof(key) - 2,stdin))
  {
   printf("\nAgain: "); fflush(stdout);
   if (fgets(key2,sizeof(key2) - 2,stdin))
     if (!strcmp(key,key2))
      {
       printf("\n"); fflush(stdout);
       while ((fgets(key2,sizeof(key2),stdin) == NULL) || strcmp(key,key2))
	{
	 printf("Bad password!\n");
	 for (i = 0;i < 20;++i)
	   putchar(7);
	 fflush(stdout);
	 sleep(1);
	}
      }
     else printf("\n%c",7);
   else printf("\n%c",7);
  }
 else printf("\n%c",7);
 resetty();
 exit(0);
}
