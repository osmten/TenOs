#ifndef TIMER_H
#define TIMER_H

#include "../kernel/util.h"
#include "isr.h"
 
void timer_callback(registers_t *regs);
void init_timer(int freq);

#endif
