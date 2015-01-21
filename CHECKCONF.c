#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "config/fdsettrouble.h"
#include "config/genericptr.h"
#include "config/devmty.h"
#include "config/devsty.h"
#include "config/posix.h"
#include "config/ptybin.h"
#include "config/ptydir.h"
#include "config/ptyext.h"
#include "config/ptygroup.h"
#include "config/ptylongname.h"
#include "config/ptymodes.h"
#include "config/ptyopts.h"
#include "config/ttyopts.h"
#include "config/sessconnfile.h"
#include "config/sessfile.h"
#include <utmp.h>
#include "config/utmpfile.h"
#include "config/wtmpfile.h"
#include <grp.h>

main()
{
 int nothing;
 struct group *grp;
 printf("Check through this list carefully.\n");
 printf("master tty extension: %s\n",DEVMTY);
 printf("slave tty extension: %s\n",DEVSTY);
 printf("pty names: %s[%s][%s]\n",DEVMTY,PTYEXT1,PTYEXT2);
 printf("pty binary directory: %s\n",PTYBIN);
 printf("pty session directory: %s\n",PTYDIR);
 printf("utmp file: %s  wtmp file: %s\n",UTMP_FILE,WTMP_FILE);
 printf("session-connection now: %s  log: %s\n",SESSCONNNOW_FILE,SESSCONNLOG_FILE);
 printf("session now: %s  log: %s\n",SESSNOW_FILE,SESSLOG_FILE);
#ifdef DESPERATE_FD_SET
 printf("DESPERATE_FD_SET turned on.\n");
#else
#ifdef LACKING_FD_ZERO
 printf("LACKING_FD_ZERO turned on.\n");
#else
 printf("System must have normal fd_set and FD_ZERO support.\n");
#endif
#endif
 /* XXX: GENERICPTR? */
#ifdef POSIX_SILLINESS
 printf("POSIX turned on. System must have setsid().\n");
#else
 printf("POSIX turned off. System should not have setsid().\n");
#endif
 printf("pty group: %d. ",PTYGROUP);
 grp = getgrnam("tty");
 if (!grp)
   printf("\nAack! You should add a tty group, group %d, to /etc/group.\n",PTYGROUP);
 else
   if (grp->gr_gid == PTYGROUP)
     printf("Okay, this matches the tty entry in /etc/group.\n");
   else
     printf("\nAack! This doesn't match the tty group entry (%d) in /etc/group.\nYou should probably edit config/ptygroup.h.\n",grp->gr_gid);
 printf("session long name length: %d\n",PTYLONGNAMELEN);
 printf("pty modes: %o used %o unused\n",PTYMODE_USED,PTYMODE_UNUSED);
 printf("MUSTNOT: "); nothing = 1;
#ifdef PTY_MUSTNOT_SESSION
 printf("session "); nothing = 0;
#endif
#ifdef PTY_MUSTNOT_UTMPHOST
 printf("utmphost "); nothing = 0;
#endif
#ifdef PTY_MUSTNOT_UTMP
 printf("utmp "); nothing = 0;
#endif
#ifdef PTY_MUSTNOT_WTMP
 printf("wtmp "); nothing = 0;
#endif
#ifdef PTY_MUSTNOT_CHOWN
 printf("chown "); nothing = 0;
#endif
 if (nothing) printf("(nothing)");
 printf("\n");
#ifdef TTY_AUXCHARS
 printf("System must support tty auxiliary characters.\n");
#endif
#ifdef TTY_WINDOWS
 printf("System must support tty windows and SIGWINCH.\n");
#endif
 exit(0);
}
