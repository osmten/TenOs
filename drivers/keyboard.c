#include "keyboard.h"

int rptr = 0, wptr = 0;
char screen_buff[256] = {0};

#define SC_MAX 57

void keyboard_callback(registers_t *regs) {
    /* The PIC leaves us the scancode in port 0x60 */

    u8 scancode = port_byte_in(0x60);

    if (scancode > SC_MAX) 
        return;
    
    // print_letter(scancode);
    screen_buff[wptr] = scancode;
    wptr++;

    return;
}

int read_key()
{    
    int scancode = screen_buff[rptr];
    if(rptr == wptr)
    {
        return -1;
    }
    rptr++;
    return scancode;
}

void init_keyboard(int a) {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}

