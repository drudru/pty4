#include <sys/types.h>
#include <sys/file.h>
#include "sessconnlog.h"
#include "ptymisc.h"
#include "config/sessconnfile.h"
#include "config/ptyext.h"

static int flagsessconnlog = 1;

void sessconnlog_disable()
{
 flagsessconnlog = 0;
}

int sessconnlog(sl)
struct sessconnlog *sl;
{
 int fdnow;
 int fd;
 int x;
 char *s;

 if (!flagsessconnlog)
   return 0;
 for (x = 0,s = PTYEXT1;*s;++s,x += (sizeof(PTYEXT2) - 1))
   if (*s == sl->ext[0])
    {
     for (s = PTYEXT2;*s;++s,++x)
       if (*s == sl->ext[1])
	{
	 fdnow = open(SESSCONNNOW_FILE,O_WRONLY | O_CREAT,0644);
	 if (fdnow == -1)
	   return -1;
         if (lseek(fdnow,x * (long) sizeof(*sl),L_SET) == -1)
	  {
	   close(fdnow);
	   return -1;
	  }
	 if (write(fdnow,(char *) sl,sizeof(*sl)) < sizeof(*sl))
	  {
	   close(fdnow); /* XXX: but now sessconnnow is messed up! */
	   return -1;
	  }
	 lflock(fdnow);
	 fd = open(SESSCONNLOG_FILE,O_WRONLY | O_APPEND | O_CREAT,0644);
	 if (fd == -1)
	  {
	   lfunlock(fdnow);
	   close(fdnow);
	   return -1; /* XXX: but now sessconnnow is messed up! */
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

void sessconnlog_fill(sl,ext,remote,siglerpid,date)
struct sessconnlog *sl;
char *ext;
char *remote;
int siglerpid;
long date;
{
 sl->ext[0] = ext[0];
 sl->ext[1] = ext[1];
 sl->ext[2] = ext[2]; /* requires SESSCONNLOG_EXTLEN be at least 3 */
 strncpy(sl->remote,remote,SESSCONNLOG_REMOTELEN);
 sl->siglerpid = siglerpid;
 sl->date = date;
}
