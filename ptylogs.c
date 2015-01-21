#include <sys/types.h>
#include <sys/file.h>
#include <utmp.h>
#include "fmt.h"
#include "config/utmpfile.h"
#include "config/wtmpfile.h"
#include "config/sysv.h" /*XXX*/
#include "ptymisc.h"
#include "ptylogs.h"
extern int flagxutmp; /*XXX*/
extern int flagxwtmp; /*XXX*/

/* utmp and wtmp make about as much sense as /etc/passwd: not much. */

int utmp_on(ext,name,host,date)
char *ext;
char *name;
char *host;
long date;
{
 struct utmp ut;
 struct utmp xt;
 int fd;
 int i;
 char *t;

 if (!flagxutmp)
   return 0;

 /* XXXX: This uses sequential allocation. See utmpinit. */

 t = ut.ut_line;
 t += fmt_strncpy(t,"tty",0);
 *t++ = ext[0];
 *t++ = ext[1];
 *t = 0;
 strncpy(ut.ut_name,name,sizeof(ut.ut_name));
#ifndef SYSV
 strncpy(ut.ut_host,host,sizeof(ut.ut_host));
#endif
 ut.ut_time = date;

 if ((fd = open(UTMP_FILE,O_RDWR)) == -1)
   return -1;

 i = 0;
 while (bread(fd,(char *) &xt,sizeof(xt)) == sizeof(xt)) /* XXX: should buffer */
  {
   if (!strncmp(xt.ut_line,ut.ut_line,sizeof(ut.ut_line)))
    {
     if (lseek(fd,i * (long) sizeof(xt),L_SET) == -1)
      {
       close(fd);
       return -1;
      }
     i = -1;
     break;
    }
   ++i;
  }
 if (i != -1)
  {
   /* We have to reopen to avoid a race with other end-of-utmp entries. */
   close(fd);
   if ((fd = open(UTMP_FILE,O_RDWR | O_APPEND)) == -1)
     return -1;
  }

 if (bwrite(fd,(char *) &ut,sizeof(ut)) < sizeof(ut))
  {
   close(fd);
   return -1;
  }
 close(fd);
 return 0;
}

int utmp_off(ext,host,date)
char *ext;
char *host;
long date;
{
 utmp_on(ext,"",host,date);
}

int wtmp_on(ext,name,host,date)
char *ext;
char *name;
char *host;
long date;
{
 struct utmp wt;
 int fd;
 char *t;

 if (!flagxwtmp)
   return 0;

 t = wt.ut_line;
 t += fmt_strncpy(t,"tty",0);
 *t++ = ext[0];
 *t++ = ext[1];
 *t = 0;
 strncpy(wt.ut_name,name,sizeof(wt.ut_name));
#ifndef SYSV
 strncpy(wt.ut_host,host,sizeof(wt.ut_host));
#endif
 wt.ut_time = date;

 if ((fd = open(WTMP_FILE,O_WRONLY | O_APPEND)) == -1)
   return -1;
 if (bwrite(fd,(char *) &wt,sizeof(wt)) < sizeof(wt))
  {
   close(fd);
   return -1;
  }
 close(fd);
 return 0;
}

int wtmp_off(ext,host,date)
char *ext;
char *host;
long date;
{
 wtmp_on(ext,"",host,date);
}
