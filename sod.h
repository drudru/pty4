#ifndef SOD_H
#define SOD_H

/* a half-hearted attempt at a generic stack library */

#define SODdecl(foostack,foo) \
typedef struct foostack { struct foostack *next; foo data; } *foostack
  /* note that user must supply semicolon */

#define SODnext(x) ((x)->next)
#define SODdata(x) ((x)->data)
#define SODalloc(t,x,ralloc) ((t) ((ralloc)(sizeof(*x))))
#define SODpush(x,y) ((y)->next = (x),(x) = (y))
#define SODpop(x,y) ((y) = (x),(x) = (x)->next)
#define SODfree(u,rfree) ((rfree)((char *)(u)))

#endif
