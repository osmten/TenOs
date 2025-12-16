[bits 32]

global syscall_handler_asm
extern syscall_dispatcher

syscall_handler_asm:
    ; Save only EAX, EBX, ECX, EDX (common syscall convention)
    push ebx
    push ecx
    push edx
    
    push edx
    push ecx
    push ebx
    push eax
    call syscall_dispatcher
    add esp, 16
    
    ; Restore (EAX has return value, don't restore it!)
    pop edx
    pop ecx
    pop ebx
    
    iret