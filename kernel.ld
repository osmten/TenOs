MEMORY {
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

    .stack 0x180000 (NOLOAD) : ALIGN(16) { 
        _stack_bottom = .;
        . = . + 0xFFFC;
        _stack_top = .;
    } > stack

    PROVIDE(_stack_top = _stack_top);
    PROVIDE(_stack_bottom = _stack_bottom);
    PROVIDE(__bss_start = __bss_start);
    PROVIDE(__bss_end = __bss_end);
}