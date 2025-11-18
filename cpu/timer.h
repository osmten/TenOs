#ifndef TIMER_H
#define TIMER_H

#include <lib/lib.h>
#include "isr.h"
#include <drivers/ports.h>
 
void timer_callback(registers_t *regs);
void init_timer(int freq);

#endif
