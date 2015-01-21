/* whoami.c: clone of whoami program
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on username.h.
Requires puts() and geteuid().
7/22/91: Baseline. whoami 1.0, public domain.
No known patent problems.

Documentation in whoami.1.
*/

#include "username.h"

main()
{
 char *username;
 uid2username(geteuid(),&username);
 puts(username);
 exit(0);
}
