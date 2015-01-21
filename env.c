/* env.c, env.h: environ library
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on ralloc.h.
Requires strlen, strncmp, environ.
8/3/91: Fixed off-by-one [sigh] in env_put2().
7/24/91: Added env_put2(). Recoded in terms of ralloc macros.
7/18/91: Cleanups. env 1.1, public domain.
7/10/91: Was ralloc()ing too little; didn't init env right. Tnx CW/EW/HB.
6/29/91: Added env_unsetlen(), made env_add use it so string can be const.
6/28/91: Baseline. env 1.0, public domain.
No known patent problems.

Thanks to Christian Wettergren <d88-cwe@pdc.kth.se>, Erik Wallin
<d87-ewa@pdc.kth.se>, and Harald Barth <d88-hba@tds.kth.se> for bug
fixes.

This was originally meant as a portable version of putenv(). It expanded
to include unsetenv() and getenv(). Note that the routines always
maintain environ properly, so execvp() and friends will pick up the new
variables. I recommend that programs which do a lot of environment
manipulation work with strings and keep their own hash table, then use
these routines to manipulate environ before an execvp().

env_init() does optional initialization. It returns 0 on success, -1 on
failure. Note that env_init(), env_put(), and env_unset() may all change
environ.

env_put("FOO=BAR") adds FOO=BAR to the environment, destroying any
previous value of FOO. It returns 0 on success, -1 on failure. Note that
previous versions of env.c required env_put's argument to be writable;
this problem has been removed.

env_put2("FOO","BAR") is just like env_put("FOO=BAR"), except of course
that in the second case the new environment variable refers to the
string passed to env_put, while in the first case it refers to
internally malloc()ed memory.

env_unset("FOO") unsets any variable FOO. It returns 0 on success, -1 on
failure. It will always succeed if env_init() has previously succeeded.

env_get("FOO") returns the value of the first variable FOO, or 0 if
there is no such variable.

env_pick() returns any FOO=BAR in the environment, or 0 if the
environment is empty. This can be used to implement the BSD printenv
call, or to clear the environment.
*/

#include "env.h"
#include "ralloc.h"

static int init = 0;
static int numenv;
static int allocenv;

extern char *env_get(s)
char *s;
{
 int i;
 int slen;
 char *envi;

 slen = strlen(s);
 for (i = 0;envi = environ[i];++i)
   if ((!strncmp(s,envi,slen)) && (envi[slen] == '='))
     return envi + slen + 1;
 return 0;
}

extern char *env_pick()
{
 return environ[0]; /* environ[numenv-1] would make (pick-unset)^n easier */
}

static void env_unsetlen(s,slen)
char *s;
int slen;
{
 int i;
 for (i = 0;i < numenv;++i)
   if ((!strncmp(s,environ[i],slen)) && (environ[i][slen] == '='))
    {
     if (i < --numenv)
       environ[i] = environ[numenv];
     environ[numenv] = 0;
    }
}

extern int env_unset(s)
char *s;
{
 if (!init)
   if (env_init())
     return -1;
 env_unsetlen(s,strlen(s));
 return 0;
}

static int env_realloc()
{
 char **envp;

 allocenv = numenv + 30;
 envp = environ;
 environ = RALLOC(char *,allocenv + 1);
 if (!environ)
  {
   environ = envp;
   allocenv = numenv;
   return -1;
  }
 numenv = 0;
 while (*envp)
  {
   environ[numenv] = *envp;
   ++numenv;
   ++envp;
  }
 environ[numenv] = 0;
 RFREE(envp - numenv);
 return 0;
}

static int env_add(s)
char *s;
{
 char *t;
 for (t = s;*t;++t)
   if (*t == '=')
    {
     env_unsetlen(s,t - s);
     break;
    }
 if (numenv == allocenv)
   if (env_realloc())
     return -1;
 environ[numenv] = s;
 ++numenv;
 environ[numenv] = 0;
 return 0;
}

int env_init()
{
 char **envp;

 numenv = 0;
 allocenv = 0;
 envp = environ;
 environ = RALLOC(char *,1);
 if (!environ)
  {
   environ = envp;
   return -1;
  }
 environ[0] = 0;
 init = 1;
 while (*envp)
  {
   if (env_add(*envp))
     return -1;
   ++envp;
  }
 return 0;
}

int env_put(s)
char *s;
{
 if (!init)
   if (env_init())
     return -1;
 return env_add(s);
}

int env_put2(s,t)
char *s;
char *t;
{
 char *u;
 int slen;
 slen = strlen(s);
 u = ralloc(slen + strlen(t) + 2);
 if (!u)
   return -1;
 strcpy(u,s);
 u[slen] = '=';
 strcpy(u + slen + 1,t);
 return env_put(u);
}
