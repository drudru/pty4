#ifndef PTY_TTY_H
#define PTY_TTY_H

#include "config/ttyopts.h"
#include <sys/ioctl.h>
#ifdef TTY_TERMIO
#include <sys/termio.h>
#endif

struct ttywin
 {
#ifdef TTY_WINDOWS
  struct winsize ws;
#endif
  int dummy;
 }
;

struct ttymodes
 {
#ifdef TTY_TERMIO
  struct termio ti;
#else
  int di; long lb;
  struct sgttyb sg; struct tchars tc; struct ltchars lt;
#endif
  struct ttywin wi;
#ifdef TTY_AUXCHARS
  struct auxchars au;
#endif
 }
;

extern int tty_getctrl();
extern int tty_dissoc();
extern int tty_spaceleft();
extern int tty_setexcl();
extern int tctpgrp();

extern int tty_getmodes();
extern int tty_setmodes();
extern int tty_modifymodes();

/* The following don't do any ioctls; they just mangle internal ttymodes. */

extern void tty_copymodes();
extern void tty_win2modes();
extern void tty_modes2win();
extern void tty_zeromode();
extern void tty_mungemodes();
extern void tty_initmodes();

#endif
