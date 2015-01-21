#ifndef SESSCONNLOG_H
#define SESSCONNLOG_H

#define SESSCONNLOG_EXTLEN 4
#define SESSCONNLOG_REMOTELEN 116 /* reasonable for the moment... */

struct sessconnlog
 {
  char ext[SESSCONNLOG_EXTLEN];
  long date;
  int siglerpid; /* 0 for unused---currently inaccurate otherwise */
  char remote[SESSCONNLOG_REMOTELEN]; /* always 0-terminated */
 }
;

extern int sessconnlog();
extern void sessconnlog_fill();
extern void sessconnlog_disable();

#endif
