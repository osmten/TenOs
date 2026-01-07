; ; process/jump_usermode.asm
; global jump_usermode_proc

; jump_usermode_proc:
;     mov eax, [esp + 4]  ; Get pointer to registers_t
    
;     ; Load segment registers (offsets 0, 4, 8, 12)
;     mov ax, [eax + 0]   ; gs
;     mov gs, ax
;     mov ax, [eax + 4]   ; fs
;     mov fs, ax
;     mov ax, [eax + 8]   ; es
;     mov es, ax
;     mov ax, [eax + 12]  ; ds
;     mov ds, ax
    
;     ; ═══════════════════════════════════════════════════════════
;     ; Push values for iret (in reverse order)
;     ; iret expects stack to look like:
;     ;   [ESP+16] SS          ← Top
;     ;   [ESP+12] User ESP
;     ;   [ESP+8]  EFLAGS
;     ;   [ESP+4]  CS
;     ;   [ESP+0]  EIP         ← ESP points here
;     ; ═══════════════════════════════════════════════════════════
;     push dword [eax + 64]  ; ss       (offset 64)
;     push dword [eax + 60]  ; user_esp (offset 60)
;     push dword [eax + 56]  ; eflags   (offset 56)
;     push dword [eax + 52]  ; cs       (offset 52)
;     push dword [eax + 48]  ; eip      (offset 48)
    
;     ; Load general purpose registers
;     mov edi, [eax + 16]  ; edi (offset 16)
;     mov esi, [eax + 20]  ; esi (offset 20)
;     mov ebp, [eax + 24]  ; ebp (offset 24)
;     ; Skip esp (offset 28) - will be loaded by iret
;     mov ebx, [eax + 32]  ; ebx (offset 32)
;     mov edx, [eax + 36]  ; edx (offset 36)
;     mov ecx, [eax + 40]  ; ecx (offset 40)
;     ; Load eax last (offset 44)
;     mov eax, [eax + 44]
    
;     ; Jump to user mode!
;     iret


; process/jump_usermode.asm
global jump_usermode_proc
jump_usermode_proc:
    cli                     ; Disable interrupts during transition
    
    mov eax, [esp + 4]      ; Get pointer to registers_t
    
    ; ═══════════════════════════════════════════════════════════
    ; Push values for iret (in REVERSE order!)
    ; ═══════════════════════════════════════════════════════════
    push dword [eax + 64]   ; ss       (offset 64)
    push dword [eax + 60]   ; user_esp (offset 60)
    push dword [eax + 56]   ; eflags   (offset 56)
    push dword [eax + 52]   ; cs       (offset 52)
    push dword [eax + 48]   ; eip      (offset 48)
    
    ; Load segment registers from structure
    ; We need to do this BEFORE loading general registers
    ; because we'll lose access to the structure after loading DS
    mov bx, [eax + 0]       ; gs
    mov cx, [eax + 4]       ; fs  
    mov dx, [eax + 8]       ; es
    mov si, [eax + 12]      ; ds (save for later)
    
    ; Load general purpose registers
    mov edi, [eax + 16]     ; edi
    ; esi will be loaded from saved ds value
    mov ebp, [eax + 24]     ; ebp
    push dword [eax + 32]   ; Save ebx on stack temporarily
    mov edx, [eax + 36]     ; edx
    mov ecx, [eax + 40]     ; ecx
    push dword [eax + 44]   ; Save target eax on stack
    
    ; Now we can safely load segment registers
    mov ax, bx              ; Load gs
    mov gs, ax
    mov ax, cx              ; Load fs (was in cx)
    mov fs, ax
    mov ax, dx              ; Load es (was in dx)
    mov es, ax
    mov ax, si              ; Load ds (was in si)
    mov ds, ax
    
    ; Restore saved registers
    pop eax                 ; eax
    pop ebx                 ; ebx
    
    ; Jump to user mode!
    iret