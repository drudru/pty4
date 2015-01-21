#include "getoptquiet.h"
#include "fmt.h"
#include "ptyerr.h"
#include "ralloc.h"

void die(n)
int n;
{
 _exit(n);
}

static int flagwarning = 1;

void warn_disable()
{
 flagwarning = 0;
}

void warn(level,problem)
char *level;
char *problem;
{
 char *buf;

 if (!flagwarning)
   return;
 if (!optprogname)
   optprogname = "pty";
 buf = ralloc(strlen(optprogname) + strlen(level) + strlen(problem) + 10);
 if (!buf)
  {
   write(2,optprogname,strlen(optprogname));
   write(2,": ",2);
   write(2,level,strlen(level));
   write(2,": ",2);
   write(2,problem,strlen(problem));
   write(2,"\r\n",2);
  }
 else
  {
   char *t; t = buf;
   t += fmt_strncpy(t,optprogname,0);
   t += fmt_strncpy(t,": ",0);
   t += fmt_strncpy(t,level,0);
   t += fmt_strncpy(t,": ",0);
   t += fmt_strncpy(t,problem,0);
   t += fmt_strncpy(t,"\r\n",0);
   *t = 0;
   write(2,buf,strlen(buf));
   rfree(buf);
  }
}

void info(text)
char *text;
{
 write(2,text,strlen(text));
}
