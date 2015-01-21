#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <errno.h>
extern int errno;
#include <fcntl.h>
#include "ptymisc.h"
#include "config/fdsettrouble.h"

#ifdef DESPERATE_FD_SET
#undef fd_set
#define fd_set long
#endif

extern long time();
long now()
{
 return time((long *) 0);
}

int gaargh(n) int n;
{
 struct timeval t;

 t.tv_sec = 0;
 t.tv_usec = n;
 return select(0,(fd_set *) 0,(fd_set *) 0,(fd_set *) 0,&t);
}

int setnonblock(fd)
int fd;
{
 return fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0) | FNDELAY); /* XXX */
}

int unsetnonblock(fd)
int fd;
{
 return fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0) & ~FNDELAY); /*XXX*/
}

int forceopen(fd)
int fd;
{
 int fdnull;
 if (fcntl(fd,F_GETFL,0) != -1) /* it's open already */
   return 0;
 fdnull = open("/dev/null",O_RDWR,0);
 if (fdnull == -1)
   fdnull = open("/",O_RDONLY,0);
 if (fdnull == -1)
   return -1;
 if (fdnull != fd)
  {
   if (dup2(fdnull,fd) == -1)
    {
     close(fdnull);
     return -1;
    }
   close(fdnull);
  }
 return 0;
}

int respeq(resp,str)
char *resp;
char *str;
{
 return scan_strncmp(resp,str,6) == 6;
}

int bread(fd,buf,n)
int fd; char *buf; int n;
{
 int r; int tot; tot = 0;
 while (n)
  {
   r = read(fd,buf,n);
   if (r == 0) break;
   if (r == -1)
     if ((errno == EINTR) || (errno == EWOULDBLOCK)) continue;
     else return -1; /* XXX: losing data! */
   buf += r; tot += r; n -= r;
  }
 return tot;
}

int bwrite(fd,buf,n)
int fd; char *buf; int n;
{
 int w; int tot; tot = 0;
 while (n)
  {
   w = write(fd,buf,n);
   if (w == 0) break; /* XXX: can happen under System V [sigh] */
   if (w == -1)
     if ((errno == EINTR) || (errno == EWOULDBLOCK)) continue;
     else return -1; /* XXX: losing data! */
   buf += w; tot += w; n -= w;
  }
 return tot;
}

int lflock(fd)
int fd;
{
 /* must depend only on write access */
 /* does not need to disappear automatically upon close() */
 /* but should disappear automatically upon crash of any sort */
 return flock(fd,LOCK_EX);
}

int lfunlock(fd)
int fd;
{
 return flock(fd,LOCK_UN);
}
