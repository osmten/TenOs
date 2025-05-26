#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"
#include "../cpu/memory.h"

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


void main(struct multiboot_info* bootinfo) {
    kprint("\nKernel loaded:\n");
    isr_install();
    asm volatile("sti");

    init_timer(50);
    init_keyboard();

	struct memory_region*	region = (struct memory_region*)bootinfo->m_mmap_addr;
	int size = bootinfo->m_mmap_length/24;

	//! get memory size in KB
	u32 memSize = 1024 + bootinfo->m_memoryLo + bootinfo->m_memoryHi*64;

	kprint("\nTotal Size ");
	char *sz;
	int_to_ascii(memSize, sz);
	kprint(sz);
	kprint(" ");

	init_mem_mngr(0x100000, memSize);

	for (int i=0; i<size; ++i) {

		if (region[i].type>4)
			region[i].type=1;

		if (i>0 && region[i].startLo==0)
			break;

        kprint("\nddress ");
        char *sc_ascii;
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
	u32* p = (u32*)pmmngr_alloc_block();
	kprint("address\n");
	int_to_ascii(p, sz);
	kprint(sz);
	kprint(" ");
	u32* p1 = (u32*)pmmngr_alloc_block();
	kprint("address1\n");
	int_to_ascii(p1, sz);
	kprint(sz);
	kprint(" ");
	u32* p2 = (u32*)pmmngr_alloc_blocks(3);
	kprint("\naddress2 ");
	int_to_ascii(p2, sz);
	kprint(sz);
	kprint(" ");
	u32* p3 = (u32*)pmmngr_alloc_block();
	kprint("\naddress3 ");
	int_to_ascii(p3, sz);
	kprint(sz);
	kprint(" ");
}
