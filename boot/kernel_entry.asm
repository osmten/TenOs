[bits 32]
[extern main]
[extern _stack_top]
[extern __bss_end]
[extern __bss_start]

global kernel_entry

kernel_entry:
    ; Verify symbols are valid
    mov eax, _stack_top
    cmp eax, 0
    je .bad_symbols

    mov eax, [esp+4] ; Multiboot info

    ; Set up stack
    mov esp, _stack_top
    push eax
    mov ebp, esp

    ; Clear BSS (critical!)
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    ; Continue boot

    call main
    jmp $

.bad_symbols:
    mov dword [0xB8000], 0x4F524F45 ; Print 'ERR' to screen
    mov dword [0xB8004], 0x4F3A4F52
    mov dword [0xB8008], 0x4F204F20
    jmp $


;[bits 32]
;[extern main]
;[extern _stack_top]
;[extern __bss_end]
;[extern __bss_start]
;
;global kernel_entry
;
;kernel_entry:
;    ; At this point:
;    ; - Paging is ENABLED (by paging.asm)
;    ; - We're executing at 0xC0... addresses
;    ; - Identity mapping is REMOVED
;    ; - EAX = 0x2BADB002
;    ; - EBX = multiboot info (physical address)
;    
;    ; Verify stack
;    mov esp, _stack_top
;    cmp esp, 0xC0000000
;    jb .bad_stack  ; If stack is below 3GB, something's wrong
;    
;    mov ebp, esp
;    push ebx  ; Save multiboot info
;    
;    ; Clear BSS
;    mov edi, __bss_start
;    mov ecx, __bss_end
;    sub ecx, edi
;    xor eax, eax
;    rep stosb
;    
;    ; Restore multiboot and call main
;    pop eax
;    push eax
;    call main
;    
;    cli
;    hlt
;    jmp $
;
;.bad_stack:
;    mov dword [0xB8000], 0x4F524F45  ; "ERR"
;    jmp $