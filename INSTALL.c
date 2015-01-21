#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "config/ptybin.h"
#include "config/ptydir.h"
#include "config/ptygroup.h"
#include "config/sessconnfile.h"
#include "config/sessfile.h"
#include <utmp.h>
#include "config/utmpfile.h"
#include "config/wtmpfile.h"
#include "config/ptygroup.h"

char ptybin[] = PTYBIN;
char ptydir[] = PTYDIR;
char sessconnnow[] = SESSCONNNOW_FILE;
char sessconnlog[] = SESSCONNLOG_FILE;
char sessnow[] = SESSNOW_FILE;
char sesslog[] = SESSLOG_FILE;
char utmp[] = UTMP_FILE;
char wtmp[] = WTMP_FILE;

static int num = 0;

void section(s)
char *s;
{
 ++num;
 printf("\n%d. %s.\n",num,s);
}

int dontskip(s,t,u)
char *s;
char *t;
char *u;
{
 char buf[100];
 char format[200];
 sprintf(format,"! %s: ",s);
 printf(format,t,u);
 if (fgets(buf,sizeof(buf),stdin) == 0)
   return 0;
 if (buf[0] == 'o')
  {
   puts("Okay.");
   return 1;
  }
 if (buf[0] == 's')
  {
   puts("Skipped.");
   return 0;
  }
 return 1;
}

copyf2d(fn,dirfn)
char *fn;
char *dirfn;
{
 int fdold;
 int fdnew;
 int r;
 int n;
 int w;
 char buf[16384];

 fdold = open(fn,O_RDONLY);
 if (fdold == -1)
   return -1;
 fdnew = open(dirfn,O_WRONLY | O_CREAT | O_TRUNC,0600);
 if (fdnew == -1)
  { close(fdold); return -1; }
 while ((r = read(fdold,buf,sizeof(buf))) > 0)
  {
   n = 0;
   while (n < r)
    {
     w = write(fdnew,buf + n,r - n);
     if (w == -1)
      {
       close(fdold); close(fdnew); return -1;
      }
     n += w;
    }
  }
 close(fdnew);
 close(fdold);
 if (r == -1)
   return -1;
 return 0;
}

static char *ptyuname = "pty";

chownpty(fn)
char *fn;
{
 struct passwd *own;
 own = getpwnam(ptyuname);
 if (!own)
   return -1;
 return chown(fn,own->pw_uid,-1);
}

chgrptty(fn)
char *fn;
{
 struct group *grp;
 grp = getgrnam("tty");
 if (!grp)
   return -1;
 return chown(fn,-1,grp->gr_gid);
}

touch(fn)
char *fn;
{
 int fd;
 fd = open(fn,O_WRONLY | O_CREAT,0644);
 if (fd == -1)
   return -1;
 close(fd);
 if (chmod(fn,0644) == -1)
   return -1;
 if (chownpty(fn) == -1)
   return -1;
 return 0;
}

static char CHOPTYSS[100] = "chown pty %s/%s";

dobin(fn,level)
char *fn;
int level;
{
 char dirfn[sizeof(ptybin) + 50];
 sprintf(dirfn,"%s/%s",ptybin,fn);
 if (dontskip("cp %s %s",fn,ptybin))
   if (copyf2d(fn,dirfn) == -1)
     perror("copy failed");
 switch(level)
  {
   case 0:
     if (dontskip("chmod 755 %s/%s",ptybin,fn))
       if (chmod(dirfn,0755) == -1)
	 perror("chmod: cannot change mode");
     break;
   case 1:
     if (dontskip("chmod 755 %s/%s",ptybin,fn))
       if (chmod(dirfn,0755) == -1)
	 perror("chmod: cannot change mode");
     break;
   case 2:
     if (dontskip("chgrp tty %s/%s",ptybin,fn))
       if (chgrptty(dirfn) == -1)
	 perror("chgrp: cannot change group");
     if (dontskip("chmod 2755 %s/%s",ptybin,fn))
       if (chmod(dirfn,02755) == -1)
	 perror("chmod: cannot change mode");
     break;
   case 3:
     if (dontskip(CHOPTYSS,ptybin,fn))
       if (chownpty(dirfn) == -1)
	 perror("chown: cannot change owner");
     if (dontskip("chmod 4755 %s/%s",ptybin,fn))
       if (chmod(dirfn,04755) == -1)
	 perror("chmod: cannot change mode");
     break;
  }
}

