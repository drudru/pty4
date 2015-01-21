#ifndef SIGSCHED_H
#define SIGSCHED_H

typedef struct
 {
  int type;
  union { int n; char *c; } u;
 }
ss_sig;

typedef struct
 {
  int (*sched)();
  int (*unsched)();
  union { int n; char *c; } u;
 }
ss_extern;

typedef void ss_thread();
typedef int ss_id;
typedef char *ss_idptr;

extern ss_sig *ss_asap();
extern ss_sig *ss_signal();
extern ss_sig *ss_sigread();
extern ss_sig *ss_sigwrite();
extern ss_sig *ss_sigexcept();

extern int ss_addsig();

extern void ss_externsetsig();

extern int ss_schedvwait();
extern int ss_schedwait();
extern int ss_sched();
extern int ss_schedonce();
extern int ss_unschedv();
extern int ss_unsched();

extern void ss_forcewait();
extern void ss_unforcewait();

extern int ss_exec();

#endif
