;[bits 32]
;
;global syscall_handler_asm
;extern syscall_dispatcher
;
;syscall_handler_asm:
;    ; Save only EAX, EBX, ECX, EDX (common syscall convention)
;    push ebx
;    push ecx
;    push edx
;    
;    push edx
;    push ecx
;    push ebx
;    push eax
;    call syscall_dispatcher
;    add esp, 16
;    
;    ; Restore (EAX has return value, don't restore it!)
;    pop edx
;    pop ecx
;    pop ebx
;    
;    iret

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
    
    ; Push parameters for C function (reverse order)
    push edi      ; arg5
    push esi      ; arg4
    push edx      ; arg3
    push ecx      ; arg2
    push ebx      ; arg1
    push eax      ; syscall_num
    
    ; Call dispatcher
    call syscall_dispatcher
    
    ; Clean up parameters (6 * 4 = 24 bytes)
    add esp, 24
    
    ; Restore saved registers (skip EAX - it has return value)
    add esp, 4    ; Skip saved EAX
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi
    
    ; Return to user mode
    iret