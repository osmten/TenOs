[bits 32]

global kernel_gdt_load
global kernel_gdt_descriptor
global kernel_gdt_start

kernel_gdt_start:

    dq 0x0000000000000000        ; Null descriptor

    ; Kernel code segment
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00

    ; Kernel data segment
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00

gdt_code_user:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0xFA
    db 0xCF
    db 0x00

gdt_data_user:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0xF2
    db 0xCF
    db 0x00

gdt_tss:
    dw 0x0068        ; Limit (104 bytes - 1)
    dw 0x0000        ; Base low (set in C)
    db 0x00          ; Base middle (set in C)
    db 0xE9          ; 11101001 = P=1, DPL=0, Type=1001 (32-bit TSS)
    db 0x00          ; G=0, AVL=0, Limit high=0000
    db 0x00          ; Base high (set in C)

kernel_gdt_end:

kernel_gdt_descriptor:
    dw kernel_gdt_end - kernel_gdt_start - 1
    dd kernel_gdt_start

; ------------------------------------------------------------------
; Load GDT and update segment registers
; ------------------------------------------------------------------
kernel_gdt_load:
    lgdt [kernel_gdt_descriptor]

    ; Reload segment registers
    mov ax, 0x10       ; DATA = offset 0x10 into GDT
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Reload CS using far jump
    jmp 0x08:flush_cs

flush_cs:
    ret
