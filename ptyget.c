#ifdef IRIS_UGH_PTYS
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <stdio.h>
#endif
#include <sys/file.h>
#include <errno.h>
extern int errno;
#include "ptyget.h"
#include "config/ptyext.h"
#include "config/devmty.h"
#include "config/devsty.h"
#include "ptysecure.h"

static int gcd(x,y) /* assumes both nonnegative */
int x; int y;
{
 int t;
 while (x && y) { t = x % y; x = y; y = t; }
 return x ? x : y;
}

int ungetpty(fdmaster,fdslave,ext)
int fdmaster;
int fdslave;
char *ext;
{
#ifdef IRIS_UGH_PTYS
 return 0; /*XXX*/
#endif
 /*XXXX*/
 ptyunsecure(fdmaster,fdslave,ext);
 /* XXX: close fdmaster and fdslave? nah */
}

static char fnmty[sizeof(DEVMTY) + 2] = DEVMTY;
static char fnsty[sizeof(DEVSTY) + 2] = DEVSTY;

static char pty1[] = PTYEXT1;
static char pty2[] = PTYEXT2;
#define pty1len (sizeof(pty1) - 1)
#define pty2len (sizeof(pty2) - 1)

static int bankok[pty1len]; /* must be initialized to 0! */

/* not reentrant */
int getfreepty(fdmaster,fdslave,ext,r1,r2,eachpty,flagxchown,allowinsecure)
int *fdmaster;
int *fdslave;
char *ext;
int r1;
int r2; /* set both r1 and r2 to 0 for the standard searching order */
int (*eachpty)();
int flagxchown;
int allowinsecure;
{
 int start;
 int increment;
 int pos;
 int p1;
 int fdmty;
 int fdsty;

#ifdef IRIS_UGH_PTYS /* XXX: needless to say, deprecated */

char foo[200]; int ptynum; struct stat statmty;
if (eachpty("xx") == -1) return -1;
fdmty = open("/dev/ptc",O_RDWR | O_NDELAY);
if (fdmty == -1) return -1;
if (fstat(fdmty,&statmty) == -1) { close(fdmty); return -1; }
ptynum = minor(statmty.st_rdev);
sprintf(foo,"/dev/ttyq%d",ptynum);
fdsty = open(foo,O_RDWR);
*fdmaster = fdmty; *fdslave = fdsty;
ext[0] = 'a' + ptynum / 26;
ext[1] = 'a' + ptynum % 26;
return 0;

#endif

 /* XXX: Here would be a good spot to include pty limits, say through */
 /* the file PTYDIR/LIMITS. Lines of the form user group num, saying */
 /* that user in that group is limited to num ptys, with * for all. */
 /* All pty use would have to be logged somewhere. Anyway, with a */
 /* streams-based pty, there wouldn't be much point to limits. */

 pos = pty1len * pty2len;
 start = r1 % pos; if (start < 0) start += pos;
 increment = r2 % pos; if (increment < 0) increment += pos;

 while (gcd(increment,pos) != 1)
   ++increment; /* note that this weights some increments more heavily */
 
 fnmty[sizeof(DEVMTY) + 1] = 0;
 fnsty[sizeof(DEVSTY) + 1] = 0;

 pos = start;
 do
  {
   p1 = pos / pty2len;
   fnmty[sizeof(DEVMTY) - 1] = pty1[p1];
   if (!bankok[p1])
    {
     fnmty[sizeof(DEVMTY)] = pty2[0];
     if (access(fnmty,F_OK) == -1)
       bankok[p1] = -1;
     else
       bankok[p1] = 1;
    }
   if (bankok[p1] == 1) /* okay, we know bank exists. */
    {
     fnsty[sizeof(DEVMTY)] = fnmty[sizeof(DEVMTY)] = pty2[pos % pty2len];
     fnsty[sizeof(DEVSTY) - 1] = pty1[p1];
     if (eachpty(fnmty + sizeof(DEVMTY) - 1) == -1)
       return -1;
     do
       fdmty = open(fnmty,O_RDWR);
     while ((fdmty == -1) && (errno == EINTR));
     if (fdmty != -1)
      {
       do
         fdsty = open(fnsty,O_RDWR);
       while ((fdsty == -1) && (errno == EINTR));
       if (fdsty == -1)
	 close(fdmty); /* XXX: warning that slave isn't openable? */
       else
	{
	 setnonblock(fdmty); /*XXX*/
	 ext[0] = pty1[p1];
	 ext[1] = pty2[pos % pty2len];
	 /* Got both sides of the tty open! Now comes the tricky part. */
	 if (ptysecure(&fdmty,&fdsty,ext,fnmty,fnsty,flagxchown,allowinsecure) == -1)
	  {
	   /* XXX: warning of security violation? */
	  }
	 else
	  {
	   unsetnonblock(fdmty); /*XXX*/
	   /* It's ours. */
	   *fdmaster = fdmty;
	   *fdslave = fdsty;
	   return 0;
	  }
	}
      }
    }
   pos = (pos + increment) % (pty1len * pty2len);
  }
 while (pos != start);

 return -1; /* all unopenable? yikes */
}
