Newsgroups: comp.sources.unix
From: brnstnd@nyu.edu (Dan Bernstein)
Subject: v25i127: Generalized interface to pseudo-tty devices, Part01/09
Message-ID: <1992Feb19.220452.29446@PA.dec.com>
Date: Wed, 19 Feb 92 22:04:52 GMT
Approved: vixie@pa.dec.com

Submitted-By: brnstnd@nyu.edu (Dan Bernstein)
Posting-Number: Volume 25, Issue 127
Archive-Name: pty4/part01

[ What can I say?  It slices, it dices, it washes dishes, it walks the dog.
  It "just works", meaning that if you follow the directions you'll get a
  working package without any pulling of hair or gnashing of teeth or other
  standard porting activities.  Here's BLURB, to convince you that it's worth
  unpacking and installing.					--vix ]

pty is meant as the sole interface between pseudo-terminals and the rest
of the system. Rich Salz said of pty 3.0: ``This is the Ginsu knife (it
slices, it dices, it never rusts) that Dan has been talking about in
comp.unix.wizards/internals for some time now. It is a mind-blower.''
But I just couldn't leave well enough alone, so here's pty 4.0, a vastly
improved rewrite of the entire package. A taste of what it has to offer:

* Improved security - pty 3.0 offered tty security ahead of its time---
several months afterwards, Sun released a ``critical'' security patch
with essentially the same security tests. Now pty 4.0 offers proven
security. Although you can install and use the package without
privileges, system administrators can install pty 4.0 so that it
_guarantees_ that nobody else has access to your tty. I'm offering a
cash reward for anyone who can subvert these guarantees.

* Session management - If you run your shell under pty, and the
connection is hung up, you can log in again and reconnect. The session
management model is extremely simple---it has just three primitives---
yet powerful enough to accomplish tricky tasks, such as recording the
output from a process after the process has started. A paper in the
package, ``An introduction to session management,'' leads even novice
users through competent use of session management commands.

* Automatic installation - pty 4.0 comes with a completely automated
configure/compile/install/verify-configuration setup. It will configure
itself properly for most popular systems without human input.

* Modularity - When I say that pty is meant as the sole interface to
pseudo-terminals, I mean it! pty doesn't get in the way of direct,
efficient pseudo-terminal I/O. So you can use it as a component of other
programs which add input line editing, virtual screen support, or other
fancy features. pty handles just one job, and handles it so cleanly that
you'll never have to duplicate pseudo-terminal code in another program.

* Free utilities - pty 4.0 comes with even more useful utilities than
pty 3.0. It includes ten improved clones of standard utilities, notably
a version of ``script'' which makes a proper utmp entry; and thirty new
tools ranging from administrative helpers to ``tscript'', which records
an interactive session _including the timing between characters_. Power
users will appreciate ``nobuf'', which uses pty to transparently turn
off stdio buffering in any program.

* Free libraries - The pty package comes with several of my favorite
libraries: env, fmt, getopt, radixsort, ralloc, scan, sigdfl, sigsched,
sod, timer, username. You can use all of these for your own programs.

* POSIX support - pty 4.0 works without trouble under popular POSIX/BSD
systems, including Ultrix 4.1 and SunOS 4.1.1. All the job control
features have been adapted to work with POSIX job control. pty should
also be included with BSD 4.4.

* Detailed documentation - Let your worries about incomplete program
documentation be over. The pty package includes more than five thousand
lines of documentation: forty quick-reference man pages; papers on
controlling ttys, job control, session management, and user log files;
extensive notes on pty internals and porting issues; and more.

Okay, enough hype. What's pty good for? Once upon a time nethack would,
if you were lucky, produce characters with both a ring of polymorph and
a ring of polymorph control. I wanted to run nethack inside a script
which would keep rerolling characters until it saw that combination.
(Playing by the rules was never my forte.) Unfortunately, nethack didn't
like having its input and output redirected. So I wrote the first
versions of pty. ``pty nethack'' worked just like ``nethack'' but could
be invoked in the middle of a pipe inside a script. As the years went
by, pty became somewhat more powerful and flexible, but its basic
function has always remained the same: to run programs, especially
``interactive'' programs, under a pseudo-tty. Despite this single-minded
attitude, pty has wormed its way into the solutions to dozens of
problems, ranging from buffer control to automating telnet scripts to
making rlogind secure. Pseudo-terminal code seems to spring up
everywhere; pty is your weapon to slash that code to a single line.
Enjoy!

	---Dan Bernstein, brnstnd@nyu.edu

