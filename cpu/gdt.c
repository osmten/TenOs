// gdt.c
#include "gdt.h"

// Global TSS
tss_entry_t tss;

// Set TSS descriptor in GDT
void gdt_set_tss(u32 base, u32 limit) {

    
    gdt_entry_t *tss_entry = &kernel_gdt_start[5];  // Entry 5 is TSS
    
    // Set base address (split across 3 fields)
    tss_entry->base_low = base & 0xFFFF;
    tss_entry->base_middle = (base >> 16) & 0xFF;
    tss_entry->base_high = (base >> 24) & 0xFF;
    
    // Set limit (split across 2 fields)
    tss_entry->limit_low = limit & 0xFFFF;
    tss_entry->limit_high = (limit >> 16) & 0x0F;
    
    // Set access byte
    // 0x89 = 10001001 binary
    // Bit 7: Present = 1
    // Bit 6-5: DPL = 00 (ring 0)
    // Bit 4: Descriptor type = 0 (system segment)
    // Bit 3-0: Type = 1001 (32-bit TSS, available)
    tss_entry->access = 0x89;
    
    // Set flags
    // 0x0 = byte granularity, 32-bit (actually ignored for TSS)
    tss_entry->flags = 0x00;
    
}

// Initialize TSS
void tss_init(u32 kernel_stack) {
    printk("\nInitializing TSS...\n");
    printk("  TSS address: %x\n", (u32)&tss);
    printk("  TSS size: %d bytes\n", sizeof(tss));
    printk("  Kernel stack: %x\n", kernel_stack);
    
    // Clear TSS
    memory_set((u8*)&tss, 0, sizeof(tss));
    
    // Set kernel stack (used when switching from ring 3 to ring 0)
    tss.ss0 = DATA_SEG;    // 0x10 = kernel data segment
    tss.esp0 = kernel_stack;
    
    // Set I/O map base address (we don't use I/O permissions, so set to TSS size)
    tss.iomap_base = sizeof(tss);
    
    // Set TSS descriptor in GDT
    gdt_set_tss((u32)&tss, sizeof(tss) - 1);  // Limit = size - 1
    
    printk("  TSS descriptor set in GDT\n");
    
    // Load TSS into Task Register
    // Selector: 0x28 (TSS_SEG) with RPL=0 (we're in ring 0 now)
    asm volatile("ltr %%ax" : : "a"(TSS_SEG));  // 0x28, NOT 0x2B!
    
    printk("  TSS loaded into TR\n");
    
    // Verify TSS is loaded
    u16 tr;
    asm volatile("str %%ax" : "=a"(tr));
    printk("  Task Register: %x (should be 0x28)\n", tr);
}

// Update kernel stack (for multitasking - not needed now)
void tss_set_kernel_stack(u32 stack) {
    tss.esp0 = stack;
}