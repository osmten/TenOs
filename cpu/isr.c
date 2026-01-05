#include "isr.h"

isr_t interrupt_handlers[256] = {0};

void isr_install() {
    set_idt_gate(0, (u32)isr0, KERNEL_CS, 0x8E);
    set_idt_gate(1, (u32)isr1, KERNEL_CS, 0x8E);
    set_idt_gate(2, (u32)isr2, KERNEL_CS, 0x8E);
    set_idt_gate(3, (u32)isr3, KERNEL_CS, 0x8E);
    set_idt_gate(4, (u32)isr4, KERNEL_CS, 0x8E);
    set_idt_gate(5, (u32)isr5, KERNEL_CS, 0x8E);
    set_idt_gate(6, (u32)isr6, KERNEL_CS, 0x8E);
    set_idt_gate(7, (u32)isr7, KERNEL_CS, 0x8E);
    set_idt_gate(8, (u32)isr8, KERNEL_CS, 0x8E);
    set_idt_gate(9, (u32)isr9, KERNEL_CS, 0x8E);
    set_idt_gate(10, (u32)isr10, KERNEL_CS, 0x8E);
    set_idt_gate(11, (u32)isr11, KERNEL_CS, 0x8E);
    set_idt_gate(12, (u32)isr12, KERNEL_CS, 0x8E);
    set_idt_gate(13, (u32)isr13, KERNEL_CS, 0x8E);
    set_idt_gate(14, (u32)isr14, KERNEL_CS, 0x8E);
    set_idt_gate(15, (u32)isr15, KERNEL_CS, 0x8E);
    set_idt_gate(16, (u32)isr16, KERNEL_CS, 0x8E);
    set_idt_gate(17, (u32)isr17, KERNEL_CS, 0x8E);
    set_idt_gate(18, (u32)isr18, KERNEL_CS, 0x8E);
    set_idt_gate(19, (u32)isr19, KERNEL_CS, 0x8E);
    set_idt_gate(20, (u32)isr20, KERNEL_CS, 0x8E);
    set_idt_gate(21, (u32)isr21, KERNEL_CS, 0x8E);
    set_idt_gate(22, (u32)isr22, KERNEL_CS, 0x8E);
    set_idt_gate(23, (u32)isr23, KERNEL_CS, 0x8E);
    set_idt_gate(24, (u32)isr24, KERNEL_CS, 0x8E);
    set_idt_gate(25, (u32)isr25, KERNEL_CS, 0x8E);
    set_idt_gate(26, (u32)isr26, KERNEL_CS, 0x8E);
    set_idt_gate(27, (u32)isr27, KERNEL_CS, 0x8E);
    set_idt_gate(28, (u32)isr28, KERNEL_CS, 0x8E);
    set_idt_gate(29, (u32)isr29, KERNEL_CS, 0x8E);
    set_idt_gate(30, (u32)isr30, KERNEL_CS, 0x8E);
    set_idt_gate(31, (u32)isr31, KERNEL_CS, 0x8E);

    // Remap the PIC
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, 0x20);
    port_byte_out(0xA1, 0x28);
    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);
    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);
    port_byte_out(0x21, 0x00);
    port_byte_out(0xA1, 0x00); 

    // Install the IRQs
    set_idt_gate(32, (u32)irq0, KERNEL_CS, 0x8E);
    set_idt_gate(33, (u32)irq1, KERNEL_CS, 0x8E);
    set_idt_gate(34, (u32)irq2, KERNEL_CS, 0x8E);
    set_idt_gate(35, (u32)irq3, KERNEL_CS, 0x8E);
    set_idt_gate(36, (u32)irq4, KERNEL_CS, 0x8E);
    set_idt_gate(37, (u32)irq5, KERNEL_CS, 0x8E);
    set_idt_gate(38, (u32)irq6, KERNEL_CS, 0x8E);
    set_idt_gate(39, (u32)irq7, KERNEL_CS, 0x8E);
    set_idt_gate(40, (u32)irq8, KERNEL_CS, 0x8E);
    set_idt_gate(41, (u32)irq9, KERNEL_CS, 0x8E);
    set_idt_gate(42, (u32)irq10, KERNEL_CS, 0x8E);
    set_idt_gate(43, (u32)irq11, KERNEL_CS, 0x8E);
    set_idt_gate(44, (u32)irq12, KERNEL_CS, 0x8E);
    set_idt_gate(45, (u32)irq13, KERNEL_CS, 0x8E);
    set_idt_gate(46, (u32)irq14, KERNEL_CS, 0x8E);
    set_idt_gate(47, (u32)irq15, KERNEL_CS, 0x8E);

    /* system call entry*/
    set_idt_gate(0x80, (u32)syscall_handler_asm, 0x08, 0xEE);

    set_idt();
}

/* To print the message which defines every exception */
char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// void isr_handler(registers_t *r) {
//     printk("Received interrupt: %d -> %s\n", r->int_no, exception_messages[r->int_no]);
//     /* Todo: Remove this. Add proper handling */
//     while(1)
//         asm volatile("hlt");
// }

void isr_handler(registers_t *r) {
    // Special handling for page fault (interrupt 14)
    if (r->int_no == 14) {
        // Read CR2 register (contains faulting address)
        u32 faulting_address;
        __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));
        
        printk("\n========== PAGE FAULT ==========\n");
        printk("Faulting address: 0x%x\n", faulting_address);
        printk("Error code: 0x%x\n", r->err_code);
        
        // Decode error code
        printk("  ");
        if (!(r->err_code & 0x1)) printk("[NOT PRESENT] ");
        else printk("[PROTECTION] ");
        
        if (r->err_code & 0x2) printk("[WRITE] ");
        else printk("[READ] ");
        
        if (r->err_code & 0x4) printk("[USER MODE] ");
        else printk("[KERNEL MODE] ");
        
        if (r->err_code & 0x8) printk("[RESERVED BITS] ");
        if (r->err_code & 0x10) printk("[INSTRUCTION FETCH] ");
        
        printk("\n");
        printk("EIP: 0x%x\n", r->eip);
        printk("ESP: 0x%x\n", r->esp);
        printk("EBP: 0x%x\n", r->ebp);
        printk("CS: 0x%x, SS: 0x%x\n", r->cs, r->ss);
        printk("================================\n");
        
        while(1) asm volatile("hlt");
    }
    
    // Handle other exceptions
    printk("Received interrupt: %d -> %s\n", r->int_no, exception_messages[r->int_no]);
    printk("  EIP: 0x%x\n", r->eip);
    printk("  Error code: 0x%x\n", r->err_code);
    
    while(1) asm volatile("hlt");
}

void register_interrupt_handler(u8 n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void irq_handler(registers_t *r) {
    /* After every interrupt send an EOI to the PICs
     * or they will not send another interrupt again */

    if (r->int_no >= 40) port_byte_out(0xA0, 0x20); /* slave */
    port_byte_out(0x20, 0x20); /* master */

    if (interrupt_handlers[r->int_no] != 0) {
        isr_t handler = interrupt_handlers[r->int_no];
        handler(r);
    }

}