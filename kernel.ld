OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(kernel_entry)

SECTIONS {
    . = 0xC0100000;
    
    _kernel_virt_start = .;
    _kernel_phys_start = . - 0xC0000000;
    
    .text : AT(ADDR(.text) - 0xC0000000) {
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    }
    
    . = ALIGN(4096);
    
    .data : AT(ADDR(.data) - 0xC0000000) {
        *(.data)
        *(.data.*)
    }
    
    . = ALIGN(4096);
    
    __bss_start = .;
    .bss : AT(ADDR(.bss) - 0xC0000000) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    __bss_end = .;
    
    . = ALIGN(4096);
    kernel_end = .;
    
    . = 0xC0180000;
    .stack (NOLOAD) : {
        _stack_bottom = .;
        . += 0x10000;
        _stack_top = .;
    }
}