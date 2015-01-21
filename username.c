/* username.c, username.h: username-uid conversions
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on fmt.h, scan.h.
Requires getpwuid(), getpwnam().
7/18/91: Rewritten to use fmt/scan. username 1.1, public domain.
5/1/91: Baseline. username 1.0, public domain.
No known patent problems.

XXX: cache

*/

#include <pwd.h>
#include "username.h"
#include "fmt.h"
#include "scan.h"

int uid2username(uid,unp)
int uid;
char **unp;
{
 struct passwd *pw;
 static char un[FMT_ULONG + 1];

 if (pw = getpwuid(uid))
  {
   *unp = pw->pw_name;
   return 0;
  }
 un[fmt_uint(un,uid)] = 0;
 *unp = un;
 return 1;
}

int username2uid(un,uid)
char *un;
int *uid;
{
 struct passwd *pw;

 if (!un[scan_uint(un,uid)])
   return 1;
 pw = getpwnam(un);
 if (!pw)
   return -1; /*XXX*/
 *uid = pw->pw_uid;
 return 0;
}
