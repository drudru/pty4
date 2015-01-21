#ifndef RALLOC_H
#define RALLOC_H

extern char *ralloc();
extern void rfree();
extern int rcount();
extern int rallocinstall();
extern void rallocneverfail();

#define RFREE(x) rfree((char *) (x))
#define RALLOC(t,x) ((t *) ralloc((x) * sizeof(t)))

#endif
