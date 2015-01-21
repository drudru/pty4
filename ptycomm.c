#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <errno.h>
extern int errno;
#include "fmt.h"
#include "config/ptyext.h"
#include "ptycomm.h"
#include "ptymisc.h"

/*
Under most versions of SunOS and Ultrix, as well as lots of other systems,
file descriptor passing doesn't work. The most important problem is that
in some situations recvmsg() may return 0 without actually waiting to
receive the file descriptors; in other words, msg_accrightslen may be
sporadically 0. The workaround here is to pause very briefly (1/100 of a
second) and try the recvmsg() again, up to 100 times. I'd prefer seeing
the bug fixed.
*/

#define ESTUPID EINPROGRESS /* [sigh] */

static int comm_path(s,ext,uid)
char *s;
char *ext;
int uid;
{
 int i;
 char *t;

/* as a last layer of protection we make sure that we only create */
/* valid filenames... */
 for (i = 0;PTYEXT1[i];++i)
   if (ext[0] == PTYEXT1[i])
     break;
 if (!PTYEXT1[i])
   return -1;
 for (i = 0;PTYEXT2[i];++i)
   if (ext[1] == PTYEXT2[i])
     break;
 if (!PTYEXT2[i])
   return -1;
 t = s;
 t += fmt_strncpy(t,"comm.",0);
 t += fmt_uint(t,uid);
 *t++ = '.';
 *t++ = ext[0];
 *t++ = ext[1];
 *t = 0;
 return 0;
}

int comm_read(ext,uid)
char *ext;
int uid;
{
 int s;
 struct sockaddr_un sa;

 if ((s = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
   return -1;
 sa.sun_family = AF_UNIX;
 if (comm_path(sa.sun_path,ext,uid) == -1)
   return -1;
 unlink(sa.sun_path);
 if (bind(s,(struct sockaddr *) &sa,strlen(sa.sun_path) + 2) == -1)
   return -1;
 if (listen(s,5) == -1)
   return -1;
 return s;
}

int comm_accept(fd)
int fd;
{
 struct sockaddr_un sa;
 int salen;
 salen = sizeof(sa);
 return accept(fd,(struct sockaddr *) &sa,&salen);
}

int comm_unlink(ext,uid)
char *ext;
int uid;
{
 struct sockaddr_un sa;
 if (comm_path(sa.sun_path,ext,uid) == -1)
   return -1;
 return unlink(sa.sun_path);
}

int comm_write(ext,uid)
char *ext;
int uid;
{
 int s;
 struct sockaddr_un sa;

 if ((s = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
   return -1;
 sa.sun_family = AF_UNIX;
 if (comm_path(sa.sun_path,ext,uid) == -1)
   return -1;
 if (connect(s,(struct sockaddr *) &sa,strlen(sa.sun_path) + 2) == -1)
   return -1;
 return s;
}

int csp(fdcomm,fd)
int fdcomm;
int fd;
{
 struct msghdr msg[2];
 int acc[5];
 struct iovec i[2];

 msg[0].msg_name = 0;
 msg[0].msg_namelen = 0;
 msg[0].msg_iov = i; /* grrrr */
 msg[0].msg_iovlen = 0;
 msg[0].msg_accrights = (caddr_t) acc;
 msg[0].msg_accrightslen = sizeof(int);

 acc[0] = fd;

 return sendmsg(fdcomm,msg,0);
}

int comm_putfd(fdcomm,fd)
int fdcomm;
int fd;
{
 int tries;
 for (tries = 0;tries < 100;++tries)
   if (csp(fdcomm,fd) == 0)
     return 0;
 return -1;
}

static int cgf(fdcomm)
int fdcomm;
{
 struct msghdr msg[2];
 int acc[5];
 struct iovec i[2];
 int r;

 msg[0].msg_name = 0;
 msg[0].msg_namelen = 0;
 msg[0].msg_iov = i; /* grrrr */
 msg[0].msg_iovlen = 0;
 msg[0].msg_accrights = (caddr_t) acc;
 msg[0].msg_accrightslen = sizeof(int);

 do
   r = recvmsg(fdcomm,msg,0);
 while ((r == -1) && ((errno == EINTR) || (errno == EWOULDBLOCK)));
 if (r == -1)
   return -1;
 if (msg[0].msg_accrightslen != sizeof(int))
  {
   errno = ESTUPID; /* it didn't arrive. */
   return -1;
  }
 return acc[0];
}

int comm_getfd(fdcomm)
int fdcomm;
{
 int fd;
 int tries;

 for (tries = 0;tries < 100;++tries)
  {
   fd = cgf(fdcomm);
   if (fd != -1)
     return fd;
   gaargh(10000);
  }
 return -1;
}