main(argc,argv)
int argc;
char *argv[];
{
 if (argv[1])
  {
   ptyuname = argv[1];
   sprintf(CHOPTYSS,"chown %s %%s/%%s",ptyuname);
  }
 printf("I assume you've already set up a %s user and a tty (%d) group.\n\n",ptyuname,PTYGROUP);
 printf("Each action will be printed before it is run. Press return to proceed.\n");
 printf("Type skip (or anything beginning with an s) to skip a step.\n");

 section("Make pty session directory");
 if (dontskip("mkdir %s",ptydir,""))
   if (mkdir(ptydir,0700) == -1)
     perror("mkdir: cannot create directory");
 if (dontskip("chown %s %s",ptyuname,ptydir))
   if (chownpty(ptydir) == -1)
     perror("chown: cannot change owner");

 section("Make pty binary directory");
 if (dontskip("mkdir %s",ptybin,""))
   if (mkdir(ptybin,0700) == -1)
     perror("mkdir: cannot create directory");
 if (dontskip("chmod 755 %s",ptybin,""))
   if (chmod(ptybin,0755) == -1)
     perror("chmod: cannot change mode");

 section("Make session and session-connection log files");
 if (dontskip("touch %s",sessnow,""))
   if (touch(sessnow) == -1)
     perror("touch: cannot touch file");
 if (dontskip("touch %s",sesslog,""))
   if (touch(sesslog) == -1)
     perror("touch: cannot touch file");
 if (dontskip("touch %s",sessconnnow,""))
   if (touch(sessconnnow) == -1)
     perror("touch: cannot touch file");
 if (dontskip("touch %s",sessconnlog,""))
   if (touch(sessconnlog) == -1)
     perror("touch: cannot touch file");
 
 section("Make utmp and wtmp files (note: utmp will be owned by pty)");
 if (dontskip("touch %s",utmp,""))
   if (touch(utmp) == -1)
     perror("touch: cannot touch file");
 if (dontskip("touch %s",wtmp,""))
   if (touch(wtmp) == -1)
     perror("touch: cannot touch file");

 section("Copy executables into pty binary directory");
 dobin("argv0",1);
 dobin("biff",1);
 dobin("checkptys",1);
 dobin("condom",0);
 dobin("ctrlv",1);
 dobin("disconnect",3);
 dobin("excloff",1);
 dobin("exclon",1);
 dobin("lock",1);
 dobin("mesg",1);
 dobin("nobuf",0);
 dobin("pty",3);
 dobin("reconnect",3);
 dobin("script",0);
 dobin("script.tidy",0);
 dobin("sess",0);
 dobin("sesskill",3);
 dobin("sesslist",3);
 dobin("sessmenu",1);
 dobin("sessname",3);
 dobin("sesswhere",1);
 dobin("sesswho",1);
 dobin("tiocsti",1);
 dobin("tplay",1);
 dobin("trecord",1);
 dobin("tscript",0);
 dobin("tty",1);
 dobin("ttydetach",1);
 dobin("ttyprotect",0);
 dobin("users",1);
 dobin("utmpinit",1);
 dobin("waitfor",1);
 dobin("wall",2);
 dobin("who",1);
 dobin("whoami",1);
 dobin("write",2);
 dobin("wtmprotate",0);
 dobin("sessrotate",0);
 dobin("sclogrotate",0);
 dobin("sessnowinit",0);
 dobin("scnowinit",0);

 section("Add log file rotations to daily, weekly, or monthly cron scripts");
 printf("I'll leave this to you.\n");
 printf("You may want to invoke wtmprotate, sessrotate, or sclogrotate.\n");

 section("Add utmp/sessnow/scnow initializations to /etc/rc.local");
 printf("I'll leave this to you.\n");
 printf("You may want to invoke utmpinit, sessnowinit, or scnowinit.\n");

 exit(0);
}
