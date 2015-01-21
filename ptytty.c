#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
extern int errno;
#include "config/genericptr.h"
#include "config/ttyopts.h"
#include "ptytty.h"

static int ioc(fd,req,arg) /* non-interruptable ioctl */
int fd;
unsigned long req;
GENERICPTR arg;
{
 int result;
 do
   result = ioctl(fd,req,(char *) arg);
 while ((result == -1) && (errno == EINTR));
 return result;
}

#define IOC(f,req,arg) ioc((fd),(unsigned long) (req),(GENERICPTR) (arg))
#define IOCR(f,req,arg) { if (IOC(f,req,arg) == -1) return -1; }

int tty_getctrl()
{
 int fd;
 int dummy;

#define ISTTY(f) (IOC(f,TIOCGPGRP,&dummy) == 0)

 if ((fd = dup(3)) != -1)
   if (ISTTY(fd)) return fd; else close(fd);
 if ((fd = open("/dev/tty",O_RDWR)) != -1)
   if (ISTTY(fd)) return fd; else close(fd);
 if ((fd = dup(0)) != -1)
   if (ISTTY(fd)) return fd; else close(fd);
 if ((fd = dup(1)) != -1)
   if (ISTTY(fd)) return fd; else close(fd);
 if ((fd = dup(2)) != -1)
   if (ISTTY(fd)) return fd; else close(fd);
 return -1;
}

int tty_spaceleft(fd)
int fd;
{
#ifdef FIONREAD
 long result;
 if (IOC(fd,FIONREAD,&result) == -1)
   return -1;
 if (result > 200)
   return 0;
 return 200 - result;
#else
 errno = EIO;
 return -1; /*XXX*/
#endif
}

int tty_setexcl(fd)
int fd;
{
#ifdef TIOCEXCL
 return IOC(fd,TIOCEXCL,0);
 /* setting exclusive use is a bit unusual but it works */
 /* opening /dev/tty should still be allowed, though */
#else
 errno = EIO;
 return -1;
#endif
}

int tctpgrp(fd,pid)
int fd;
int pid;
{
 int pgrp;
 if ((pgrp = getpgrp(pid)) == -1)
   return -1;
 return IOC(fdtty,TIOCSPGRP,&pgrp);
}

int tty_forcefg(fd)
int fd;
{
 int err;
 signal(SIGTTOU,SIG_DFL);
 /* XXX: what if TTOU is blocked? what if TTIN applies (e.g., under GNU)? */
 err = tctpgrp(fd,getpid()); /* could also use ,0 */
 signal(SIGTTOU,SIG_IGN);
 return err;
}

int tty_dissoc(fd)
int fd;
{
 int fdtty;
 if (fd != -1)
   if (IOC(fd,TIOCNOTTY,0) == 0)
     return 0;
 fdtty = open("/dev/tty",O_RDWR); /* XXX */
 if (fdtty == -1)
   if (errno == EBUSY)
     if (fd != -1)
       return IOC(fd,TIOCNOTTY,0);
     else
       return -1;
   else
     return 0; /* XXX: in other words, if we're already dissociated, ok */
 if (IOC(fdtty,TIOCNOTTY,0) == 0)
  {
   close(fdtty);
   return 0;
  }
 close(fdtty);
 return -1;
}

int tty_getmodes(fd,tmo)
int fd;
struct ttymodes *tmo;
{
#ifdef TTY_TERMIO
 IOCR(fd,TCGETA,&(tmo->ti))
#else
 IOCR(fd,TIOCGETD,&(tmo->di))
 IOCR(fd,TIOCGETP,&(tmo->sg))
 IOCR(fd,TIOCGETC,&(tmo->tc))
 IOCR(fd,TIOCLGET,&(tmo->lb))
 IOCR(fd,TIOCGLTC,&(tmo->lt))
#endif
#ifdef TTY_WINDOWS
 IOCR(fd,TIOCGWINSZ,&(tmo->wi.ws))
#endif
#ifdef TTY_AUXCHARS
 IOCR(fd,TIOCGAUXC,&(tmo->au))
#endif
 return 0;
}

