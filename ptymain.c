#include <signal.h>
#include "sigsched.h"
#include "getoptquiet.h"
#include "ralloc.h"
#include "env.h"
#include "fmt.h"
#include "config/ptydir.h"
#include "config/ttyopts.h"
#include "config/ptyopts.h"
#include "config/posix.h"
#include "ptyget.h"
#include "ptytty.h"
#include "ptylogs.h"
#include "ptymisc.h"
#include "ptytexts.h"
#include "ptycomm.h"
#include "ptymaster.h"
#include "ptysigler.h"
#include "ptyerr.h"
#include "ptyslave.h"
#include "sesslog.h"
#include "sessconnlog.h"

#define verbose 0, /*XXX*/

/* XXX Exported to other files: */
int flagxutmp = 0;
int flagxwtmp = 0;

/* Private flags: */
static int flagxchown = 0;
static int flagxexcl = 0; /* should be 1, but that'd break write & friends */
static int flagxerrwo = 0; /* should be 1, but that'd break csh & more */
static int flagxrandom = 1;
static int flagxflowctl = 0; /* shouldn't have to exist */
static int flagxonlysecure = 0; /* XXX: which one is right? */

static int flagreading = 1;
static int flagdetached = 0;
static int flagverbose = 1;
static int flagjobctrl = 1;
static int flagttymodes = 1;
static int flagsameerr = 0;
static int flagsession = 0;

static int flagpcbreak = 0; /* -pc, character-at-a-time */
static int flagpnew = 1; /* -pd, new line discipline---traditionally off to start */
static int flagpecho = 1; /* -pe, echo characters */
static int flagpcrmod = 1; /* -pn, munge carriage returns */
static int flagpraw = 0; /* -pr, raw mode */
static int flagpcrt = 1; /* -ps, screen */
static int flagp8bit = 1; /* -p8, 8-bit data path---traditionally off */

static int uid = -1; /* no harm in being safe */
static char *username;
static char *host;
static char *remote;
static char **program;

void setupsessionfiles(fdmty,fdsty,ext) /* XXX: move into master? */
int fdmty;
int fdsty;
char *ext;
{
 struct sesslog sl;
 long t;
 t = now();
 if (utmp_on(ext,username,host,t) == -1)
  {
   warn("warning","cannot write utmp entry");
  }
 if (wtmp_on(ext,username,host,t) == -1)
  {
   warn("warning","cannot write wtmp entry");
   utmp_off(ext,host,t); /* if this fails, too bad */
  }
 sesslog_fill(&sl,ext,username,uid,getpid(),t);
 if (sesslog(&sl) == -1)
  {
   warn("warning","cannot write sesslog entry");
   utmp_off(ext,host,t);
   wtmp_off(ext,host,t);
  }
 /* sessconn will be handled later---remember, we start out disconnected! */
}

static int eachpty(ext)
char *ext;
{
 int fdtty;

 verbose("trying %c%c",ext[0],ext[1]);

 /* Note that there are several situations in which dissociation will fail. */
 /* Fortunately, pty doesn't really care. */
 fdtty = tty_getctrl();
 /* note that we do this whether or not flagdetached */
 if (tty_dissoc(fdtty) == -1)
   warn("warning","cannot dissociate from current tty");
 if (fdtty != -1)
   close(fdtty);
 return 0;
}

/* for sigler if flagttymodes: */
static struct ttymodes tmotty; /* original tty modes */
static struct ttymodes tmottyzero; /* zero tty modes */

