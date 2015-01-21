#include <stdio.h>
extern char *malloc();

static char crterase[3] = { 8, ' ', 8 };

main(argc,argv,envp)
int argc;
char *argv[];
char *envp[];
{
 int flagecho;
 int flagdisc;
 int flagseveral;
 FILE *fi;
 int ch;
 char okext1;
 char okext2;
 char ext1;
 char ext2;
 int i;
 char cmd[200];
 char **newargv;

 flagdisc = 0;
 flagseveral = 0;
 flagecho = 1;

 fi = popen("sesslist -0","r");
 if (fi)
  {
   while ((ch = getc(fi)) != EOF)
    {
     if (ch == 's')
      {
       while (((ch = getc(fi)) != EOF) && (ch != ' '))
	 ;
       if (ch == EOF) break;
       if ((ch = getc(fi)) == EOF) break; ext1 = ch;
       if ((ch = getc(fi)) == EOF) break; ext2 = ch;
       for (i = 0;i < 5;++i)
	{
	 while (((ch = getc(fi)) != EOF) && (ch != ' '))
	   ;
	 if (ch == EOF) break;
	}
       if (ch == EOF) break;
       if ((ch = getc(fi)) != 'd') goto breakif;

       /* Now ext1 and ext2 give a disconnected session. */
       if (flagdisc)
	 flagseveral = 1;
       else
         flagdisc = 1;
       okext1 = ext1;
       okext2 = ext2;
       printf("You have a disconnected session on /dev/tty%c%c, with these processes:\r\n",ext1,ext2); fflush(stdout);
       sprintf(cmd,"ps augxt%c%c | sed 's/$/\r/'",ext1,ext2);
       system(cmd);
       while (((ch = getc(fi)) != EOF) && (ch != 'd'))
	 ;
       if ((ch = getc(fi)) == ':')
	{
	 printf("Session %c%c is named:",ext1,ext2); fflush(stdout);
	 while (((ch = getc(fi)) != EOF) && ch)
	   putchar(ch);
	 printf(".\r\n"); fflush(stdout);
	}
      }
     breakif: ;
     if (ch == EOF) break;
     if (ch)
       while (((ch = getc(fi)) != EOF) && ch)
         ;
     if (ch == EOF) break;
    }
   pclose(fi); /* XXX: or maybe it's not worth waiting? */
  }

 if (flagdisc)
  {
   char r1;
   char r2;
   int pos;
   char ch;

   if (flagseveral)
     sprintf(cmd,"\
Would you like to reconnect to one of those sessions?\r\n\
If so, type its two-character extension, like %c%c for the last one.\r\n\
To instead start a new session as usual, just press return: ",okext1,okext2);
   else
     sprintf(cmd,"\
Would you like to reconnect to that session?\r\n\
If so, type its two-character extension, %c%c.\r\n\
To instead start a new session as usual, just press return: ",okext1,okext2);
   write(1,cmd,strlen(cmd));

   pos = 0;
   while (read(0,&ch,1) == 1)
    {
     if ((ch == 8) || (ch == 127))
      {
       if (pos)
	{
	 --pos;
         if (flagecho)
           write(1,crterase,3);
	}
       continue;
      }
     if ((ch == 10) || (ch == 13))
      {
       if (flagecho)
	 write(1,"\r\n",2);
       if (pos < 2)
	 break;
       sprintf(cmd,"exec pty -d reconnect %c%c",r1,r2);
       execl("/bin/sh","sh","-c",cmd,(char *) 0);
       /* XXX: warning? */
       exit(1);
      }
     /* XXX: other special chars? */
     switch(pos)
      {
       case 0:
	 r1 = ch;
	 ++pos;
	 if (flagecho)
	   write(1,&ch,1);
	 break;
       case 1:
	 r2 = ch;
	 ++pos;
	 if (flagecho)
	   write(1,&ch,1);
	 break;
       case 2:
	 ch = 7;
	 write(1,&ch,1);
	 break;
      }
    }
  }
 
 newargv = (char **) malloc((argc + 4) * sizeof(char *));
 if (!newargv)
   exit(1); /*XXXX*/

 for (i = 0;i <= argc;++i)
   newargv[i + 2] = argv[i + 1];
 newargv[0] = "pty";
 newargv[1] = "-ds";
 execvp("pty",newargv,envp);
 /* XXX: warning? */
 exit(1);
}
