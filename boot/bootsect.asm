; Identical to lesson 13's boot sector, but the %included files have new paths
[org 0x7c00]

STAGE_2_OFFSET equ 0x500

    mov [BOOT_DRIVE], dl 
    mov bp, 0x9000
    mov sp, bp

    mov bx, MSG_REAL_MODE 
    call print
    call print_nl

    call load_stage2 ;read stage2 bootloader from the disk

    call STAGE_2_OFFSET
    jmp $ ; Never executed

%include "boot/print.asm"
%include "boot/print_hex.asm"
%include "boot/disk.asm"

[bits 16]
load_stage2:
    mov bx, MSG_LOAD_STAGE_2
    call print
    call print_nl

    mov bx, STAGE_2_OFFSET ; Read from disk and store in 0x500
    mov dh, 2 ; Our future kernel will be larger, make this big
     mov al, 0x02
    mov dl, [BOOT_DRIVE]
    call disk_load
    ret

BOOT_DRIVE db 0 ; It is a good idea to store it in memory because 'dl' may get overwritten
MSG_REAL_MODE db "Started in 16-bit Real Mode", 0
MSG_LOAD_STAGE_2 db "Jumping to second stage bootlaoder", 0
MSG_RETURNED_KERNEL db "Returned from kernel. Error?", 0
MSG_DEBUG_DRIVE db "Drive: 0x", 0

; padding
times 510 - ($-$$) db 0
dw 0xaa55
