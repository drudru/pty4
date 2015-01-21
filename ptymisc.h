#ifndef PTY_MISC_H
#define PTY_MISC_H

extern long now();
extern int gaargh();
extern int forceopen();
extern int respeq();
extern int bread();
extern int bwrite();
extern int lflock();
extern int lfunlock();
extern int setnonblock();
extern int unsetnonblock();

#endif
