; boot/paging.asm
bits 32

; Memory locations for page structures
%define PAGE_DIR          0x9C000
%define PAGE_TABLE_0      0x9D000
%define PAGE_TABLE_768    0x9E000
%define PAGE_TABLE_ENTRIES 1024
%define PRIV              0x007    ; Present, Writable, Supervisor

global setup_paging

section .text

;****************************************
; Setup and Enable Paging
; - Identity maps first 4MB (0-4MB)
; - Maps 3GB to physical 1MB (where kernel is)
;****************************************
setup_paging:
    pusha
    
    ;------------------------------------------
    ; Clear page directory
    ;------------------------------------------
    mov edi, PAGE_DIR
    mov ecx, 1024
    xor eax, eax
    rep stosd
    
    ;------------------------------------------
    ; Identity map first 4MB (0x00000000 - 0x003FFFFF)
    ;------------------------------------------
    mov eax, PAGE_TABLE_0           ; Point to first page table
    mov ebx, 0x0 | PRIV             ; Start at physical 0
    mov ecx, PAGE_TABLE_ENTRIES
    
.loop1:
    mov dword [eax], ebx            ; Map page
    add eax, 4                      ; Next entry
    add ebx, 4096                   ; Next physical page (4KB)
    loop .loop1
    
    ;------------------------------------------
    ; Set up page directory entries
    ;------------------------------------------
    mov eax, PAGE_TABLE_0 | PRIV
    mov dword [PAGE_DIR], eax       ; PD[0] -> Page Table 0
    
    mov eax, PAGE_TABLE_768 | PRIV
    mov dword [PAGE_DIR + (768*4)], eax  ; PD[768] -> Page Table 768
    
    ;------------------------------------------
    ; Install page directory and enable paging
    ;------------------------------------------
    mov eax, PAGE_DIR
    mov cr3, eax                    ; Load CR3
    
    mov eax, cr0
    or eax, 0x80000000              ; Enable paging bit
    mov cr0, eax
    
    ; *** IMPORTANT: Paging is now ON! ***
    ; Identity mapping allows us to continue executing
    
    ;------------------------------------------
    ; Map 3GB to physical 1MB (kernel location)
    ; This happens AFTER paging is enabled!
    ;------------------------------------------
    mov eax, PAGE_TABLE_768         ; Point to 768th table
    mov ebx, 0x0 | PRIV        ; Physical 1MB
    mov ecx, PAGE_TABLE_ENTRIES
    
.loop2:
    mov dword [eax], ebx            ; Map page
    add eax, 4
    add ebx, 4096
    loop .loop2
    
    ; Flush TLB to ensure new mappings are active
    mov eax, cr3
    mov cr3, eax
    
    popa
    ret