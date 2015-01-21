/* XXX: this sort of depends on the dirent interface */
#undef POSIX /* JIC XXX */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <stdio.h>
#include "config/ptydir.h"
#include "config/ptylongname.h"
#include "fmt.h"
#include "scan.h"
#include "getopt.h"
#include "ptycomm.h"

main(argc,argv)
int argc;
char *argv[];
{
 int uid;
 DIR *dirp;
 struct direct *dp;
 int opt;
 char sep;
 int flagall;

 uid = getuid();
 sep = '\n';
 flagall = 0;

 while ((opt = getopt(argc,argv,"0a")) != opteof)
   switch(opt)
    {
     case 'a':
       flagall = 1;
       break;
     case '0':
       sep = 0;
       break;
     case '?':
     default:
       exit(1);
    }
 argc -= optind; argv += optind;

 if (chdir(PTYDIR) == -1)
  {
   fprintf(stderr,"%s: fatal: cannot change to session directory %s\n",optprogname,PTYDIR);
   exit(2);
  }

 dirp = opendir(".");
 while (dp = readdir(dirp))
  {
   unsigned int duid;
   char dext[2];
   char *t;
   unsigned int len;

   t = dp->d_name;
   len = scan_strncmp(t,"comm.",5);
   if (len < 5)
     ; /*XXX*/
   t += len;
   len = scan_uint(t,&duid);
   if (len < 1)
     ; /*XXX*/
   t += len;
   len = scan_strncmp(t,".",1);
   if (len < 1)
     ; /*XXX*/
   t += len;
   dext[0] = *t;
   if (dext[0])
     dext[1] = *++t;
   else
     dext[1] = 0;

   if ((duid == uid) || (flagall && !uid)) /*XXXX*/
    {
     int fdcomm;
     int mslavepid;
     int mpid;
     char mrecoext[2];
     char resp6[6];
     int mflagsession;
     char mlongname[PTYLONGNAMELEN];
     static char outbuf[PTYLONGNAMELEN + 200];
     char mext[2];
     int mconn;
     char *t;

#define DO6 read(fdcomm,resp6,6);
#define BUMMER { close(fdcomm); continue; }

     fdcomm = comm_write(dext,uid);
     if (fdcomm == -1)
       continue;
     if (write(fdcomm,"a",1) < 1)
       BUMMER
     DO6
     if (write(fdcomm,"e",1) < 1)
       BUMMER
     if (read(fdcomm,mext,2) < 2)
       BUMMER
     if (write(fdcomm,"l",1) < 1)
       BUMMER
     DO6
     if (!respeq(resp6,"longnm"))
       BUMMER
     if (read(fdcomm,mlongname,sizeof(mlongname)) < sizeof(mlongname))
       BUMMER
     if (write(fdcomm,"C",1) < 1)
       BUMMER
     DO6
     mconn = respeq(resp6,"owuno?");
     if (write(fdcomm,"p",1) < 1)
       BUMMER
     if (read(fdcomm,&mpid,sizeof(mpid)) < sizeof(mpid))
       BUMMER
     if (write(fdcomm,"P",1) < 1)
       BUMMER
     if (read(fdcomm,&mslavepid,sizeof(mslavepid)) < sizeof(mslavepid))
       BUMMER
     if (write(fdcomm,"D",1) < 1)
       BUMMER
     if (read(fdcomm,&mflagsession,sizeof(mflagsession)) < sizeof(mflagsession))
       BUMMER
     if (write(fdcomm,"S",1) < 1)
       BUMMER
     DO6
     if (!respeq(resp6,"latest"))
       BUMMER
     if (read(fdcomm,mrecoext,2) < 2)
       BUMMER
     close(fdcomm);

     if ((mext[0] != dext[0]) || (mext[1] != dext[1]))
       ; /* better to report what the master says */

     t = outbuf;
     if (!mflagsession) t += fmt_strncpy(t,"non-",0);
     t += fmt_strncpy(t,"session ",0);
     *t++ = mext[0];
     *t++ = mext[1];
     t += fmt_strncpy(t," pid ",0);
     t += fmt_uint(t,mpid);
     t += fmt_strncpy(t," slave ",0);
     t += fmt_uint(t,mslavepid);
     if (mflagsession)
      {
       *t++ = ' ';
       if (!mconn) t += fmt_strncpy(t,"dis",0);
       t += fmt_strncpy(t,"connected",0);
      }
     if (mrecoext[0])
      {
       t += fmt_strncpy(t," (will drop into ",0);
       *t++ = mrecoext[0];
       *t++ = mrecoext[1];
       *t++ = ')';
      }
     if (mlongname[0])
      {
       *t++ = ':';
       *t++ = ' ';
       t += fmt_strncpy(t,mlongname,0);
      }
     *t++ = sep;

     fwrite(outbuf,1,t - outbuf,stdout);
    }
  }
 exit(0);
}
