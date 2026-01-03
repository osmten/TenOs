#include "kernel.h"

extern u32 kernel_end;
extern u32 _stack_top;

void main(struct multiboot_info* bootinfo){

	kernel_gdt_load();
	printk_color(VGA_COLOR_CYAN,"============kernel loaded================\n");

	u32 bitmap_addr = ((u32)&kernel_end + 0xFFF) & ~0xFFF; 

    asm volatile("cli");
    isr_install();
    init_timer(50);
    init_keyboard();
	printk_init();

    asm volatile("sti");

	struct memory_region* region = (struct memory_region*)bootinfo->m_mmap_addr;
	int size = bootinfo->m_mmap_length/24;

	// get memory size in KB
	u32 memSize = 1024 + bootinfo->m_memoryLo + (bootinfo->m_memoryHi * 64);
	memSize *=  1024;
	
	pr_debug("KERNEL", "Kernel ends at: %x\n", (u32)&kernel_end);
    pr_debug("KERNEL", "Bitmap starts at: %x\n", bitmap_addr);

	printk_color(VGA_COLOR_MAGENTA,"===========MEMORY MAP START========== \n");

	init_mem_mngr(bitmap_addr, memSize);

	for (int i=0; i<size; ++i) {	
		if (region[i].type>4)
			region[i].type=1;

		if (i>0 && region[i].startLo==0)
			break;

        printk("Address: %x	\n", region[i].startLo);
        printk("length: %d \n", region[i].sizeLo);

		if (region[i].type==1)
			init_memory_region(region[i].startLo, region[i].sizeLo);
	}
	printk_color(VGA_COLOR_MAGENTA,"===========MEMORY MAP END========== \n");
	pr_info("Memory","Total Size %d", memSize);

	printk("Kernel stack top %x\n ", &_stack_top);
	
	vmmngr_initialize(memSize);
	fat12_init();

	// Allocate user stack
    u32 user_stack = (u32)vmmngr_alloc_page() + 0x1000;

	tss_init(&_stack_top);

	jump_usermode(user_stack);

}
