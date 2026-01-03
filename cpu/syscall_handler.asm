[bits 32]
global syscall_handler_asm
extern syscall_dispatcher

syscall_handler_asm:
    ; ═══════════════════════════════════════════════════════════
    ; Syscall convention:
    ; EAX = syscall number
    ; EBX = arg1
    ; ECX = arg2
    ; EDX = arg3
    ; ESI = arg4 (if used)
    ; EDI = arg5 (if used)
    ; ═══════════════════════════════════════════════════════════
    
    ; Save all caller-saved registers
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax
    
    push edi      ; arg5
    push esi      ; arg4
    push edx      ; arg3
    push ecx      ; arg2
    push ebx      ; arg1
    push eax      ; syscall_num
    
    call syscall_dispatcher
    
    ; Clean up parameters (6 * 4 = 24 bytes)
    add esp, 24
    
    add esp, 4    ; Skip saved EAX
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi

    iret