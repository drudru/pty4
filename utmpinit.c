/* derived from contribution from paul graham <pjg@acsu.buffalo.edu> */

#include <sys/types.h>
#include <sys/file.h>
#include <ttyent.h>
#include <utmp.h>
#include "config/utmpfile.h"
#include "ptymisc.h"

main()
{
 struct ttyent *tt;
 int fd;
 static struct utmp ut;

 fd = open(UTMP_FILE,O_WRONLY | O_TRUNC | O_CREAT,0644);
 if (fd == -1)
   exit(1); /*XXX*/

 ut.ut_host[0] = 0;
 ut.ut_name[0] = 0;
 ut.ut_time = now(); /* XXX: init uses 0 */

 write(fd,(char *) &ut,sizeof(ut));
 while (tt = getttyent())
  {
   strncpy(ut.ut_line,tt->ty_name,sizeof(ut.ut_line));
   write(fd,(char *) &ut,sizeof(ut)); /*XXX*/
  }
}