int tty_setmodes(fd,tmo)
int fd;
struct ttymodes *tmo;
{
#ifdef TTY_TERMIO
 IOCR(fd,TCSETA,&(tmo->ti))
#else
 IOCR(fd,TIOCSETD,&(tmo->di))
 IOCR(fd,TIOCSETP,&(tmo->sg))
 IOCR(fd,TIOCSETC,&(tmo->tc))
 IOCR(fd,TIOCLSET,&(tmo->lb))
 IOCR(fd,TIOCSLTC,&(tmo->lt))
#endif
#ifdef TTY_WINDOWS
 IOCR(fd,TIOCSWINSZ,&(tmo->wi.ws))
#endif
#ifdef TTY_AUXCHARS
 IOCR(fd,TIOCSAUXC,&(tmo->au))
#endif
 return 0;
}

int tty_modifymodes(fd,tmonew,tmoold)
int fd;
struct ttymodes *tmonew;
struct ttymodes *tmoold;
{
#ifdef TTY_TERMIO
 IOCR(fd,TCSETA,&(tmonew->ti))
   /* XXX: someone want to flesh this out a bit? */
#else
 if (tmonew->di != tmoold->di)
   IOCR(fd,TIOCSETD,&(tmonew->di))
/* XXX: should make other tests dependent on new discipline? hmmm */
 if ((tmonew->sg.sg_flags != tmoold->sg.sg_flags)
   ||(tmonew->sg.sg_ispeed != tmoold->sg.sg_ispeed)
   ||(tmonew->sg.sg_ospeed != tmoold->sg.sg_ospeed)
   ||(tmonew->sg.sg_erase != tmoold->sg.sg_erase)
   ||(tmonew->sg.sg_kill != tmoold->sg.sg_kill))
   IOCR(fd,TIOCSETP,&(tmonew->sg))
 if ((tmonew->tc.t_intrc != tmoold->tc.t_intrc)
   ||(tmonew->tc.t_quitc != tmoold->tc.t_quitc)
   ||(tmonew->tc.t_startc != tmoold->tc.t_startc)
   ||(tmonew->tc.t_stopc != tmoold->tc.t_stopc)
   ||(tmonew->tc.t_eofc != tmoold->tc.t_eofc)
   ||(tmonew->tc.t_brkc != tmoold->tc.t_brkc))
   IOCR(fd,TIOCSETC,&(tmonew->tc))
 if (tmonew->lb != tmoold->lb)
   IOCR(fd,TIOCLSET,&(tmonew->lb))
 if ((tmonew->lt.t_suspc != tmoold->lt.t_suspc)
   ||(tmonew->lt.t_dsuspc != tmoold->lt.t_dsuspc)
   ||(tmonew->lt.t_rprntc != tmoold->lt.t_rprntc)
   ||(tmonew->lt.t_flushc != tmoold->lt.t_flushc)
   ||(tmonew->lt.t_werasc != tmoold->lt.t_werasc)
   ||(tmonew->lt.t_lnextc != tmoold->lt.t_lnextc))
   IOCR(fd,TIOCSLTC,&(tmonew->lt))
#endif
#ifdef TTY_WINDOWS
 if ((tmonew->wi.ws.ws_xpixel != tmoold->wi.ws.ws_xpixel)
   ||(tmonew->wi.ws.ws_ypixel != tmoold->wi.ws.ws_ypixel)
   ||(tmonew->wi.ws.ws_row != tmoold->wi.ws.ws_row)
   ||(tmonew->wi.ws.ws_col != tmoold->wi.ws.ws_col))
   IOCR(fd,TIOCSWINSZ,&(tmonew->wi.ws))
#endif
#ifdef TTY_AUXCHARS
 if ((tmonew->au.t_usemap != tmoold->au.t_usemap)
   ||(tmonew->au.t_usest != tmoold->au.t_usest))
   IOCR(fd,TIOCSAUXC,&(tmonew->au))
#endif
 return 0;
}

void tty_copymodes(tmonew,tmoold)
struct ttymodes *tmonew;
struct ttymodes *tmoold;
{
 *tmonew = *tmoold; /* XXX: structure copying */
}

