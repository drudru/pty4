/* Public domain. */

#include <sys/types.h>
#include <sys/file.h>
#ifdef BSD
#include <limits.h>
#endif
#include <stdio.h>
#include <strings.h>
#include <utmp.h>
#include <pwd.h>
#include <time.h>
#include <ctype.h>
#include "fmt.h"
#include "config/utmpfile.h"
extern unsigned short getuid();
extern char *ttyname();
extern long time();

main()
{ 
 register FILE *fi;
 struct utmp ut;
 char fntty[30];
 int fd;
 char buf[10000];
 char *username;
 struct passwd *pw;
 char hostname[64];
 char *ttyn;
 long t;
 struct tm *tm;
 int r;
 int pos;

 if (!(pw = getpwuid((int) getuid())))
  {
   fprintf(stderr,"write: who are you?\n");
   exit(1);
  }
 username = pw->pw_name;

 gethostname(hostname,sizeof(hostname));

 if (!(ttyn = ttyname(2)))
  {
   fprintf(stderr,"wall: Can't find your tty\n");
   exit(1);
  }

 t = time((long *) 0);
 tm = localtime(&t);

 sprintf(buf,"\nBroadcast message from %s@%s on %s at %d:%02d ...%c\n\n",
		username,hostname,ttyn + 5,tm->tm_hour,tm->tm_min,7);
 pos = strlen(buf);
  {
   static char pre[] =
"End your message with the EOF character. This is how it will show up:";
   write(1,pre,strlen(pre));
  }
 write(1,buf,pos);
 while ((pos < 10000) && ((r = read(0,buf + pos,10000 - pos)) > 0))
   pos += r;
 if (fmt_nvis(FMT_LEN,buf,pos) != pos + 1) /* +1 for extra beep */
  {
   fprintf(stderr,"wall: sorry, that message has control characters\n");
   exit(1);
  }

 if (fi = fopen(UTMP_FILE,"r"))
   while (fread((char *) &ut,sizeof(ut),1,fi))
     if (ut.ut_name[0])
      {
       sprintf(fntty,"/dev/%.8s",ut.ut_line);
       if ((fd = open(fntty,O_WRONLY)) == -1)
         fprintf(stderr,"wall: cannot write to %.8s\n",ut.ut_line);
       else
        {
	 write(fd,buf,pos);
         close(fd);
        }
      }

 exit(0);
}
