#include "kernel.h"

struct multiboot_info {
	u32	m_flags;
	u32	m_memoryLo;
	u32	m_memoryHi;
	u32	m_bootDevice;
	u32	m_cmdLine;
	u32	m_modsCount;
	u32	m_modsAddr;
	u32	m_syms0;
	u32	m_syms1;
	u32	m_syms2;
	u32	m_mmap_length;
	u32	m_mmap_addr;
	u32	m_drives_length;
	u32	m_drives_addr;
	u32	m_config_table;
	u32	m_bootloader_name;
	u32	m_apm_table;
	u32	m_vbe_control_info;
	u32	m_vbe_mode_info;
	u32	m_vbe_mode;
	u32	m_vbe_interface_addr;
	u16	m_vbe_interface_len;
};

struct memory_region {
	u32	startLo;
	u32	startHi;
	u32	sizeLo;
	u32	sizeHi;
	u32	type;
	u32	acpi_3_0;
};

void main(struct multiboot_info* bootinfo){

	kprint("kernel loaded\n");
    asm volatile("cli");
    isr_install();

    init_timer(50);
    init_keyboard();

    asm volatile("sti");

	struct memory_region* region = (struct memory_region*)bootinfo->m_mmap_addr;
	int size = bootinfo->m_mmap_length/24;

	//! get memory size in KB
	u32 memSize = 1024 + bootinfo->m_memoryLo + bootinfo->m_memoryHi*64;

	kprint("\nTotal Size ");
	char sz[5]={0};
	int_to_ascii(memSize, sz);
	kprint(sz);
	kprint(" ");

	init_mem_mngr(400000, memSize);

	char sc_ascii[5]={0};

	for (int i=0; i<size; ++i) {

		if (region[i].type>4)
			region[i].type=1;

		if (i>0 && region[i].startLo==0)
			break;

        kprint("\nddress ");
       
        int_to_ascii(region[i].startHi, sc_ascii);
        kprint(sc_ascii);
        kprint(" ");

		int_to_ascii(region[i].startLo, sc_ascii);
        kprint(sc_ascii);
        kprint("\nlength ");
        int_to_ascii(region[i].sizeHi, sc_ascii);
        kprint(sc_ascii);
        kprint(" ");

        int_to_ascii(region[i].sizeLo, sc_ascii);
        kprint(sc_ascii);

		if (region[i].type==1)
			pmmngr_init_region (region[i].startLo, region[i].sizeLo);
	}

	kprint("\n");

	// char arr[512];
    // read_sector(200, arr);


    // kprint("First 16 bytes as string: \n");
    // for(int i = 0; i < 16; i++) {
    //     if(arr[i] == 0) break;
	// 	char temp[5];
	// 	int_to_ascii(arr[i], temp);
    //     kprint(temp);
    // }
    kprint('\n');

	fat12_init();
	printk_init();
	shell_init();

	int a = 10;
	char b = 'f';
	char *str = "This is log\0";
	printk("Value of int is %d, char is %c, and string is %s and this is hex %x\n"
			, a, b, str, a);

	pr_debug("","This is DEBUG print");
	pr_info("","This is INFO print");
	pr_err("","This is ERROR print");
	pr_warn("","This is WARN print");
	set_log_level(KERN_DEBUG);
	pr_debug("","This is DEBUG print");

	printk("Zero: %d\n", 0);           // Should print "0"
	printk("Negative: %d\n", -42);     // Should print "-42"
	printk("Hex zero: %x\n", 0);       // Should print "0x0"
	printk("Hex: %x\n", 0xDEADBEEF);   // Should print "0xDEADBEEF"
	printk("Mixed: %d, %x, %s\n", 10, 0xFF, "test");

	pr_debug("MAIN", "This is integer %d", 512);
	while(1)
	{
		printk_color(VGA_COLOR_GREEN,"\nTenOS> ");
		shell_process();
	}
	// fat12_create("OSAMAOS.txt", 100);
	// char *name = "My name is Osama\0";

	// fat12_write(fat12_open("OSAMAOS.txt"), name, 100);

	// char arr[512] = {0};

	// fat12_read(fat12_open("OSAMAOS.txt"), arr, 100);

	// kprint(arr);
	
	// vmmngr_initialize();

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