void tty_modes2win(tmo,twi)
struct ttymodes *tmo;
struct ttywin *twi;
{
 ;
#ifdef TTY_WINDOWS
 twi->ws.ws_xpixel = tmo->wi.ws.ws_xpixel;
 twi->ws.ws_ypixel = tmo->wi.ws.ws_ypixel;
 twi->ws.ws_row = tmo->wi.ws.ws_row;
 twi->ws.ws_col = tmo->wi.ws.ws_col;
#endif
}

void tty_win2modes(twi,tmo)
struct ttywin *twi;
struct ttymodes *tmo;
{
 ;
#ifdef TTY_WINDOWS
 tmo->wi.ws.ws_xpixel = twi->ws.ws_xpixel;
 tmo->wi.ws.ws_ypixel = twi->ws.ws_ypixel;
 tmo->wi.ws.ws_row = twi->ws.ws_row;
 tmo->wi.ws.ws_col = twi->ws.ws_col;
#endif
}

void tty_zeromode(tmo)
struct ttymodes *tmo;
{
 /* XXXXX: This is supposed to provide a mode in which the tty does */
 /* absolutely no processing. I don't think such a mode truly exists. */
 tty_mungemodes(tmo,3,0,2,0,3,0,3);
}

void tty_mungemodes(tmo,cbreak,new,echo,crmod,raw,crt,p8bit)
struct ttymodes *tmo;
int cbreak;
int new;
int echo;
int crmod;
int raw;
int crt;
int p8bit;
{
#ifdef TTY_TERMIO
 if (crmod >= 2)
   tmo->ti.c_iflag = (tmo->ti.c_iflag & ~ICRNL) | (ICRNL * (crmod == 3));
 if (echo >= 2)
   tmo->ti.c_lflag = (tmo->ti.c_lflag & ~ECHO) | (ECHO * (echo == 3));
 if (cbreak >= 2)
  {
   tmo->ti.c_lflag = (tmo->ti.c_lflag & ~ICANON) | (ICANON * (cbreak == 2));
   if (cbreak == 2) { tmo->ti.c_cc[VEOF] = 4; tmo->ti.c_cc[VEOL] = 0; }
     /* XXX */
   else { tmo->ti.c_cc[VMIN] = 1; tmo->ti.c_cc[VTIME] = 1; }
  }
 if (raw >= 2)
  {
#define NOTIRAW (BRKINT | IGNBRK | IGNPAR | ISTRIP | IXON | IXANY | IXOFF)
   tmo->ti.c_iflag = (tmo->ti.c_oflag & ~NOTIRAW) | (NOTIRAW * (raw == 2));
   tmo->ti.c_oflag = (tmo->ti.c_oflag & ~OPOST) | (OPOST * (raw == 2));
   tmo->ti.c_lflag = (tmo->ti.c_lflag & ~ISIG) | (ISIG * (raw == 2));
  }
 if (crt >= 2)
  {
#define CRTECHO (ECHOE | ECHOCTL | ECHOKE)
   tmo->ti.c_lflag = (tmo->ti.c_lflag & ~CRTECHO) | (CRTECHO * (crt == 3));
  }
 /* XXX: p8bit: c_cflag? */
 /* XXX: new: c_line? */
#else
 if (crmod >= 2)
   tmo->sg.sg_flags = (tmo->sg.sg_flags & ~CRMOD) | (CRMOD * (crmod == 3));
 if (echo >= 2)
   tmo->sg.sg_flags = (tmo->sg.sg_flags & ~ECHO) | (ECHO * (echo == 3));
 if (cbreak >= 2)
   tmo->sg.sg_flags = (tmo->sg.sg_flags & ~CBREAK) | (CBREAK * (cbreak == 3));
 if (raw >= 2)
   tmo->sg.sg_flags = (tmo->sg.sg_flags & ~RAW) | (RAW * (raw == 3));
 if (new >= 2)
   tmo->di = ((new == 3) ? NTTYDISC : OTTYDISC);
 if (p8bit >= 2)
  {
   tmo->lb = (tmo->lb & ~PASS8) | (PASS8 * (p8bit == 3));
   tmo->sg.sg_flags = (tmo->sg.sg_flags & ~(EVENP | ODDP))
     | ((EVENP | ODDP) * (p8bit == 2)); /* have to do this for Suns */
  }
 if (crt >= 2)
   tmo->lb = (tmo->lb & ~(CRTBS | CRTERA | CRTKIL | CTLECH))
                      | ((CRTBS | CRTERA | CRTKIL | CTLECH) * (crt == 3));
#endif
}

