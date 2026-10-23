#ifndef CHOS_TIMER_H
#define CHOS_TIMER_H
#include "platform.h"
uint64_t timer_redistributor(void);
bool timer_init(const struct platform *);
uint64_t timer_ticks(void);
#endif
