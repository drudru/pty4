#ifndef TIMER_H
#define TIMER_H

#include "sigsched.h"

#define TIMER_REAL ((timer_type) 0)
#define TIMER_VIRTUAL ((timer_type) 1)
#define TIMER_PROF ((timer_type) 2)
#define TIMER_NUM ((timer_type) 3)

typedef int timer_type;
typedef struct { unsigned long sec; unsigned long usec; } timer_clock;
typedef struct { ss_sig sig; ss_extern x; timer_type t; timer_clock when; } timer_sig;

extern int timer_now();
extern void timer_sum();
extern int timer_diff();
extern void timer_setsig();
extern int timer_init();

#endif