void tty_initmodes(tmo)
struct ttymodes *tmo;
{
#ifdef TTY_TERMIO
 /* Here we specify Ye Standard BSD Terminal Settings in termio format. */
 tmo->ti.c_iflag =
   IGNBRK | BRKINT | IGNPAR | ISTRIP | IXON | IXANY | IXOFF;
   /* XXX: IMAXBEL? */
 tmo->ti.c_oflag = OPOST | ONLCR;
 tmo->ti.c_lflag = ISIG | ICANON; /* XXX: | ECHO? */
 tmo->ti.c_cflag = CS7 | PARENB | PARODD;
 /* XXX: XTABS? */
 tmo->ti.c_cc[VINTR] = 3;
 tmo->ti.c_cc[VQUIT] = 28;
 tmo->ti.c_cc[VERASE] = 127;
 tmo->ti.c_cc[VKILL] = 21;
 tmo->ti.c_cc[VEOF] = 4;
 tmo->ti.c_cc[VEOL] = 0;
 tmo->ti.c_cc[VSTART] = 17;
 tmo->ti.c_cc[VSTOP] = 19;
#ifdef VSUSP
 tmo->ti.c_cc[VSUSP] = 26;
#endif
#ifdef VDSUSP
 tmo->ti.c_cc[VDSUSP] = 25;
#endif
#ifdef VREPRINT
 tmo->ti.c_cc[VREPRINT] = 18;
#endif
#ifdef VDISCARD
 tmo->ti.c_cc[VDISCARD] = 15;
#endif
#ifdef VWEASE
 tmo->ti.c_cc[VWERASE] = 23;
#endif
#ifdef VLNEXT
 tmo->ti.c_cc[VLNEXT] = 22;
#endif
#ifdef VSTATUS
 tmo->ti.c_cc[VSTATUS] = 0;
#endif
#ifdef VEOL2
 tmo->ti.c_cc[VEOL2] = 0;
#endif
#else
 /* Here we specify Ye Standard BSD Terminal Settings. */

 tmo->di = OTTYDISC;
 tmo->sg.sg_ispeed = EXTB;
 tmo->sg.sg_ospeed = EXTB;
 tmo->sg.sg_erase = 127; /* del */
 tmo->sg.sg_kill = 21; /* ^U */
 tmo->sg.sg_flags = EVENP | ODDP; /* XXX: | XTABS? | ECHO???? */
 tmo->tc.t_intrc = 3; /* ^C */
 tmo->tc.t_quitc = 28; /* ^\ */
 tmo->tc.t_startc = 17; /* ^Q */
 tmo->tc.t_stopc = 19; /* ^S */
 tmo->tc.t_eofc = 4; /* ^D */
 tmo->tc.t_brkc = -1; /* undef */
 tmo->lb = DECCTQ; /* XXX: | PASS8? contradicts EVENP | ODDP */
 tmo->lt.t_suspc = 26; /* ^Z */
 tmo->lt.t_dsuspc = 25; /* ^Y */
 tmo->lt.t_rprntc = 18; /* ^R */
 tmo->lt.t_flushc = 15; /* ^O */
 tmo->lt.t_werasc = 23; /* ^W */
 tmo->lt.t_lnextc = 22; /* ^V */
#endif
#ifdef TTY_WINDOWS
 tmo->wi.ws.ws_xpixel = 0; /* Or read from TERMCAP? Hmmm */
 tmo->wi.ws.ws_ypixel = 0;
 tmo->wi.ws.ws_row = 0;
 tmo->wi.ws.ws_col = 0;
#endif
#ifdef TTY_AUXCHARS
 tmo->au.t_usest = 20; /* ^T */
 tmo->au.t_usemap = UST_LOAD1 | UST_LOAD5 | UST_LOAD15 | UST_RAWCPU
   | UST_UPTIME | UST_PGRP | UST_CHILDS | UST_PCPU | UST_STATE;
#endif
}
