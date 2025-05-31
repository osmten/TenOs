MEMORY {
    text   (rx)  : ORIGIN = 0x100000, LENGTH = 20K  /* Kernel code */
    data   (rw)  : ORIGIN = 0x105000, LENGTH = 10K  /* Kernel data */
    stack  (rw)  : ORIGIN = 0x180000, LENGTH = 64K   /* 64KB stack at 1.5MB */
}


SECTIONS {
    .text 0x100000 : {
        *(.text)
        *(.rodata*)
        *(.eh_frame)
    } > text

    .data 0x105000 : {
        *(.data)
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > data

    .stack (NOLOAD) : {
        . = ALIGN(16);
        _stack_bottom = .;
        . = . + 0xFFFC;  /* 64KB - 4 bytes (account for alignment) */
        _stack_top = .;
        LONG(0xDEADBEEF);  /* Force inclusion */
    } > stack

    /* Explicit global exports */
    PROVIDE(_stack_top = _stack_top);
    PROVIDE(_stack_bottom = _stack_bottom);
    PROVIDE(__bss_start = __bss_start);
    PROVIDE(__bss_end = __bss_end);
}