void startup(n)
int n;
{
 int pims[2];
 int pimc[2];
 int pid;
 int fdmty;
 int fdsty;
 char ext[2];
 int r1;
 int r2;
 char ch;
 int fdcomm;
 struct ttymodes ptymodes;

 r1 = 0;
 r2 = 0;
 if (flagxrandom)
  {
   r1 = getpid();
   r2 = 37 * getpid() + (int) now;
  }

 if (pipe(pims) == -1)
  {
   warn("fatal","cannot create internal pipe");
   die(DIE_IMPOSSIBLE);
  }
 if (pipe(pimc) == -1)
  {
   warn("fatal","cannot create internal pipe");
   die(DIE_IMPOSSIBLE);
  }

 if (!flagdetached)
  {
   int fdtty;

   if ((fdtty = tty_getctrl()) == -1)
    {
     warn("fatal","cannot find controlling tty; try -d?");
     die(DIE_NOCTTY);
    }

   if (flagreading)
     if (tty_forcefg(fdtty) == -1)
      {
       warn("fatal","cannot force myself into foreground; try -R?");
       die(DIE_SETMODES);
      }
   /* The concept of !flagreading has a major problem: It's unsafe. We may */
   /* end up with someone else's tty modes. */
   if (tty_getmodes(fdtty,&ptymodes) == -1)
    {
     warn("fatal","cannot get modes of original tty");
     die(DIE_GETMODES);
    }

   close(fdtty);
  }
 else
  {
   tty_initmodes(&ptymodes);
   flagpcbreak |= 2; flagpnew |= 2; flagpecho |= 2;
   flagpcrmod |= 2; flagpraw |= 2; flagpcrt |= 2;
   flagp8bit |= 2;
  }

 tty_mungemodes(&ptymodes,
   flagpcbreak,flagpnew,flagpecho,flagpcrmod,flagpraw,flagpcrt,flagp8bit);

 switch(pid = fork())
  {
   case -1:
     warn("fatal","cannot fork master");
     die(DIE_FORK);
   case 0: /* master-slave */
#ifdef POSIX_SILLINESS
     if (setsid() == -1) /* cannot fail---we're not a pgrp leader after fork */
      {
       warn("fatal","cannot setsid");
       die(DIE_IMPOSSIBLE);
      }
#endif
     if (flagxonlysecure == -1)
      {
       if (getfreepty(&fdmty,&fdsty,ext,r1,r2,eachpty,flagxchown,1) == -1)
	{
         warn("fatal","no ptys available");
	 ch = DIE_NOPTYS; write(pims[1],&ch,1); /* XXX */
         die(DIE_NOPTYS);
	}
      }
     else if (getfreepty(&fdmty,&fdsty,ext,r1,r2,eachpty,flagxchown,0) == -1)
      {
       warn("warning","no secure ptys available");
       if ((flagxonlysecure == 1) ||
         (getfreepty(&fdmty,&fdsty,ext,r1,r2,eachpty,flagxchown,1) == -1))
	{
         warn("fatal","no ptys available");
	 ch = DIE_NOPTYS; write(pims[1],&ch,1); /* XXX */
         die(DIE_NOPTYS);
	}
      }
     if (flagverbose > 1)
      {
       char buf[50]; char *t; t = buf;
       t += fmt_strncpy(t,"using pty ",0);
       *t++ = ext[0]; *t++ = ext[1]; *t = 0;
       warn("info",buf);
      }
     switch(pid = fork())
      {
       case -1:
	 warn("fatal","cannot fork slave");
	 ch = DIE_FORK; write(pims[1],&ch,1); /* XXX */
	 die(DIE_FORK);
       case 0: /* slave */
	 signal(SIGTTOU,SIG_DFL); /* XXX: restore to original? */
	 signal(SIGTTIN,SIG_DFL);
	 signal(SIGPIPE,SIG_DFL);
         close(pims[0]);
	 close(pims[1]);
	 close(pimc[1]);
	 close(fdmty);
	 verbose("slave waiting...");
	 if ((read(pimc[0],&ch,1) < 1) || (ch != 'k'))
	  {
	   warn("fatal","slave unable to read success code from master");
	   die(1);
	  }
	 close(pimc[0]);
	 verbose("slave starting...");
	 slave(fdsty,ext,program,flagxerrwo,flagsameerr,uid,flagverbose,flagxexcl);
	 return;
       default: /* master */
	 if (setreuid(geteuid(),geteuid()) == -1)
	  {
	   warn("fatal","master unable to set its uids");
	   ch = DIE_IMPOSSIBLE; write(pims[1],&ch,1); /* XXX */
	   die(DIE_IMPOSSIBLE);
	  }
         signal(SIGHUP,SIG_IGN);
         signal(SIGTSTP,SIG_IGN);
         signal(SIGINT,SIG_IGN);
         signal(SIGQUIT,SIG_IGN);
#ifdef TTY_WINDOWS
         signal(SIGWINCH,SIG_IGN);
#endif
         /* We are now completely isolated from tty and I/O signals. */
	 verbose("master setting modes...");
	 if (tty_setmodes(fdsty,&ptymodes) == -1)
	  {
	   warn("fatal","master unable to set modes of pseudo-tty");
	   ch = DIE_SETMODES; write(pims[1],&ch,1); /* XXX */
	   die(DIE_SETMODES);
	  }
	 if (chdir(PTYDIR) == -1)
	  {
	   warn("fatal","master cannot chdir to pty directory; is it set up correctly?");
	   ch = DIE_PTYDIR; write(pims[1],&ch,1); /* XXX */
	   die(DIE_PTYDIR);
	  }
	 if ((fdcomm = comm_read(ext,uid)) == -1)
	  {
	   warn("fatal","master cannot create socket; is pty dir set up correctly?");
	   ch = DIE_ELSE; write(pims[1],&ch,1); /* XXX */
	   die(DIE_ELSE);
	  }
	 setupsessionfiles(fdmty,fdsty,ext);
	 if (write(pims[1],ext,2) != 2)
	   ; /* wtf? can't break pipe---we haven't closed it! */
         close(pims[0]);
	 close(pims[1]);
	 close(pimc[0]);
	 close(0);
	 close(1);
	 close(2); /* XXX: this leaves master without a good error fd */
	 verbose("master starting...");
	 master(fdcomm,fdmty,fdsty,ext,uid,pid,flagsession,pimc[1],username,flagxflowctl);
	 return;
      }
     break;
   default: /* sigler */
     signal(SIGTTOU,SIG_DFL);
     signal(SIGTTIN,SIG_DFL);
     signal(SIGPIPE,SIG_DFL);
     if (chdir(PTYDIR) == -1)
      {
       warn("fatal","signaller cannot chdir to pty directory; is it set up?");
       die(DIE_PTYDIR);
      }
     close(pims[1]);
     close(pimc[0]);
     close(pimc[1]);
     switch(read(pims[0],ext,2))
      {
       case 1: /* XXX: master has already printed an error */
	 die((int) ext[0]); /* XXX */
       case 2:
	 break;
       default:
         warn("fatal","signaller cannot read success code from master");
         die(DIE_COMM);
	 break;
      }
     if (flagttymodes) /* which implies flagreading && !flagdetached */
      {
       int fdtty;

       if ((fdtty = tty_getctrl()) == -1)
	{
	 warn("fatal","signaller cannot find controlling tty; try -T?");
	 die(DIE_NOCTTY);
	}
       if (tty_getmodes(fdtty,&tmotty) == -1)
	{
	 warn("fatal","signaller cannot get modes of original tty; try -T?");
	 die(DIE_GETMODES);
	}
       tty_copymodes(&tmottyzero,&tmotty);
       tty_zeromode(&tmottyzero);
       if (tty_setmodes(fdtty,&tmottyzero) == -1)
	{
	 tty_setmodes(fdtty,&tmotty); /* worth a try... */
	 warn("fatal","signaller cannot set modes of original tty; try -T?");
	 die(DIE_SETMODES);
	}
       close(fdtty);
      }
     close(pims[0]);
     sigler(ext,uid,pid,flagttymodes,&tmotty,&tmottyzero,flagreading,flagjobctrl,remote);
  }
}

