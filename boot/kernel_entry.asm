[bits 32]
[extern main]
[extern _stack_top]
[extern __bss_end]
[extern __bss_start]

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
