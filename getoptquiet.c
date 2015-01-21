/* XXX: this file shouldn't exist */

#include <stdio.h> /* for EOF and stderr---talk about immodularity! */
#include "getoptquiet.h"

int optind = 1;
int optpos = 0;
int opterr = 1;
char *optarg = 0;
int optproblem = 0;
char *optprogname = 0;
int opteof = EOF;

int getopt(argc,argv,opts)
int argc;
char **argv;
char *opts;
{
 int c;
 char *s;

 optarg = 0;
 if (!optprogname)
  {
   optprogname = *argv;
   if (!optprogname) /* oh boy */
     optprogname = ""; /*XXX*/
   for (s = optprogname;*s;++s)
     if (*s == '/')
       optprogname = s + 1;
  }
 if (!argv || (optind >= argc) || !argv[optind])
   return opteof;
 while (optpos && !argv[optind][optpos])
  {
   /* we simply skip blank arguments... not any more */
   ++optind;
   optpos = 0;
   if ((optind >= argc) || !argv[optind])
     return opteof;
  }
 if (!optpos)
  {
   if (argv[optind][0] != '-')
     return opteof;
   ++optpos;
   c = argv[optind][1];
   if ((c == '-') || (c == 0))
    {
     /* XXX: this behavior of "-" is stupid */
     if (c)
       ++optind;
     optpos = 0;
     return opteof;
    }
   /* otherwise c is reassigned below */
  }
 c = argv[optind][optpos];
 ++optpos;
 s = opts;
 while (*s)
  {
   if (c == *s)
    {
     if (s[1] == ':')
      {
       optarg = argv[optind] + optpos;
       ++optind;
       optpos = 0;
       if (!*optarg)
        {
         optarg = argv[optind];
         if ((optind >= argc) || !optarg) /* argument past end */
          {
           optproblem = c;
           return '?';
          }
	 ++optind;
        }
      }
     return c;
    }
   ++s;
   if (*s == ':')
     ++s;
  }
 optproblem = c;
 return '?';
}
