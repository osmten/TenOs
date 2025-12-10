gdt_start:
    ; Null descriptor
    dq 0x0000000000000000

; Kernel code (Ring 0)
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A          ; 10011010 = P=1, DPL=0, D=1, Type=1010
    db 0xCF          ; G=1, D/B=1, Limit high=1111
    db 0x00

; Kernel data (Ring 0)
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92          ; 10010010 = P=1, DPL=0, D=1, Type=0010
    db 0xCF
    db 0x00

gdt_end:

; GDT descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; size (16 bit), always one less of its true size
    dd gdt_start ; address (32 bit)

; define some constants for later use
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

