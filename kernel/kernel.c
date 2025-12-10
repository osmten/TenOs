#include "kernel.h"

extern u32 kernel_end;
extern u32 _stack_top;


void test_gdt_and_tss() {
    printk("\n=== GDT and TSS Test ===\n");
    
    // Test 1: Check current segments
    u16 cs, ds, ss;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%ss, %0" : "=r"(ss));
    
    printk("CS: 0x%x (should be 0x08)\n", cs);
    printk("DS: 0x%x (should be 0x10)\n", ds);
    printk("SS: 0x%x (should be 0x10)\n", ss);
    
    // Test 2: Check TSS
    u16 tr;
    asm volatile("str %0" : "=r"(tr));
    printk("TR: 0x%x (should be 0x28)\n", tr);
    
    // Test 3: Check GDT address
    struct {
        u16 limit;
        u32 base;
    } __attribute__((packed)) gdtr;
    
    asm volatile("sgdt %0" : "=m"(gdtr));
    printk("GDTR base: 0x%x\n", gdtr.base);
    printk("GDTR limit: 0x%x\n", gdtr.limit);
    printk("Kernel GDT start: 0x%x\n", (u32)kernel_gdt_start);
    
    if (gdtr.base == (u32)kernel_gdt_start) {
        printk("✓ Kernel GDT is loaded!\n");
    } else {
        printk("✗ Boot GDT still active!\n");
    }
    
    // Test 4: Verify we're in ring 0
    printk("CPL: %d (should be 0)\n", cs & 0x3);
    
    printk("=== Test Complete ===\n\n");
}


void main(struct multiboot_info* bootinfo){

	kernel_gdt_load();
	printk_color(VGA_COLOR_CYAN,"============kernel loaded================\n");

	u32 bitmap_addr = ((u32)&kernel_end + 0xFFF) & ~0xFFF; 

    asm volatile("cli");
    isr_install();
    init_timer(50);
    init_keyboard();
	fat12_init();
	printk_init();

    asm volatile("sti");

	struct memory_region* region = (struct memory_region*)bootinfo->m_mmap_addr;
	int size = bootinfo->m_mmap_length/24;

	//! get memory size in KB
	u32 memSize = 1024 + bootinfo->m_memoryLo + bootinfo->m_memoryHi*64;

	pr_debug("KERNEL", "Kernel ends at: %x\n", (u32)&kernel_end);
    pr_debug("KERNEL", "Bitmap starts at: %x\n", bitmap_addr);

	pr_info("Memory","Total Size %d", memSize);
	printk_color(VGA_COLOR_MAGENTA,"===========MEMORY MAP========== \n");

	init_mem_mngr(bitmap_addr, memSize);

	for (int i=0; i<size; ++i) {	
		if (region[i].type>4)
			region[i].type=1;

		if (i>0 && region[i].startLo==0)
			break;

        printk("Address: %x	\n", region[i].startLo);
        printk("length: %d \n", region[i].sizeLo);

		if (region[i].type==1)
			pmmngr_init_region(region[i].startLo, region[i].sizeLo);
	}
	printk_color(VGA_COLOR_MAGENTA,"===========MEMORY MAP END========== \n");
	printk("Kernel stack top %x\n ", &_stack_top);
	
	vmmngr_initialize();	

	// Allocate user stack
    u32 user_stack = (u32)vmmngr_alloc_page() + 0x1000;

	tss_init(&_stack_top);
	// test_gdt_and_tss();
	jump_usermode(user_stack);

	// u32* p = (u32*)vmmngr_alloc_page();
	// kprint("address\n");
	// int_to_ascii(p, sz);
	// kprint(sz);
	// kprint(" ");
	// u32* p1 = (u32*)pmmngr_alloc_block();
	// kprint("address1\n");
	// int_to_ascii(p1, sz);
	// kprint(sz);
	// kprint(" ");
	// u32* p2 = (u32*)pmmngr_alloc_blocks(3);
	// kprint("\naddress2 ");
	// int_to_ascii(p2, sz);
	// kprint(sz);
	// kprint(" ");
	// u32* p3 = (u32*)pmmngr_alloc_block();
	// kprint("\naddress3 ");
	// int_to_ascii(p3, sz);
	// kprint(sz);
	// kprint(" ");
	//  struct ptable* p;
	// for (int i = 0; i < 512; i++)
	// {
	// 	p = (struct ptable*)pmmngr_alloc_block();
	// 	kprint("address\n");
	// 	int_to_ascii(p, sz);
	// 	kprint(sz);
	// 	kprint(" "); 
    //     memory_set (p, 0, sizeof(struct ptable));
	// }
//    struct ptable* table[512];
//    u32 frame = 0, virt = 0, page_directory_addr = 0;
   
//    for (int i = 0; i < 200; i++)
//    {
//       table[i] = (struct ptable*)pmmngr_alloc_block();
//        //! clear page table
// 	   u32 *p = table[i];
//        memory_set (table[i], 0, sizeof(struct ptable));
//    }
}
