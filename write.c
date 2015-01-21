/* Public domain. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#ifdef BSD
#include <limits.h>
#endif
#include <stdio.h>
#include <strings.h>
#include <utmp.h>
#include <pwd.h>
#include <time.h>
#include "config/utmpfile.h"
#include "fmt.h"
extern unsigned short getuid();
extern char *ttyname();
extern long time();

main(argc,argv)
int argc;
char *argv[];
{ 
 register FILE *fi;
 struct utmp ut;
 char line[9];
 int lines = 0;
 char fntty[30];
 int fd;
 struct stat st;
 char buf[500];
 char outbuf[3000];
 int offset;
 char *username;
 struct passwd *pw;
 char hostname[64];
 char *ttyn;
 long t;
 struct tm *tm;
 char *s;

 if (argc < 2)
  {
   fputs("Usage: write user [ttyname]\n",stderr);
   exit(1);
  }

 if (!(pw = getpwuid((int) getuid())))
  {
   fputs("write: who are you?\n",stderr);
   exit(1);
  }
 username = pw->pw_name;

 gethostname(hostname,sizeof(hostname));
 hostname[sizeof(hostname) - 1] = 0;

 if (!(ttyn = ttyname(2)))
  {
   fputs("write: Can't find your tty\n",stderr);
   exit(1);
  }
 if ((fstat(2,&st) == -1) || !(st.st_mode & 0020))
   fputs("write: warning: your messages are off, recipient will not be able to respond\n",stderr);

 t = time((long *) 0);
 tm = localtime(&t);

 if (fi = fopen(UTMP_FILE,"r"))
   while (fread((char *) &ut,sizeof(ut),1,fi))
     if (!strncmp(ut.ut_name,argv[1],8))
       if ((argc == 2) || (!strncmp(ut.ut_line,argv[2],8)))
	 if (!lines)
	  {
	   fmt_strncpy(line,ut.ut_line,8);
	   line[8] = '\0';
	   lines = 1;
	  }
	 else
	   lines++;
 if (!lines)
  {
   if (argc == 2)
     (void) fprintf(stderr,"write: %s not logged in\n",argv[1]);
   else
     (void) fprintf(stderr,"write: %s not logged in on tty %s\n",
		    argv[1],argv[2]);
   (void) exit(1);
  }
 if (lines > 1)
   (void) fprintf(stderr,
   "write: %s logged in more than once ... writing to %s\n",
   argv[1],line);

 (void) sprintf(fntty,"/dev/%s",line);
 if ((fd = open(fntty,O_WRONLY)) == -1)
  {
   fputs("write: Permission denied\n",stderr);
   exit(1);
  }

 (void) sprintf(buf,"\nMessage from %s@%s on %s at %d:%02d ...%c\n",
		username,hostname,ttyn + 5,tm->tm_hour,tm->tm_min,7);
 (void) write(fd,buf,strlen(buf));

 (void) sprintf(buf,"%s: ",username);
 offset = strlen(buf);

 while (fgets(buf + offset,sizeof(buf) - offset,stdin))
  {
   if ((fstat(fd,&st) == -1) || !(st.st_mode & 0020))
    {
     fprintf(stderr,"write: Permission denied\n");
     exit(1);
    }
   s = outbuf + fmt_nvis(outbuf,buf,strlen(buf));
   write(fd,outbuf,s - outbuf); /*XXX*/
   sleep(1);
  }

 t = time((long *) 0);
 tm = localtime(&t);
 (void) sprintf(buf,"End of message from %s@%s on %s at %d:%02d\n",
		username,hostname,ttyn + 5,tm->tm_hour,tm->tm_min);
 (void) write(fd,buf,strlen(buf));

 exit(0);
}
