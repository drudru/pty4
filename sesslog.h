#ifndef SESSLOG_H
#define SESSLOG_H

#define SESSLOG_EXTLEN 4
#define SESSLOG_USERLEN 16 /* no harm in planning for the future */

struct sesslog
 {
  char ext[SESSLOG_EXTLEN];
  char username[SESSLOG_USERLEN];
  int uid;
  int masterpid; /* 0 if it's an ending */
  long date;
 }
;

extern int sesslog();
extern void sesslog_fill();
extern void sesslog_disable();

#endif
