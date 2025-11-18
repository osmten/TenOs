#include "../cpu/types.h"
#include "../cpu/isr.h"

void init_keyboard();
void print_letter(u8 scancode);
void keyboard_callback(registers_t *regs);
