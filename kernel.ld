/* MEMORY {
    text   (rx)  : ORIGIN = 0x100000, LENGTH = 32K
    data   (rw)  : ORIGIN = 0x108000, LENGTH = 16K
    stack  (rw)  : ORIGIN = 0x180000, LENGTH = 64K
}

SECTIONS {
    .text 0x100000 : ALIGN(4) {
        *(.text)
        *(.text.*)
        . = ALIGN(16);
        *(.rodata*)
        . = ALIGN(4);
        *(.eh_frame)
        . = ALIGN(4);
    } > text

    .data 0x108000 : ALIGN(4) {
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        . = ALIGN(4);
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > data

    . = ALIGN(4096);
    kernel_end = .; 

    .stack 0x180000 (NOLOAD) : ALIGN(16) { 
        _stack_bottom = .;
        . = . + 0xFFFC;
        _stack_top = .;
    } > stack

    PROVIDE(_stack_top = _stack_top);
    PROVIDE(_stack_bottom = _stack_bottom);
    PROVIDE(__bss_start = __bss_start);
    PROVIDE(__bss_end = __bss_end);
    PROVIDE(__kernel_end = __kernel_end);
} */

/* 
MEMORY {
    text   (rx)  : ORIGIN = 0x100000, LENGTH = 32K
    data   (rw)  : ORIGIN = 0x108000, LENGTH = 16K
    stack  (rw)  : ORIGIN = 0x180000, LENGTH = 64K
}

SECTIONS {
    
    . =  0xC0000000;
    
    .text : AT(0x100000) ALIGN(4) {
        *(.text)
        *(.text.*)
        . = ALIGN(16);
        *(.rodata*)
        . = ALIGN(4);
        *(.eh_frame)
        . = ALIGN(4);
    } > text

    .data : AT(0x108000) ALIGN(4) {
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        . = ALIGN(4);
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > data

    . = ALIGN(4096);
    kernel_end = .; 

    .stack : AT(0x180000) ALIGN(16) { 
        _stack_bottom = .;
        . = . + 0xFFFC;
        _stack_top = .;
    } > stack

    PROVIDE(_stack_top = _stack_top);
    PROVIDE(_stack_bottom = _stack_bottom);
    PROVIDE(__bss_start = __bss_start);
    PROVIDE(__bss_end = __bss_end);
    PROVIDE(__kernel_end = kernel_end);
} */

/* OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(kernel_entry)

SECTIONS {
    . = 0xC0100000;
    
    .text : AT(0x100000) {
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    }
    
    . = ALIGN(4096);
    
    .data : AT(0x108000) {
        *(.data)
        *(.data.*)
    }
    
    __bss_start = .;
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    __bss_end = .;
    
    kernel_end = .;
    
    . = 0xC0180000;
    _stack_bottom = .;
    . += 0x10000;
    _stack_top = .;
} */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(kernel_entry)

SECTIONS {
    /* Virtual base address */
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