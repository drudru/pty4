#include <sys/types.h>
#include <sys/file.h>
#include "sesslog.h"
#include "ptymisc.h"
#include "config/sessfile.h"
#include "config/ptyext.h"

static int flagsesslog = 1;

void sesslog_disable()
{
 flagsesslog = 0;
}

int sesslog(sl)
struct sesslog *sl;
{
 int fdnow;
 int fd;
 int x;
 char *s;

 if (!flagsesslog)
   return 0;
 for (x = 0,s = PTYEXT1;*s;++s,x += (sizeof(PTYEXT2) - 1))
   if (*s == sl->ext[0])
    {
     for (s = PTYEXT2;*s;++s,++x)
       if (*s == sl->ext[1])
	{
	 fdnow = open(SESSNOW_FILE,O_WRONLY | O_CREAT,0644);
	 if (fdnow == -1)
	   return -1;
         if (lseek(fdnow,x * (long) sizeof(*sl),L_SET) == -1)
	  {
	   close(fdnow);
	   return -1;
	  }
	 if (write(fdnow,(char *) sl,sizeof(*sl)) < sizeof(*sl))
	  {
	   close(fdnow); /* XXX: but now sessnow is messed up! */
	   return -1;
	  }
	 lflock(fdnow); /* XXX: what if it fails? */
	 fd = open(SESSLOG_FILE,O_WRONLY | O_APPEND | O_CREAT,0644);
	 if (fd == -1)
	  {
	   lfunlock(fdnow);
	   close(fdnow);
	   return -1; /* XXX: but now sessnow is messed up! */
	  }
	 if (write(fd,(char *) sl,sizeof(*sl)) < sizeof(*sl))
	  {
	   lfunlock(fdnow);
	   close(fdnow);
	   close(fd);
	   return -1; /* XXX: but now both logs are messed up! */
	  }
	 close(fd);
	 lfunlock(fdnow);
	 close(fdnow);
	 return 0;
	}
     return -1;
    }
 return -1;
}

void sesslog_fill(sl,ext,username,uid,masterpid,date)
struct sesslog *sl;
char *ext;
char *username;
int uid;
int masterpid;
long date;
{
 sl->ext[0] = ext[0];
 sl->ext[1] = ext[1];
 sl->ext[2] = ext[2]; /* requires SESSLOG_EXTLEN be at least 3 */
 strncpy(sl->username,username,SESSLOG_USERLEN);
 sl->uid = uid;
 sl->masterpid = masterpid;
 sl->date = date;
}
