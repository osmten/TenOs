[bits 32]
[extern main] ; Define calling point. Must have same name as kernel.c 'main' function
mov ebx, 0xB8000
mov byte [ebx], 'B' ; should show "X" on screen if protected mode works
call main ; Calls the C function. The linker will know where it is placed in memory
jmp $
