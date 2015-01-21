#ifndef SIGDFL_H
#define SIGDFL_H

extern int sigdfl();

extern int sigdfl_tstp();
extern int sigdfl_stop();
extern int sigdfl_ttin();
extern int sigdfl_ttou();

extern int sigdfl_abrt(); /* professional version of abort() */

#endif
