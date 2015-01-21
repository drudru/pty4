#ifndef GETOPT_H
#define GETOPT_H

#ifndef GETOPTORIGDEF
#define getopt getoptmine
#define optarg getoptarg
#define optind getoptind
#define opterr getopterr
#define optpos getoptpos
#define optproblem getoptproblem
#define optprogname getoptprogname
#define opteof getopteof
#endif

extern int getopt();
extern char *optarg;
extern int optind;
extern int opterr;
extern int optpos;
extern int optproblem;
extern char *optprogname;
extern int opteof;

#endif
