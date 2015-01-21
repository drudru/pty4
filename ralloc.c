/* ralloc.c, ralloc.h: recovering alloc
Daniel J. Bernstein, brnstnd@nyu.edu.
Depends on sod.h.
Requires malloc/free.
8/26/91: Changed exit() to _exit().
8/26/91: Made rallocneverfail() overwrite any previous handler.
7/24/91: Added rallocneverfail().
7/18/91: Baseline. ralloc 1.0, public domain.
No known patent problems.

Lots of library routines allocate space for temporary objects: compiled
regular expressions, for example. They don't destroy the objects between
each call---wouldn't it be a waste to reallocate and recompile those
regular expressions on every single pattern match? But when space gets
tight, you don't want all those temporary objects cluttering the heap.
You've got to deallocate them as soon as possible. Sure, library X might
have some deallocation routines---but if X is hidden below library Y and
separate library A runs out of space, do you expect A to know about X
and call X's routines? Of course not. How can A and X coordinate?

The solution is ralloc. ralloc works just like malloc, except that when
it runs out of memory, it tries to recover space from anyone who's
willing to give a little slack. If f is a deallocation function, you can
call rallocinstall(f), and ralloc(n) will call f() if there aren't n
bytes free. f() should return a non-zero integer if it could free some
memory, 0 if not. Several libraries can rallocinstall their deallocation
routines, and ralloc will cycle between all of them. Make sure that f
actually frees some memory if it returns non-zero---otherwise ralloc()
will loop, trying f again and again and wondering why malloc() never has
enough space. (In a future implementation I might add a loop counter and
have ralloc give up after trying f enough times.)

According to John F. Haugh, ralloc is a Bad Thing, because it inherently
requires static variables, hence can't be put into a ``pure'' shared
library. Face it, John: ralloc() solves a real problem, and if you can't
put it in a shared library, it's not because ralloc() is somehow evil.
It's because your shared libraries aren't good enough.

*/

#include "ralloc.h"
#include "sod.h"
extern char *malloc(); /*XXXX*/
extern void free();

typedef int (*foo)();

SODdecl(funlist,foo);

static funlist funhead = 0;
static funlist funlast = 0; /* last fun to successfully recover */

static int ralloccount = 0;

int rcount()
{
 return ralloccount;
}

void rfree(s)
char *s;
{
 /* This is for completeness, and for another reason: so that you only */
 /* have to modify this file if you want a debugging malloc-free. */
 --ralloccount; /* for instance */
 free(s);
}

static int crit = 0; /* just to be safe */

static int (*neverfail)() = 0;

static void die(n)
unsigned n;
{
 if (neverfail)
   neverfail(n);
 _exit(1); /*XXX*/
}

char *ralloc(n)
unsigned n;
{
 char *t;
 funlist fun;

 if(t = malloc(n))
  {
   ++ralloccount;
   return t;
  }
 if (crit)
   if (neverfail)
     die(n);
   else
     return 0;
 if (!funhead)
   if (neverfail)
     die(n);
   else
     return 0;
 crit = 1;
 fun = (funlast ? SODnext(funlast) : funhead);
 do
  {
   if(!fun)
     fun = funhead;
   if((*SODdata(fun))()) /* XXX: can we make use of args or return code? */
     funlast = fun;
   else
     if(fun == funlast)
      {
       crit = 0;
       if (neverfail)
	 die(n);
       else
         return 0; /* gaack! */
      }
   fun = SODnext(fun);
   t = malloc(n);
  }
 while(!t);
 ++ralloccount;
 crit = 0;
 return t;
}

void rallocneverfail(f)
int (*f)();
{
 neverfail = f; /* possibly overwriting previous handler */
}

#define malloc ralloc

int rallocinstall(f)
int (*f)();
{
 funlist fun;

 fun = SODalloc(funlist,fun,ralloc);
 if(!fun)
   return -1;
 SODdata(fun) = f;
 SODpush(funhead,fun);

 funlast = funhead; /* need to set it to something */

 return 0;
}
