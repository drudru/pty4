#include "ptytexts.h"

char *ptyauthor = "\
pty was written by Daniel J. Bernstein.\n\
Internet address: brnstnd@nyu.edu.\n\
" ;

char *ptyversion = "\
pty version 4.0, February 9, 1992.\n\
Copyright (c) 1992, Daniel J. Bernstein.\n\
All rights reserved.\n\
" ;

char *ptycopyright = "\
pty version 4.0, February 9, 1992.\n\
Copyright (c) 1992, Daniel J. Bernstein.\n\
All rights reserved.\n\
\n\
I want this program to be distributed freely in original form.\n\
\n\
Once you've received a legal copy of this program, you can use it.\n\
Forever. Nobody can take that right away from you. You can make changes\n\
and backup copies for your use (or, if you're an organization, for the\n\
use of everyone in the organization). You can distribute patches (though\n\
not patched versions). You'd have all these rights even if I didn't tell\n\
you about them.\n\
\n\
I do grant you further rights, as detailed in the source package. Don't\n\
worry about them unless you're planning to distribute further copies.\n\
\n\
If you have questions about this program or about this notice, or if you\n\
would like additional rights beyond those granted above, or if you have\n\
a patch that you don't mind sharing, please contact me on the Internet\n\
at brnstnd@nyu.edu.\n\
" ;

char *ptywarranty = "\
Daniel J. Bernstein disclaims all warranties to the extent permitted\n\
by applicable law. He is not and shall not be liable for any damages\n\
arising from the use of this program. This disclaimer shall be governed\n\
by the laws of the state of New York.\n\
\n\
In other words, use this program at your own risk.\n\
\n\
If you have questions about this program or about this disclaimer of\n\
warranty, please contact me on the Internet at brnstnd@nyu.edu.\n\
" ;

char *ptyusage = "\
Usage: pty [ -qQve3EdDjJsStTrR0ACHUVW ] [ -h host ] [ -O remote ]\n\
           [ -p[cCdDeEnNrRsS780] ] [ -x[cCeEfFrRsSiuUwWxX] ] program [ arg... ]\n\
Help:  pty -H\n\
" ;

char *ptyhelp = "\
pty runs a program under a pseudo-terminal session.\n\
pty -ACHUVW: print authorship notice, copyright notice, this notice,\n\
             short usage summary, version number, disclaimer of warranty\n\
pty [-qQve3EdDjJsStTrR0] [-p[cCdDeEnNrRsS780]] [-x[cCeEfFrRsSiuUwWxX]] [-hhost]\n\
    [-O remote] program [arg...]: run program under a pseudo-terminal\n\
Options processed l to r. Capitals turn things off. Here + means default.\n\
-q: quiet (nothing on stderr)   -e: leave fds 2 & 3    d=>T  s=>E  R=>T\n\
+Q: normal level of verbosity   -3: leave fd 3 only    d=dJT D=Djt 0=eSTp0\n\
-v: complain about everything   +E: 2 & 3 both->pty    s=sxu S=SxU p0=pcrEND\n\
-d: we are detached    +j: job control    +t: change orig tty to char mode\n\
+D: we have ctrl tty   -J: ignore stops   -T: leave orig tty alone\n\
-s: session (allow disconnect & reconnect)   +r: read input\n\
+S: no session: disconnect will send HUP     -R: output only (like rsh -n)\n\
-h host: use host in utmp/wtmp  -O remote: use remote in sclog\n\
-p[cCdDeEnNrRsS78]: set pty modes; defaults taken from original tty if -D\n\
  c: cbreak, character mode  +n: change return to newline  +e: echo\n\
 +d: new line discipline      r: raw, no keyboard signals  +s: screen, crt\n\
-x[cCeEfFrRsSiuUwWxX]: security/experimental/extended, may be restricted\n\
  c: chown pty  e: pty's stderr wronly  f: FIONREAD flow-control  i: insecure\n\
 +r: random pty  w: /usr/adm/wtmp  u: /etc/utmp  x: TIOCEXCL  s: extra-secure\n\
If you have questions about or suggestions for pty, please feel free\n\
to contact the author, Daniel J. Bernstein, at brnstnd@nyu.edu\n\
on the Internet.\n\
" ;
/* ptyhelp *still* fits. Incredible. :-) */