static void outofmem(n)
unsigned int n; /* number of bytes in failing malloc */
{
 static char buf[] = "pty: fatal: out of memory\n";
 /* XXX: if this is from master, we may die silently! */
 bwrite(2,buf,sizeof(buf) - 1); /*XXXX*/
 die(DIE_NOMEM);
}

static void usageerr(why,opt)
int why;
int opt;
{
 static char buf[100];
 char *t; t = buf;
 switch(why)
  {
   case 'a':
     warn("fatal","what program do you want to run?");
     break;
   case 'p':
     t += fmt_strncpy(t,"unrecognized terminal mode option -p",0);
     *t++ = opt; *t = 0;
     warn("fatal",buf);
     break;
   case 'x':
     t += fmt_strncpy(t,"unrecognized security option -x",0);
     *t++ = opt; *t = 0;
     warn("fatal",buf);
     break;
   case 'u':
     t += fmt_strncpy(t,"unrecognized option -",0);
     *t++ = optproblem; *t = 0;
     warn("fatal",buf);
     break;
   case 'o':
     t += fmt_strncpy(t,"option -",0);
     *t++ = optproblem;
     t += fmt_strncpy(t," requires an argument",0);
     *t = 0;
     warn("fatal",buf);
     break;
  }
 info(ptyusage);
 die(DIE_USAGE);
}

