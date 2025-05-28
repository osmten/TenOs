MEMORY {
    text   (rx)  : ORIGIN = 0x2000, LENGTH = 16K
    data   (rw)  : ORIGIN = 0x6000, LENGTH = 16K
    stack  (rw)  : ORIGIN = 0xA000, LENGTH = 16K  /* 40KB aligned */
}

SECTIONS {
    .text 0x2000 : {
        *(.text)
        *(.rodata*)
        *(.eh_frame)
    } > text

    .data 0x6000 : {
        *(.data)
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > data

    .stack (NOLOAD) : {
        . = ALIGN(16);
        _stack_bottom = .;
        . = . + 0x3FFC;  /* 16KB - 4 bytes (account for alignment) */
        _stack_top = .;
        LONG(0xDEADBEEF);  /* Force inclusion */
    } > stack

    /* Explicit global exports */
    PROVIDE(_stack_top = _stack_top);
    PROVIDE(_stack_bottom = _stack_bottom);
    PROVIDE(__bss_start = __bss_start);
    PROVIDE(__bss_end = __bss_end);
}