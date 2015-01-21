#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
extern int errno;
#include "ptysecure.h"
#include "ptytty.h"
#include "config/ptymodes.h"
#include "config/ptygroup.h"

static struct stat storig;

/* must close fdm and fds upon -1 return */
int ptysecure(fdm,fds,ext,fnm,fns,flagxchown,allowinsecure)
int *fdm; /* master */
int *fds; /* slave */
char *ext; /* char array, not string */
char *fnm;
char *fns;
int flagxchown;
int allowinsecure;
{
/* XXX: check that the pathnames are secure? nah */
/* XXX: vhangup(): don't make me laugh */
/* XXX: revoke() under 4.4 */
/* XXX: ofiles... fstat... pff... */
/* XXX: opencount()---but what about unprivileged users? */

/* We have the master and slave open. */
/* Any number of other processes may have the slave open. */
/* Any number of other processes may have our pgrp. */

/* We depend on never passing the master side to another process. */
/* We depend on opens of the master side being mutually exclusive. */
/* We opened the master, so no other process has the master open. */
/* Through this routine we will never close the master. */
/* Hence no other process will have the master open while we work. */

 char buf[20];
 int fdp[2];
 char psarg1[20];
 int kidpid;
 int numpsrets;
 int pspid;
 int flagchmodworked;
 int parsingpid;
 char *(psargs[3]);
 int r;

 if (fstat(*fds,&storig) == -1)
   goto death;
 flagchmodworked = 1;
 if (storig.st_uid != geteuid())
   flagchmodworked = 0;
 if (!allowinsecure && !flagchmodworked)
   goto death;
 if (fchmod(*fds,0600) == -1)
  {
   flagchmodworked = 0;
   /* XXX: warning? other action? */
  }
 if (!allowinsecure && !flagchmodworked)
   goto pdeath;
/* All security guarantees are off unless the slave tty is now */
/* protected from open() by normal users. In other words, we require */
/* either (1) being the owner of the tty, so that the fchmod worked, */
/* or (2) being privileged and having the slave ttys be protected */
/* all the time anyway. (2) is a better situation. (1) is more */
/* realistic for pty installations under current systems. Anyway, */
/* we don't do the more powerful security tests if we can't chmod. */

/* Anyway, we depend on the slave tty now being unopenable by users. */
/* This situation will persist until we change the mode again. */
/* (In situation (2) it will always be true.) */

/* We depend on read() of the master side returning 0 or -1/EIO if */
/* nobody has the slave side open, and something else otherwise. */
 close(*fds); r = read(*fdm,buf,1);
 if (r > 0) { /* XXX: warning? */ goto pmdeath; }
 if ((r == -1) && (errno != EIO) && (errno != EWOULDBLOCK))
  { /* XXX: warning? */ goto pmdeath; }
 *fds = open(fns,O_RDWR);
 if (*fds == -1) goto pmdeath;

/*
Now nobody but us has the slave side open.
Furthermore, as noted above, nobody but us has the master side open,
and nobody will be able to open either the master or the slave.

But we're still not done! A process can access a tty even if the tty
is completely protected, and even if it doesn't have a descriptor open
to the tty. (Does this sound stupid? It is.) All it has to do is open
/dev/tty, provided that it has the right controlling terminal. Before
pronouncing the tty secure we have to take care of processes with the
same ctty.

We depend on the fact that all methods of associating a process with a
tty depend on (1) having the actual tty (not just /dev/tty) open; or
(2) opening the tty. We know that nobody can have the actual tty open
from here on.
*/

 if (flagchmodworked)
  {
   if (pipe(fdp) == -1)
     goto pdeath;
  
   switch(kidpid = fork())
    {
     case -1:
       close(fdp[0]);
       close(fdp[1]);
       goto pdeath;
     case 0:
       /* XXX: WARNING! We invoke /bin/ps with our privileges! */
       /* If we switched back to the real uid, we couldn't trust the results. */
       close(*fdm); close(*fds);
       close(0); if (dup(fdp[0]) != 0) exit(1);
       close(1); if (dup(fdp[1]) != 1) exit(1);
       close(2); if (dup(fdp[1]) != 2) exit(1);
       close(fdp[0]);
       close(fdp[1]);
       close(0);
       psargs[0] = "/bin/ps";
       psargs[1] = psarg1;
       psarg1[0] = 'c'; psarg1[1] = 'g'; psarg1[2] = 'a'; psarg1[3] = 'x';
       psarg1[4] = 't'; psarg1[5] = ext[0]; psarg1[6] = ext[1]; psarg1[7] = 0;
       psargs[2] = (char *) 0;
       setreuid(getuid(),getuid()); /* XXX: do we really want this? */
       execve(psargs[0],psargs,psargs + 2);
       exit(1);
     default:
       close(fdp[1]);
    }
  
   numpsrets = 0;
   parsingpid = 0;
   while (read(fdp[0],buf,1) == 1)
    {
     if (parsingpid)
       if ((buf[0] != ' ') && (10 > (unsigned long) (unsigned char) (buf[0] - '0')))
         pspid = pspid * 10 + (buf[0] - '0');
       else if (pspid)
        {
         parsingpid = 0;
	 /* XXX: tell user about pspid? */
         if ((pspid == getpid()) || (pspid == kidpid))
           --numpsrets; /*XXX*/
        }
     if (buf[0] == '\n')
      {
       parsingpid = 1;
       pspid = 0;
       ++numpsrets;
      }
    }
   close(fdp[0]);
   wait((int *) 0); /*XXX*/

   if (numpsrets != 1)
    {
     /* XXX: warning? */
     goto pdeath;
    }
  }

/*
We depend on /bin/ps being set up so that numpsrets == 1 implies that
there were, at some point in time, no processes (except possibly us)
with that tty as controlling tty.

Now lots of processes could have /dev/tty open and somehow pointing to
this tty, but there are none (other than us) with the actual tty open,
or with the master open, or with this controlling tty. There's an easy
way to finish off: we simply repeat the first test!
*/
 close(*fds); r = read(*fdm,buf,1);
 if (r > 0) { /* XXX: warning? */ goto pmdeath; }
 if ((r == -1) && (errno != EIO) && (errno != EWOULDBLOCK))
  { /* XXX: warning? */ goto pmdeath; }
 *fds = open(fns,O_RDWR);
 if (*fds == -1) goto pmdeath;

/*
Finally! The pseudo-tty is secure---or at least a hell of a lot more
secure than the ttys you get from any other program.
*/

 if (fchmod(*fds,PTYMODE_USED) == -1)
  {
   /* XXX: warning? */
   ;
  }
 if (flagxchown)
  {
   if (fchown(*fds,getuid(),PTYGROUP) == -1)
    {
     /* XXX: warning? */
     ;
    }
  }
 return 0;

pmdeath:
 *fds = open(fns,O_RDWR);
pdeath:
 if (*fds != -1)
   fchmod(*fds,PTYMODE_UNUSED);
death:
 close(*fds); /* *fds could be -1; that's okay */
mdeath:
 *fds = open("/dev/tty",O_RDWR);
 if (*fds != -1)
  {
   if (tty_dissoc(*fds) == -1)
     ;
   close(*fds);
  }
 close(*fdm);
 return -1;
}

int ptyunsecure(fdm,fds,ext)
int fdm;
int fds;
char *ext;
{
 if (fchmod(fds,PTYMODE_UNUSED) == -1)
  {
   /* XXX: warning? */
   ;
  }
 if (getuid() != storig.st_uid)
   if (fchown(fds,storig.st_uid,PTYGROUP) == -1)
    {
     /* XXX: warning? */
     ;
    }
 return 0;
}
