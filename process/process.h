#ifndef PROCESS_H
#define PROCESS_H

#include <lib/lib.h>
#include <mm/paging.h>
#include <kernel/kernel.h>

#define MAX_NUM_PROCESS 10
#define KERNEL_STACK_SIZE 8192

/* To temporary map the page directiory of new process in kernel map*/
#define TEMP_MAP_ADDR 0xFFFFF000

extern struct pdirectory *kernel_directory;
extern u32 kernel_directory_physical;

typedef enum {
    PROCESS_READY,      // Ready to run
    PROCESS_RUNNING,    // Currently executing
    PROCESS_BLOCKED,    // Waiting for I/O or event
    PROCESS_TERMINATED  // Finished execution
} process_state_t;

// Saved register state (for context switching)
typedef struct {
    // Segment registers
    u32 gs, fs, es, ds;
    
    // General purpose registers (pushed by pusha)
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    
    // Interrupt frame (pushed by CPU)
    u32 eip;
    u32 cs;
    u32 eflags;
    u32 user_esp;
    u32 ss;
} proc_registers_t;

// Process Control Block (PCB)
struct process {
    u32 pid;                        // Process ID
    char name[32];                  // Process name
    
    process_state_t state;          // Current state
    
    // Memory management
    struct pdirectory *page_dir;    // Virtual address of page directory
    u32 page_dir_phys;              // Physical address of page directory
    
    // Virtual memory regions
    u32 code_start;                 // Start of code segment (e.g., 0x00001000)
    u32 code_end;                   // End of code segment
    u32 data_start;                 // Start of data segment
    u32 data_end;                   // End of data segment
    u32 heap_start;                 // Start of heap
    u32 heap_end;                   // Current end of heap (grows up)
    u32 stack_start;                // Start of stack (e.g., 0xC0000000)
    u32 stack_end;                  // Current stack pointer (grows down)
    
    // Saved context (for context switching)
    proc_registers_t regs;               // Saved register state
    u32 kernel_stack;               // Kernel stack pointer for this process
    
    // Scheduling
    u32 priority;                   // Process priority
    u32 time_slice;                 // Remaining time slice
    
    // Parent/child relationships
    struct process *parent;         // Parent process
    struct process *next;           // Next process in list
    
};

#endif