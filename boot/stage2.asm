[bits 16]
[org 0x500]

KERNEL_OFFSET equ 0x10000 ; The same one we used when linking the kernel
KERNEL_JUMP   equ 0x100000

struc multiboot_info
	.flags				resd	1
	.memoryLo			resd	1
	.memoryHi			resd	1
	.bootDevice			resd	1
	.cmdLine			resd	1
	.mods_count			resd	1
	.mods_addr			resd	1
	.syms0				resd	1
	.syms1				resd	1
	.syms2				resd	1
	.mmap_length		resd	1
	.mmap_addr			resd	1
	.drives_length		resd	1
	.drives_addr		resd	1
	.config_table		resd	1
	.bootloader_name	resd	1
	.apm_table			resd	1
	.vbe_control_info	resd	1
	.vbe_mode_info		resd	1
	.vbe_mode			resw	1
	.vbe_interface_seg	resw	1
	.vbe_interface_off	resw	1
	.vbe_interface_len	resw	1
endstruc


boot_info:
istruc multiboot_info
	at multiboot_info.flags,			dd 0
	at multiboot_info.memoryLo,			dd 0
	at multiboot_info.memoryHi,			dd 0
	at multiboot_info.bootDevice,		dd 0
	at multiboot_info.cmdLine,			dd 0
	at multiboot_info.mods_count,		dd 0
	at multiboot_info.mods_addr,		dd 0
	at multiboot_info.syms0,			dd 0
	at multiboot_info.syms1,			dd 0
	at multiboot_info.syms2,			dd 0
	at multiboot_info.mmap_length,		dd 0
	at multiboot_info.mmap_addr,		dd 0
	at multiboot_info.drives_length,	dd 0
	at multiboot_info.drives_addr,		dd 0
	at multiboot_info.config_table,		dd 0
	at multiboot_info.bootloader_name,	dd 0
	at multiboot_info.apm_table,		dd 0
	at multiboot_info.vbe_control_info,	dd 0
	at multiboot_info.vbe_mode_info,	dw 0
	at multiboot_info.vbe_interface_seg,dw 0
	at multiboot_info.vbe_interface_off,dw 0
	at multiboot_info.vbe_interface_len,dw 0
iend

jmp stage2_main


stage2_main:
    
    
    cli	                   ; clear interrupts
	xor		ax, ax             ; null segments
	mov		ds, ax
	mov		es, ax
    mov     bp, 0x9000
    mov     sp, bp
	sti	                   ; enable interrupts


	call _EnableA20
	xor		eax, eax
	xor		ebx, ebx
	call	BiosGetMemorySize64MB_32Bit

	mov		word [boot_info+multiboot_info.memoryHi], bx
	mov		word [boot_info+multiboot_info.memoryLo], ax

	mov		eax, 0x0
	mov		ds, ax
	mov		di, 0x7e00
	call	BiosGetMemoryMap
	mov  	word [boot_info+multiboot_info.mmap_addr], di
	mov 	eax, [mmap_length]
	mov		dword [boot_info+multiboot_info.mmap_length], eax 

    call load_kernel

    mov bx, MSG_PROT_MODE
    call print
	call print_nl
    call switch_to_pm ; disable interrupts, load GDT,  etc. Finally jumps to 'BEGIN_PM'

    jmp $
    

%include "boot/print.asm"
%include "boot/print_hex.asm"
%include "boot/disk.asm"
%include "boot/gdt.asm"
%include "boot/32bit_print.asm"
%include "boot/switch_pm.asm"
%include "boot/memory.asm"
; %include "boot/paging.asm"


[bits 16]
load_kernel:
    mov bx, MSG_LOAD_KERNEL
    call print
    call print_nl

    ; push es
    ; mov ax, 0x1000
    ; mov es, ax
    ; xor bx, bx
    ; mov dh, 55     ; ← 55 sectors (27.5KB) for .text + .rodata
    ; mov al, 0x04
    ; mov dl, 0x80
    ; call disk_load
    ; pop es

    ;  push es
    ; mov ax, 0x2000  
    ; mov es, ax
    ; xor bx, bx
    ; mov dh, 50    ; ← 16 sectors (8KB) for .data + .bss + padding
    ; mov al, 0x3b   ; ← Start from sector 59 (4 + 55)
    ; mov dl, 0x80
    ; call disk_load
    ; pop es

    push es
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov dh, 98     ; 73 sectors = 37,376 bytes
    mov al, 0x04   ; Start from sector 4
    mov dl, 0x80
    call disk_load
    pop es
    ret

	ret


_EnableA20:

        cli

        call    a20wait
        mov     al,0xAD
        out     0x64,al

        call    a20wait
        mov     al,0xD0
        out     0x64,al

        call    a20wait2
        in      al,0x60
        push    eax

        call    a20wait
        mov     al,0xD1
        out     0x64,al

        call    a20wait
        pop     eax
        or      al,2
        out     0x60,al

        call    a20wait
        mov     al,0xAE
        out     0x64,al

        call    a20wait
        sti
        ret

a20wait:
        in      al,0x64
        test    al,2
        jnz     a20wait
        ret


a20wait2:
        in      al,0x64
        test    al,1
        jz      a20wait2
        ret

    
[bits 32]
BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_string_pm

    ; mov esi, 0x10000
    ; mov edi, 0x100000
    ; mov ecx, (55 * 512) / 4  ; ← Match the sector count
    ; rep movsd

    ; ; Copy second part (.data + .bss)
    ; mov esi, 0x20000
    ; mov edi, 0x108000
    ; mov ecx, (50 * 512) / 4  ; ← Match the sector count
    ; rep movsd

    ; mov eax, 0x2BADB002
    ; mov ebx, boot_info
    ; push boot_info
    ; call KERNEL_JUMP

    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, (73 * 512) / 4  ; All 73 sectors
    rep movsd

    mov eax, 0x2BADB002
    mov ebx, boot_info
    push boot_info
    call KERNEL_JUMP
    jmp $ ; Stay here when the kernel returns control to us (if ever)



MSG_PROT_MODE db "Landed in 32-bit Protected Mode", 0
MSG_LOAD_KERNEL db "Loading kernel into memory", 0

;PADDING
times 1024 - ($-$$) db 0