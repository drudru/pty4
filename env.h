#ifndef ENV_H
#define ENV_H

extern int env_init();
extern int env_put();
extern int env_put2();
extern int env_unset();
extern char *env_get();
extern char *env_pick();

extern char **environ;

#endif
