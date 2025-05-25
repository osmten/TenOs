#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"

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

//    struct memory_region *mr = (struct memory_region*)bootinfo;
    //kprint(bootinfo->m_mmap_addr);

    asm volatile("sti");

    init_timer(50);
    init_keyboard();
}