main(argc,argv,envp)
int argc;
char *argv[];
char *envp[];
{
 int opt;
 char *s;
 int i;
 char *proto;
 char *protoremote;

/* Stage 1: Initial security checks. */
 if (forceopen(0) || forceopen(1) || forceopen(2) || forceopen(3))
  {
   warn("fatal","cannot set up open descriptors");
   die(DIE_SETUP);
  }

/* Stage 2: Figure out userid and username. */
 uid = getuid();
 /* Preserve LOGNAME or USER if it's accurate. */
 s = env_get("LOGNAME");
 if (!s)
   s = env_get("USER");
 if (s && (username2uid(s,&i) != -1) && (i == uid))
   username = s;
 else
   uid2username(uid,&username);

/* Stage 3: Guess at host and remote for system logs. */
 host = 0;
 remote = 0;

 proto = env_get("PROTO");
 if (proto)
  {
   protoremote = ralloc(strlen(proto) + 10);
   if (protoremote)
    {
     s = protoremote;
     s += fmt_strncpy(s,proto,0);
     fmt_strncpy(s,"REMOTE",0);
     s = env_get(protoremote);
     rfree(protoremote);
     if (s)
      {
       remote = ralloc(strlen(proto) + strlen(s) + 10);
       if (remote)
	{
	 char *t;
	 t = remote;
	 t += fmt_strncpy(t,proto,0);
	 t += fmt_strncpy(t,":",0);
	 fmt_strncpy(t,s,0);
	 if (proto[0] == 'T' && proto[1] == 'C' && proto[2] == 'P' && !proto[3])
	  { /* might as well assign host as well */
	   t = remote;
	   while (*t != '@') ++t; ++t;
	   while (*t != '(') ++t; ++t;
	   host = ralloc(strlen(t) + 4);
	   if (host)
	    {
	     fmt_strncpy(host,t,0);
	     t = host;
	     while (*t != ')') ++t;
	     *t = 0;
	    }
	  }
	}
       else
	 remote = 0;
      }
    }
  }
 if (!host)
   host = "pty4.0";
 if (!remote)
   remote = "(unknown)";

/* Stage 4: Process options. */
 while ((opt = getopt(argc,argv,"ACHUVWqQvdDe3EjJsStTp:x:0rRh:O:")) != opteof)
   switch(opt)
    {
     case 'A': info(ptyauthor); die(DIE_USAGE); break;
     case 'C': info(ptycopyright); die(DIE_USAGE); break;
     case 'H': info(ptyhelp); die(DIE_USAGE); break;
     case 'U': info(ptyusage); die(DIE_USAGE); break;
     case 'V': info(ptyversion); die(DIE_USAGE); break;
     case 'W': info(ptywarranty); die(DIE_USAGE); break;
     case 'h': host = optarg; break;
     case 'O': remote = optarg; break;
     case 'q': flagverbose = 0; break;
     case 'Q': flagverbose = 1; break;
     case 'v': flagverbose = 2; break;
     case 'd': flagdetached = 1; flagjobctrl = 0; flagttymodes = 0; break;
     case 'D': flagdetached = 0; flagjobctrl = 1; flagttymodes = 1; break;
     case 'e': flagsameerr = 2; break;
     case '3': flagsameerr = 1; break;
     case 'E': flagsameerr = 0; break;
     case 'j': flagjobctrl = 1; break;
     case 'J': flagjobctrl = 0; break;
     case 'r': flagreading = 1; break;
     case 'R': flagreading = 0; break;
     case 's': flagsession = 1; flagxutmp = 1; break;
     case 'S': flagsession = 0; flagxutmp = 0; break;
     case 't': flagttymodes = 1; break;
     case 'T': flagttymodes = 0; break;
     case '0': flagsameerr = 2; flagsession = 0; flagttymodes = 0;
	       flagxutmp = 0; /* XXX: also flagxwtmp = 0? */
	       flagpcbreak = 3; flagpraw = 3; flagpecho = 2; flagpnew = 2;
	       flagpcrmod = 2;
	       /* XXXXXX: is this sensible behavior? */
	       break;
     case 'p':
       while (opt = *(optarg++))
	 switch(opt)
	  {
	   case 'c': flagpcbreak = 3; break;
	   case 'C': flagpcbreak = 2; break;
	   case 'd': flagpnew = 3; break;
	   case 'D': flagpnew = 2; break;
	   case 'e': flagpecho = 3; break;
	   case 'E': flagpecho = 2; break;
	   case '7': flagp8bit = 2; break;
	   case '8': flagp8bit = 3; break;
	   case 'n': flagpcrmod = 3; break;
	   case 'N': flagpcrmod = 2; break;
	   case 'r': flagpraw = 3; break;
	   case 'R': flagpraw = 2; break;
	   case 's': flagpcrt = 3; break;
	   case 'S': flagpcrt = 2; break;
	   case '0': flagpcbreak = 3; flagpraw = 3;
		     flagpecho = 2; flagpnew = 2;
		     flagpcrmod = 2;
		     break;
	   default: usageerr('p',opt); break;
	  }
       break;
     case 'x':
       while (opt = *(optarg++))
	 switch(opt)
	  {
	   case 'c': flagxchown = 1; break;
	   case 'C': flagxchown = 0; break;
	   case 'f': flagxflowctl = 1; break;
	   case 'F': flagxflowctl = 0; break;
	   case 'u': flagxutmp = 1; break;
	   case 'U': flagxutmp = 0; break;
	   case 'w': flagxwtmp = 1; break;
	   case 'W': flagxwtmp = 0; break;
	   case 'x': flagxexcl = 1; break;
	   case 'X': flagxexcl = 0; break;
	   case 'e': flagxerrwo = 1; break;
	   case 'E': flagxerrwo = 0; break;
	   case 'r': flagxrandom = 1; break;
	   case 'R': flagxrandom = 0; break;
	   case 's': flagxonlysecure = 1; break;
	   case 'S': flagxonlysecure = 0; break;
	   case 'i': flagxonlysecure = -1; break;
	   default: usageerr('x',opt); break;
	  }
       break;
     case '?':
     default:
       usageerr(argv[optind] ? 'u' : 'o',opt); break;
    }
 argc -= optind;
 argv += optind;

 program = argv;
 if (!*program)
   usageerr('a',opt);

/* Stage 5: Munge options. */

#ifdef MUSTNOT_SESSION
 if (flagsession) { flagsession = 0; warn("info","-s forced off"); }
#endif
#ifdef MUSTNOT_UTMPHOST
 host = "pty4.0";
#endif
#ifdef MUSTNOT_UTMP
 if (flagxutmp) { flagxutmp = 0; warn("info","-xu forced off"); }
#endif
#ifdef MUSTNOT_WTMP
 if (flagxwtmp) { flagxwtmp = 0; warn("info","-xw forced off"); }
#endif
#ifdef MUSTNOT_CHOWN
 if (flagxchown) { flagxchown = 0; warn("info","-xc forced off"); }
#endif

 if (flagsession) flagsameerr = 0;
 if (flagdetached) flagttymodes = 0;
 if (!flagreading) flagttymodes = 0;

 if (!flagsession)
  {
   sesslog_disable();
   sessconnlog_disable();
  }
 if (!flagverbose)
   warn_disable();

/* Stage 6: Set up signals and enter sigsched. */
 rallocneverfail(outofmem);

 ss_addsig(SIGCHLD);
 ss_addsig(SIGHUP);
 ss_addsig(SIGTSTP);
 ss_addsig(SIGINT);
 ss_addsig(SIGQUIT);
#ifdef TTY_WINDOWS
 ss_addsig(SIGWINCH);
#endif
 signal(SIGTTOU,SIG_IGN);
 signal(SIGTTIN,SIG_IGN);
 signal(SIGPIPE,SIG_IGN);

 ss_schedonce(ss_asap(),startup,0);
 ss_exec();
 die(0);
}
