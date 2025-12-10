#include "jmp_user.h"

u8 get_cpl() {
    u16 cs;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    return cs & 0x3;
}

void jump_usermode(u32 user_stack) {

    printk("\n=== Jumping to Ring 3 ===\n");
    printk("User stack:  0x%x\n", user_stack);
    printk("Current CPL: %d\n", get_cpl());
    
    asm volatile(
        "cli\n"                  // Disable interrupts during transition
        
        // Set data segments to user data segment
        "mov $0x23, %%ax\n"      // 0x23 = user data selector (0x20 | 0x3)
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        
        "pushl $0x23\n"          // User data segment (0x20 | 0x3)
        
        "pushl %0\n"             // User stack address

        "pushf\n"                // Push current EFLAGS
        "pop %%eax\n"            // Pop into EAX
        "or $0x200, %%eax\n"     // Set IF bit (enable interrupts)
        "push %%eax\n"           // Push modified EFLAGS
        
        "pushl $0x1B\n"          // User code segment (0x18 | 0x3)
        
        "pushl %1\n"             // Entry point address
        
        "iret\n"
        
        : // No outputs
        : "r"(user_stack), "r"(shell_main)
        : "eax"
    );
}