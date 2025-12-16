#ifndef GDT_H
#define GDT_H

#include <lib/lib.h>
typedef struct {
    u16 limit_low;      // Limit bits 0-15
    u16 base_low;       // Base bits 0-15
    u8  base_middle;    // Base bits 16-23
    u8  access;         // Access byte
    u8  limit_high : 4; // Limit bits 16-19
    u8  flags : 4;      // Flags (granularity, size, etc.)
    u8  base_high;      // Base bits 24-31
} __attribute__((packed)) gdt_entry_t;

// Segment selectors
#define CODE_SEG      0x08
#define DATA_SEG      0x10
#define USER_CODE_SEG 0x18
#define USER_DATA_SEG 0x20
#define TSS_SEG       0x28

typedef struct {
    u32 prev_tss;
    u32 esp0;       // Kernel stack pointer (IMPORTANT!)
    u32 ss0;        // Kernel stack segment (IMPORTANT!)
    u32 esp1;
    u32 ss1;
    u32 esp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax, ecx, edx, ebx;
    u32 esp, ebp, esi, edi;
    u32 es, cs, ss, ds, fs, gs;
    u32 ldt;
    u16 trap;
    u16 iomap_base;
} __attribute__((packed)) tss_entry_t;

extern gdt_entry_t kernel_gdt_start[6];

void gdt_set_tss(u32 base, u32 limit);
void tss_init(u32 kernel_stack);

#endif