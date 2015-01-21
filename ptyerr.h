#ifndef PTY_ERR_H
#define PTY_ERR_H

extern void die();
extern void warn();
extern void warn_disable();
extern void info();

#define DIE_USAGE 1
#define DIE_NOCTTY 2
#define DIE_GETMODES 3
#define DIE_SETMODES 4
#define DIE_NOPTYS 5
#define DIE_SETUP 6
#define DIE_FORK 7
#define DIE_PTYDIR 8
#define DIE_NOMEM 9
#define DIE_IMPOSSIBLE 10
#define DIE_COMM 11
#define DIE_ELSE 16
#define DIE_EXIST 17

#endif
