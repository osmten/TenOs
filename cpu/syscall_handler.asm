[bits 32]
global syscall_handler_asm
extern syscall_dispatcher

syscall_handler_asm:
    ; CPU has pushed: SS, ESP, EFLAGS, CS, EIP
    
    ; Save all registers
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax
    
    ; Stack: [EAX][EBX][ECX][EDX][ESI][EDI][EIP][CS][EFLAGS][ESP][SS]
    
    ; Push arguments (reverse order for C calling convention)
    push edi      ; arg5
    push esi      ; arg4
    push edx      ; arg3
    push ecx      ; arg2
    push ebx      ; arg1
    push eax      ; syscall_num
    
    ; Stack: [num][a1][a2][a3][a4][a5][EAX][EBX][ECX][EDX][ESI][EDI][EIP]...
    
    call syscall_dispatcher
    ; EAX has return value!
    
    ; Clean up function arguments (6 args * 4 bytes = 24)
    add esp, 24
    
    ; Stack back to: [EAX][EBX][ECX][EDX][ESI][EDI][EIP][CS][EFLAGS][ESP][SS]
    ;                 ↑ ESP
    
    ; Preserve return value by storing it in saved EAX slot
    mov [esp], eax
    
    ; Restore all registers (now EAX will have the return value!)
    pop eax       ; EAX = return value from syscall
    pop ebx       ; EBX = original value
    pop ecx       ; ECX = original value
    pop edx       ; EDX = original value
    pop esi       ; ESI = original value
    pop edi       ; EDI = original value
    
    ; Stack now: [EIP][CS][EFLAGS][ESP][SS]
    ; Perfect for IRET!
    
    iret