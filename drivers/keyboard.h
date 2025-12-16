#include <lib/lib.h>
#include "ports.h"
#include "screen.h"
#include <cpu/isr.h>

void init_keyboard();
void print_letter(u8 scancode);
void keyboard_callback(registers_t *regs);
int keyboard_read(char *user_buf, unsigned int count);
