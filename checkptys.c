#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "config/devmty.h"
#include "config/devsty.h"
#include "config/posix.h" /* XXX: why? */
#include "config/ptybin.h"
#include "config/ptydir.h"
#include "config/ptyext.h"
#include "config/ptygroup.h"
#include "config/ptymodes.h"
#include "config/ptyopts.h"
#include "config/sessconnfile.h"
#include "config/sessfile.h"
#include <utmp.h>
#include "config/utmpfile.h"
#include "config/wtmpfile.h"

char ptybin[] = PTYBIN;
char ptydir[] = PTYDIR;
char sessconnnow[] = SESSCONNNOW_FILE;
char sessconnlog[] = SESSCONNLOG_FILE;
char sessnow[] = SESSNOW_FILE;
char sesslog[] = SESSLOG_FILE;
char utmp[] = UTMP_FILE;
char wtmp[] = WTMP_FILE;

char devmty[sizeof(DEVMTY) + 5] = DEVMTY;
char devsty[sizeof(DEVSTY) + 5] = DEVSTY;
char pty1[] = PTYEXT1;
char pty2[] = PTYEXT2;

void aack(s,t)
char *s;
char *t;
{
 printf("aack! %s: %s\n",s,t);
}

void checkbin(s,level)
char *s;
int level; /* 0 script 1 normal 2 setgid 3 setuid */
{
 struct stat st;

 if (stat(s,&st) == -1)
  {
   aack("executable does not exist",s);
   return;
  }
 if (!(st.st_mode & 001))
   aack("program isn't world-executable",s);
 switch(st.st_mode & 06000)
  {
   case 0:
     if (level == 2)
       aack("program should be setgid but isn't",s);
     if (level == 3)
       aack("program should be setuid but isn't",s);
     break;
   case 06000:
   case 04000:
     if (level == 0)
       aack("SHELL SCRIPT IS SETUID! FIX IMMEDIATELY!",s);
     if (level == 1)
       aack("program is SETUID, shouldn't be",s);
     if (level == 2)
       aack("program is SETUID, should be setgid instead",s);
     break;
   case 02000:
     if (level == 0)
       aack("SHELL SCRIPT IS SETGID! FIX IMMEDIATELY!",s);
     if (level == 1)
       aack("program is setgid, shouldn't be",s);
     if (level == 3)
       aack("program is setgid, should be setuid instead",s);
     break;
   default:
     aack("computer doesn't understand binary arithmetic",s);
  }
 if (st.st_mode & 002)
   aack("program is WORLD-writable",s);
 if ((level == 2) && (st.st_gid != PTYGROUP))
   aack("program should be setgid to tty group but isn't",s);
}

void testpath(s)
char *s;
{
 char *t;
 struct stat st;
 char old;

 if (*s != '/')
  {
   aack("path does not start with a slash",s);
   return;
  }
 t = s + 1;
 for (;;)
  {
   if ((*t == '/') || (*t == 0))
    {
     old = *t;
     *t = 0;
     if (stat(*s ? s : "/",&st) == -1)
       aack("cannot stat component of path",*s ? s : "/");
     else
      {
       if (st.st_mode & 022)
	 aack("component directory is WORLD- and group-writable",*s ? s : "/");
       else if (st.st_mode & 002)
	 aack("component directory is WORLD-writable",*s ? s : "/");
       else if (st.st_mode & 020)
	 aack("component directory is group-writable",*s ? s : "/");
      }
     *t = old;
    }
   if (!*t)
     break;
   ++t;
  }
}

void testpty(fnm,fns,unusedptyowner)
char *fnm;
char *fns;
int unusedptyowner;
{
 struct stat stm;
 struct stat sts;
 int fdm;

 if (stat(fnm,&stm) == -1)
   return;
 if (stat(fns,&sts) == -1)
  {
   aack("pty master has no corresponding slave",fnm);
   return;
  }

 fdm = open(fnm,O_RDWR);
 if (stm.st_mode & 0777 != 0666)
  {
   aack("pty master has weird mode",fnm);
  }
 if (fdm == -1)
  {
   printf("pty master %s in use, slave owned by uid %d\n",fnm,sts.st_uid);
   if (sts.st_mode & 006)
     aack("pty slave allows WORLD access",fns);
   if (sts.st_gid != PTYGROUP)
     aack("pty slave group does not match standard tty group",fns);
   if (sts.st_mode & 020)
     printf("pty slave %s messages on\n",fns);
   if (sts.st_mode & 0100)
     printf("pty slave %s biff on\n",fns);
  }
 else
  {
   if (sts.st_uid != unusedptyowner)
     aack("unused pty slave not owned by standard unused tty owner",fns);
   close(fdm);
  }
}

main()
{
 int p1;
 int p2;

 printf("Testing main pty directory %s...\n",ptydir);
 testpath(ptydir);
 printf("Testing utmp file %s...\n",utmp);
 testpath(utmp);
 printf("Testing wtmp file %s...\n",wtmp);
 testpath(wtmp);
 printf("Testing session log file %s...\n",sesslog);
 testpath(sesslog);
 printf("Testing current session file %s...\n",sessnow);
 testpath(sessnow);
 printf("Testing session-connection log file %s...\n",sessconnlog);
 testpath(sessconnlog);
 printf("Testing current session-connection file %s...\n",sessconnnow);
 testpath(sessconnnow);
 printf("Testing pty binary directory %s...\n",ptybin);
 testpath(ptybin);

 for (p1 = 0;pty1[p1];++p1)
   for (p2 = 0;pty2[p2];++p2)
    {
     devmty[sizeof(DEVMTY) - 1] = pty1[p1];
     devmty[sizeof(DEVMTY)] = pty2[p2];
     devsty[sizeof(DEVSTY) - 1] = pty1[p1];
     devsty[sizeof(DEVSTY)] = pty2[p2];
     testpty(devmty,devsty,geteuid());
    }

 printf("Checking for actual pty-related binaries in %s...\n",ptybin);
 if (chdir(ptybin) == -1)
  {
   aack("cannot switch to pty binary directory",ptybin);
  }
 else
  {
   checkbin("argv0",1);
   checkbin("biff",1);
   checkbin("checkptys",1);
   checkbin("condom",0);
   checkbin("ctrlv",1);
   checkbin("disconnect",3);
   checkbin("excloff",1);
   checkbin("exclon",1);
   checkbin("lock",1);
   checkbin("mesg",1);
   checkbin("nobuf",0);
   checkbin("pty",3);
   checkbin("reconnect",3);
   checkbin("script",0);
   checkbin("script.tidy",0);
   checkbin("sess",0);
   checkbin("sesskill",3);
   checkbin("sesslist",3);
   checkbin("sessmenu",1);
   checkbin("sessname",3);
   checkbin("sesswhere",1);
   checkbin("sesswho",1);
   checkbin("tiocsti",1);
   checkbin("tplay",1);
   checkbin("trecord",1);
   checkbin("tscript",0);
   checkbin("tty",1);
   checkbin("ttydetach",1);
   checkbin("ttyprotect",0);
   checkbin("users",1);
   checkbin("utmpinit",1);
   checkbin("waitfor",1);
   checkbin("wall",2);
   checkbin("who",1);
   checkbin("whoami",1);
   checkbin("write",2);
   checkbin("wtmprotate",0);
   checkbin("sessrotate",0);
   checkbin("sclogrotate",0);
   checkbin("sessnowinit",0);
   checkbin("scnowinit",0);
  }

 exit(0);
}
