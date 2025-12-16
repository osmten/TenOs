#include "idt.h"

idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

void set_idt_gate(int n, u32 handler, u16 selector, u8 flags) {

    idt[n].low_offset = low_16(handler);
    idt[n].sel = selector;
    idt[n].always0 = 0;
    idt[n].flags = flags; 
    idt[n].high_offset = high_16(handler);
}

void set_idt() {
    idt_reg.base = (u32)&idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
    
    __asm__ __volatile__("lidtl (%0)" : : "r" (&idt_reg));
}

