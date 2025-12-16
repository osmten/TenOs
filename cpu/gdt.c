#include "gdt.h"

tss_entry_t tss;

// Set TSS descriptor in GDT
void gdt_set_tss(u32 base, u32 limit) {
    
    gdt_entry_t *tss_entry = &kernel_gdt_start[5];
    
    tss_entry->base_low = base & 0xFFFF;
    tss_entry->base_middle = (base >> 16) & 0xFF;
    tss_entry->base_high = (base >> 24) & 0xFF;
    
    tss_entry->limit_low = limit & 0xFFFF;
    tss_entry->limit_high = (limit >> 16) & 0x0F;
    
    // Set access byte
    // 0x89 = 10001001 binary
    // Bit 7: Present = 1
    // Bit 6-5: DPL = 00 (ring 0)
    // Bit 4: Descriptor type = 0 (system segment)
    // Bit 3-0: Type = 1001 (32-bit TSS, available)
    tss_entry->access = 0x89;
    
    tss_entry->flags = 0x00;
    
}

// Initialize TSS
void tss_init(u32 kernel_stack) {

    printk("\nInitializing TSS...\n");
    pr_debug("TSS","TSS address: %x\n", (u32)&tss);
    pr_debug("TSS","TSS size: %d bytes\n", sizeof(tss));
    pr_debug("TSS","Kernel stack: %x\n", kernel_stack);
    
    memory_set((u8*)&tss, 0, sizeof(tss));
    
    // Set kernel stack (used when switching from ring 3 to ring 0)
    tss.ss0 = DATA_SEG;
    tss.esp0 = kernel_stack;
    
    tss.iomap_base = sizeof(tss);
    
    gdt_set_tss((u32)&tss, sizeof(tss) - 1);
    
    printk("TSS descriptor set in GDT\n");
    
    asm volatile("ltr %%ax" : : "a"(TSS_SEG));
    
    printk("TSS loaded into TR\n");
    
    // Verify TSS is loaded
    u16 tr;
    asm volatile("str %%ax" : "=a"(tr));
    printk("Task Register: %x (should be 0x28)\n", tr);
}