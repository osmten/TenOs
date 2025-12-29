#include "process.h"

struct process *process_list[MAX_NUM_PROCESS] = {NULL};
static int next_pid = 1; // 0 for init process
struct process *current_process = NULL;


/* The below two function are used for the case when the new process is created
** We have no good way to add it's page directory in kernel page directory as
** during creating of a new process we are still using kernel's page directory
** One way was to just add it inside kernel page table and keep adding for new process
** but this could have exhausted kernel Virtual address. So I am using this method 
** temporary map the page inside the kernel VA and unmap it when we can safely switch
** to processe's page directory 
*/

void* temp_map_page(u32 phys_addr) {

    /*TODO: For now using map early function. Need to refacotr the map page code.*/

    vmm_map_page_late(kernel_directory, (void*)TEMP_MAP_ADDR, (void*)phys_addr,
                    I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER);

    __asm__ volatile("invlpg (%0)" :: "r"(TEMP_MAP_ADDR) : "memory");

    return (void*)TEMP_MAP_ADDR;

}

void temp_unmap_page(void* virt_addr) {

    vmmngr_unmap_page((void*)TEMP_MAP_ADDR);
    __asm__ volatile("invlpg (%0)" :: "r"(TEMP_MAP_ADDR) : "memory");
}

// Helper to map pages in current (switched) page directory
static void map_in_current_pd(void *phys, void *virt, u32 flags) {
    // Get current CR3
    u32 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    
    // Temporarily map current PD
    struct pdirectory *pd = (struct pdirectory*)temp_map_page(cr3);
    
    // Map the page
    vmm_map_page_early(pd, virt, phys, flags);
    
    // Unmap PD
    temp_unmap_page(pd);
}


/* In the future add the process-0 (idle) which creates everything
** For Now just minimal intialization.
*/
void process_init()
{
    pr_info("PROC", "Initializing process system.....\n");
    memset(process_list, 0, sizeof(process_list));
    pr_info("PROC", "Process subsytem initialization done...\n");
}

struct process *process_get_current() {
    return current_process;
}

static int add_process(struct process * *proc) {
    for (int i = 0; i < MAX_NUM_PROCESS; i++) {
        if (process_list[i] == NULL) {
            process_list[i] = proc;
            return i;
        }
    }
    return -1;
}

struct process *process_create(const char *name, void *code, int code_size) {
    printk("Creating process: %s\n", name);
    
    struct process *proc = (struct process *)alloc_memory_block();
    if (!proc) {
        pr_err("PROC","Failed to allocate process structure\n");
        return NULL;
    }
    memset(proc, 0, sizeof(struct process));
    
    proc->pid = next_pid++;
    int size_name = strlen(name);
    memcpy(proc->name, name, size_name);
    proc->name[size_name] = '\0';
    proc->state = PROCESS_READY;
    proc->priority = 1;
    proc->time_slice = 10;
    
    // Allocate kernel stack
    proc->kernel_stack = (u32)alloc_memory_blocks(2);
    if (!proc->kernel_stack) {
        printk("Failed to allocate kernel stack\n");
        return NULL;
    }
    
    // Allocate page directory
    u32 pd_phys = (u32)alloc_memory_block();
    if (!pd_phys) {
        pr_err("PROC","Failed to allocate page directory\n");
        return NULL;
    }
    proc->page_dir_phys = pd_phys;
    
    // Initialize page directory
    struct pdirectory *proc_pd = (struct pdirectory*)temp_map_page(pd_phys);
    memset(proc_pd, 0, PAGE_SIZE);
    
    // Copy kernel mappings (768-1023) - your way was correct!
    for (int i = 768; i < 1024; i++) {
        proc_pd->m_entries[i] = kernel_directory->m_entries[i];
    }
    
    // OPTIONAL: Copy identity mapping for first 4MB if you need it
    // (Only needed if kernel code uses low addresses)
    for (int i = 0; i < 4; i++) {  // First 4 PD entries = 4MB
        proc_pd->m_entries[i] = kernel_directory->m_entries[i];
    }
    
    // Set up memory regions
    u32 num_code_pages = (code_size + PAGE_SIZE - 1) / PAGE_SIZE;
    proc->code_start = 0x00001000;
    proc->code_end = proc->code_start + (num_code_pages * PAGE_SIZE);
    proc->stack_start = 0xC0000000;  // This overlaps with kernel!
    proc->stack_end = 0xBFFFF000;     // Should be much lower, like 0x7FFFF000
    proc->heap_start = proc->code_end;
    proc->heap_end = proc->heap_start;
    
    // Map code pages
    for (u32 i = 0; i < num_code_pages; i++) {
        u32 phys = (u32)alloc_memory_block();
        u32 virt = proc->code_start + (i * PAGE_SIZE);
        
        u32 pd_idx = PAGE_DIRECTORY_INDEX(virt);
        u32 pt_idx = PAGE_TABLE_INDEX(virt);
        
        if (!(proc_pd->m_entries[pd_idx] & I86_PDE_PRESENT)) {
            u32 pt_phys = (u32)alloc_memory_block();
            struct ptable *pt = (struct ptable*)P2V(pt_phys);
            memset(pt, 0, sizeof(struct ptable));
            
            proc_pd->m_entries[pd_idx] = pt_phys | I86_PDE_PRESENT | 
                                         I86_PDE_WRITABLE | I86_PDE_USER;
        }
        
        u32 pt_phys = proc_pd->m_entries[pd_idx] & ~0xFFF;
        struct ptable *pt = (struct ptable*)P2V(pt_phys);
        pt->m_entries[pt_idx] = phys | I86_PTE_PRESENT | 
                                I86_PTE_WRITABLE | I86_PTE_USER;
    }
    
    // Map stack (FIXED ADDRESS - don't overlap with kernel!)
    proc->stack_start = 0x80000000;  // 2GB
    proc->stack_end = 0x7FFFF000;    // Just below 2GB
    
    u32 stack_phys = (u32)alloc_memory_block();
    u32 pd_idx = PAGE_DIRECTORY_INDEX(proc->stack_end);
    u32 pt_idx = PAGE_TABLE_INDEX(proc->stack_end);
    
    if (!(proc_pd->m_entries[pd_idx] & I86_PDE_PRESENT)) {
        u32 pt_phys = (u32)alloc_memory_block();
        struct ptable *pt = (struct ptable*)P2V(pt_phys);
        memset(pt, 0, sizeof(struct ptable));
        proc_pd->m_entries[pd_idx] = pt_phys | I86_PDE_PRESENT | 
                                     I86_PDE_WRITABLE | I86_PDE_USER;
    }
    
    u32 pt_phys = proc_pd->m_entries[pd_idx] & ~0xFFF;
    struct ptable *pt = (struct ptable*)P2V(pt_phys);
    pt->m_entries[pt_idx] = stack_phys | I86_PTE_PRESENT | 
                            I86_PTE_WRITABLE | I86_PTE_USER;
    
    temp_unmap_page(proc_pd);
    
    // Save current CR3
    u32 old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    
    // Switch to process PD with PROPER PIPELINE FLUSH
    printk("  Switching to process PD (0x%x -> 0x%x)\n", old_cr3, pd_phys);
    __asm__ volatile(
        "mov %0, %%cr3\n\t"
        "jmp 1f\n\t"          // Flush instruction pipeline
        "1:\n\t"
        "nop"
        :: "r"(pd_phys)
        : "memory"
    );
    printk("  Switched successfully!\n");
    
    // Copy code
    for (u32 i = 0; i < num_code_pages; i++) {
        u32 virt = proc->code_start + (i * PAGE_SIZE);
        int copy_size = (i == num_code_pages - 1 && code_size % PAGE_SIZE) ?
                       code_size % PAGE_SIZE : PAGE_SIZE;
        
        memcpy((void*)virt, (u8*)code + (i * PAGE_SIZE), copy_size);
        if (copy_size < PAGE_SIZE) {
            memset((u8*)virt + copy_size, 0, PAGE_SIZE - copy_size);
        }
    }
    
    memset((void*)proc->stack_end, 0, PAGE_SIZE);
    
    // Switch back with pipeline flush
    __asm__ volatile(
        "mov %0, %%cr3\n\t"
        "jmp 1f\n\t"
        "1:\n\t"
        "nop"
        :: "r"(old_cr3)
        : "memory"
    );
    printk("  Switched back to kernel\n");
    
    // Initialize registers
    memset(&proc->regs, 0, sizeof(proc_registers_t));
    proc->regs.eip = proc->code_start;
    proc->regs.esp = proc->stack_start;
    proc->regs.user_esp = proc->stack_start;
    proc->regs.eflags = 0x202;
    proc->regs.cs = 0x1B;
    proc->regs.ss = 0x23;
    proc->regs.ds = 0x23;
    proc->regs.es = 0x23;
    proc->regs.fs = 0x23;
    proc->regs.gs = 0x23;
    
    if (add_process(proc) < 0) {
        printk("Failed to add process to list\n");
        return NULL;
    }
    
    printk("Process created successfully: PID=%u\n", proc->pid);
    return proc;
}

// struct process *process_create(const char *name, void *code, int code_size) {
//     printk("Creating process: %s\n", name);
    
//     struct process *proc = (struct process *)alloc_memory_block();
//     if (!proc) {
//         pr_err("PROC","Failed to allocate process structure\n");
//         return NULL;
//     }
//     memset(proc, 0, sizeof(struct process));
    
//     proc->pid = next_pid++;
//     int size_name = strlen(name);
//     memcpy(proc->name, name, size_name);
//     proc->name[size_name] = '\0';
//     proc->state = PROCESS_READY;
//     proc->priority = 1;
//     proc->time_slice = 10;
    
//     // Allocate kernel stack
//     proc->kernel_stack = (u32)alloc_memory_blocks(2);
//     if (!proc->kernel_stack) {
//         printk("Failed to allocate kernel stack\n");
//         return NULL;
//     }
    
//     // Allocate page directory
//     u32 pd_phys = (u32)alloc_memory_block();
//     if (!pd_phys) {
//         pr_err("PROC","Failed to allocate page directory\n");
//         return NULL;
//     }
//     proc->page_dir_phys = pd_phys;
//     printk("  Page directory: phys=0x%x\n", pd_phys);
    
//     // Initialize page directory via temp mapping
//     struct pdirectory *proc_pd = (struct pdirectory*)temp_map_page(pd_phys);
//     memset(proc_pd, 0, PAGE_SIZE);
    
//     // **CRITICAL FIX**: Copy kernel mappings from the PHYSICAL kernel directory
//     // We need to use kernel_directory_physical, not kernel_directory
//     struct pdirectory *kernel_pd_temp = (struct pdirectory*)temp_map_page(kernel_directory_physical);
    
//     for (int i = 768; i < 1024; i++) {
//         proc_pd->m_entries[i] = kernel_pd_temp->m_entries[i];
//     }
    
//     temp_unmap_page(kernel_pd_temp);
//     printk("  Page directory initialized\n");
    
//     // Set up memory regions
//     u32 num_code_pages = (code_size + PAGE_SIZE - 1) / PAGE_SIZE;
//     proc->code_start = 0x00001000;
//     proc->code_end = proc->code_start + (num_code_pages * PAGE_SIZE);
//     proc->stack_start = 0xC0000000;
//     proc->stack_end = 0xBFFFF000;
//     proc->heap_start = proc->code_end;
//     proc->heap_end = proc->heap_start;
//     printk("  Page directory initialized\n");
    
//     // Map code pages
//     for (u32 i = 0; i < num_code_pages; i++) {
//         u32 phys = (u32)alloc_memory_block();
//         u32 virt = proc->code_start + (i * PAGE_SIZE);
        
//         u32 pd_idx = PAGE_DIRECTORY_INDEX(virt);
//         u32 pt_idx = PAGE_TABLE_INDEX(virt);
        
//         if (!(proc_pd->m_entries[pd_idx] & I86_PDE_PRESENT)) {
//             u32 pt_phys = (u32)alloc_memory_block();
//             struct ptable *pt = (struct ptable*)P2V(pt_phys);
//             memset(pt, 0, sizeof(struct ptable));
            
//             proc_pd->m_entries[pd_idx] = pt_phys | I86_PDE_PRESENT | 
//                                          I86_PDE_WRITABLE | I86_PDE_USER;
//         }
        
//         u32 pt_phys = proc_pd->m_entries[pd_idx] & ~0xFFF;
//         struct ptable *pt = (struct ptable*)P2V(pt_phys);
//         pt->m_entries[pt_idx] = phys | I86_PTE_PRESENT | 
//                                 I86_PTE_WRITABLE | I86_PTE_USER;
//     }
//     printk("  Page directory initialized\n");
    
//     // Map stack
//     u32 stack_phys = (u32)alloc_memory_block();
//     u32 pd_idx = PAGE_DIRECTORY_INDEX(proc->stack_end);
//     u32 pt_idx = PAGE_TABLE_INDEX(proc->stack_end);
//     printk("  Page directory initialized\n");
    
//     if (!(proc_pd->m_entries[pd_idx] & I86_PDE_PRESENT)) {
//         u32 pt_phys = (u32)alloc_memory_block();
//         struct ptable *pt = (struct ptable*)P2V(pt_phys);
//         memset(pt, 0, sizeof(struct ptable));
//         proc_pd->m_entries[pd_idx] = pt_phys | I86_PDE_PRESENT | 
//                                      I86_PDE_WRITABLE | I86_PDE_USER;
//     }
//     printk("  Page directory initialized\n");
    
//     u32 pt_phys = proc_pd->m_entries[pd_idx] & ~0xFFF;
//     struct ptable *pt = (struct ptable*)P2V(pt_phys);
//     pt->m_entries[pt_idx] = stack_phys | I86_PTE_PRESENT | 
//                             I86_PTE_WRITABLE | I86_PTE_USER;

//     printk("  Page directory initialized\n");
    
    
//     temp_unmap_page(proc_pd);
//     printk("  Page directory initialized\n");
    
//     // Save current CR3
//     u32 old_cr3;
//     __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    
//     // Switch to process PD - this should now work!
//     printk("  Switching to process PD (0x%x -> 0x%x)\n", old_cr3, pd_phys);
//     __asm__ volatile("mov %0, %%cr3" :: "r"(pd_phys) : "memory");
//     printk("  Switched successfully!\n");
    
//     // Copy code and zero stack...
//     for (u32 i = 0; i < num_code_pages; i++) {
//         u32 virt = proc->code_start + (i * PAGE_SIZE);
//         int copy_size = (i == num_code_pages - 1 && code_size % PAGE_SIZE) ?
//                        code_size % PAGE_SIZE : PAGE_SIZE;
        
//         memcpy((void*)virt, (u8*)code + (i * PAGE_SIZE), copy_size);
//         if (copy_size < PAGE_SIZE) {
//             memset((u8*)virt + copy_size, 0, PAGE_SIZE - copy_size);
//         }
//     }
    
//     memset((void*)proc->stack_end, 0, PAGE_SIZE);
    
//     // Switch back
//     __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
//     printk("  Switched back to kernel\n");
    
//     // Initialize registers...
//     memset(&proc->regs, 0, sizeof(proc_registers_t));
//     proc->regs.eip = proc->code_start;
//     proc->regs.esp = proc->stack_start;
//     proc->regs.user_esp = proc->stack_start;
//     proc->regs.eflags = 0x202;
//     proc->regs.cs = 0x1B;
//     proc->regs.ss = 0x23;
//     proc->regs.ds = 0x23;
//     proc->regs.es = 0x23;
//     proc->regs.fs = 0x23;
//     proc->regs.gs = 0x23;
    
//     if (add_process(proc) < 0) {
//         printk("Failed to add process to list\n");
//         return NULL;
//     }
    
//     printk("Process created successfully: PID=%u\n", proc->pid);
//     return proc;
// }

// struct process *process_create(const char *name, void *code, int code_size) {

//     printk("Creating process: %s\n", name);
    
//     struct process *proc = (struct process *)alloc_memory_block();

//     if (!proc) {
//         pr_err("PROC","Failed to allocate process structure\n");
//         return NULL;
//     }

//     memset(proc, 0, sizeof(struct process));
    
//     int size_name = 0;
//     size_name = strlen(name);

//     // 2. Set basic info
//     proc->pid = next_pid++;
    
//     memcpy(proc->name, name, size_name);
//     proc->name[size_name] = '\0';
//     proc->state = PROCESS_READY;
//     proc->priority = 1;
//     proc->time_slice = 10;
    
//     proc->kernel_stack = (u32)alloc_memory_blocks(2);
//     if (!proc->kernel_stack) {
//         // kfree(proc);
//         printk("Failed to allocate kernel stack\n");
//         return NULL;
//     }
//     printk("  Kernel stack: 0x%x - 0x%x\n", 
//            proc->kernel_stack, proc->kernel_stack + KERNEL_STACK_SIZE);
    
//     u32 pd_phys = (u32)alloc_memory_block();

//     if (!pd_phys) {
//         pr_err("PROC","Failed to allocate page directory\n");
//         return NULL;
//     }

//     proc->page_dir_phys = pd_phys;
//     proc->page_dir = NULL;
    
//     printk("  Page directory: phys=0x%x\n", pd_phys);
    
//     // 5. Initialize page directory via temp mapping
//     struct pdirectory *proc_pd = (struct pdirectory*)temp_map_page(pd_phys);
//     memset(proc_pd, 0, PAGE_SIZE);
    
//     printk("  Page directory: phys=0x%x\n", pd_phys);


//     // Copy kernel mappings (768-1023)
//     for (int i = 768; i < 1024; i++) {
//         proc_pd->m_entries[i] = kernel_directory->m_entries[i];
//     }

//         u32 num_code_pages = (code_size + PAGE_SIZE - 1) / PAGE_SIZE;
//     proc->code_start = 0x00001000;
//     proc->code_end = proc->code_start + (num_code_pages * PAGE_SIZE);
    
//     for (u32 i = 0; i < num_code_pages; i++) {
//         u32 phys = (u32)alloc_memory_block();
//         u32 virt = proc->code_start + (i * PAGE_SIZE);
        
//         // Map directly in proc_pd (which is temp-mapped)
//         vmm_map_page_early(proc_pd, virt, phys,
//                           I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER);
//     }
    
//     // Map stack
//     u32 stack_phys = (u32)alloc_memory_block();
//     vmm_map_page_early(proc_pd, proc->stack_end, stack_phys,
//                       I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER);
    

//     temp_unmap_page(proc_pd);
//     printk("  Page directory initialized\n");
    
//     // 6. Save current CR3
//     u32 old_cr3;
//     __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    
//     // 7. Switch to process's page directory
//     __asm__ volatile("mov %0, %%cr3" :: "r"(pd_phys) : "memory");
//     printk("  Switched to process page directory\n");
    
//   /*  // 8. Map process code
//     u32 num_code_pages = (code_size + PAGE_SIZE - 1) / PAGE_SIZE;
//     proc->code_start = 0x00001000;
//     proc->code_end = proc->code_start + (num_code_pages * PAGE_SIZE);
    
//     printk("  Mapping %u pages for code (0x%x - 0x%x)\n",
//            num_code_pages, proc->code_start, proc->code_end);
    
//     for (u32 i = 0; i < num_code_pages; i++) {
//         u32 phys = (u32)alloc_memory_block();
//         if (!phys) {
//             printk("Failed to allocate code page %u\n", i);
//             // TODO: cleanup
//             return NULL;
//         }
        
//         u32 virt = proc->code_start + (i * PAGE_SIZE);
        
//         // Map in current (process's) page directory
//         map_in_current_pd((void*)phys, (void*)virt,
//                         I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER);
        
//         // Copy code
//         int copy_size = PAGE_SIZE;
//         if (i == num_code_pages - 1) {
//             int remainder = code_size % PAGE_SIZE;
//             if (remainder) copy_size = remainder;
//         }
        
//         memcpy((void*)virt, (u8*)code + (i * PAGE_SIZE), copy_size);
        
//         // Zero rest if partial page
//         if (copy_size < PAGE_SIZE) {
//             memset((u8*)virt + copy_size, 0, PAGE_SIZE - copy_size);
//         }
//     }
    
//     printk("  Code mapped and copied\n");
    
//     // 9. Map stack (one page for now)
//     proc->stack_start = 0xC0000000;
//     proc->stack_end = 0xBFFFF000;
    
//     u32 stack_phys = (u32)alloc_memory_block();
//     if (!stack_phys) {
//         printk("Failed to allocate stack\n");
//         // TODO: cleanup
//         return NULL;
//     }
    
//     map_in_current_pd((void*)stack_phys, (void*)proc->stack_end,
//                     I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER);
    
//     memset((void*)proc->stack_end, 0, PAGE_SIZE);
//     printk("  Stack mapped at 0x%x\n", proc->stack_end);
//     */
//       for (u32 i = 0; i < num_code_pages; i++) {
//         u32 virt = proc->code_start + (i * PAGE_SIZE);
//         int copy_size = (i == num_code_pages - 1 && code_size % PAGE_SIZE) ?
//                        code_size % PAGE_SIZE : PAGE_SIZE;
        
//         memcpy((void*)virt, (u8*)code + (i * PAGE_SIZE), copy_size);
        
//         if (copy_size < PAGE_SIZE) {
//             memset((u8*)virt + copy_size, 0, PAGE_SIZE - copy_size);
//         }
//     }
    
//     // Zero stack
//     memset((void*)proc->stack_end, 0, PAGE_SIZE);
    
//     // 10. Set up heap
//     proc->heap_start = proc->code_end;
//     proc->heap_end = proc->heap_start;
    
//     // 11. Switch back to kernel page directory
//     __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
//     printk("  Switched back to kernel\n");
    
//     // 12. Initialize registers for first run
//     memset(&proc->regs, 0, sizeof(proc_registers_t));
//     proc->regs.eip = proc->code_start;
//     proc->regs.esp = proc->stack_start;
//     proc->regs.user_esp = proc->stack_start;
//     proc->regs.eflags = 0x202;  // IF (interrupts enabled)
    
//     // User mode segments (RPL=3)
//     proc->regs.cs = 0x1B;  // 0x18 | 3
//     proc->regs.ss = 0x23;  // 0x20 | 3
//     proc->regs.ds = 0x23;
//     proc->regs.es = 0x23;
//     proc->regs.fs = 0x23;
//     proc->regs.gs = 0x23;
    
//     // 13. Add to process list
//     if (add_process(proc) < 0) {
//         printk("Failed to add process to list\n");
//         // TODO: cleanup
//         return NULL;
//     }
    
//     printk("Process created successfully: PID=%u\n", proc->pid);
//     return proc;
// }

// void process_run(struct process *proc) {
//     if (!proc) return;
    
//     printk("Running process: PID=%u, name=%s\n", proc->pid, proc->name);
    
//     current_process = proc;
//     proc->state = PROCESS_RUNNING;
    
//     // Switch to process's page directory
//     __asm__ volatile("mov %0, %%cr3" :: "r"(proc->page_dir_phys) : "memory");
    
//     // Jump to user mode
//     jump_usermode_proc(&proc->regs);
// }

// void process_run(struct process *proc) {
//     if (!proc) return;
    
//     printk("Running process: PID=%u, name=%s\n", proc->pid, proc->name);
//     printk("  EIP: 0x%x\n", proc->regs.eip);
//     printk("  ESP: 0x%x\n", proc->regs.esp);
//     printk("  CS: 0x%x, SS: 0x%x, DS: 0x%x\n", 
//            proc->regs.cs, proc->regs.ss, proc->regs.ds);
//     printk("  EFLAGS: 0x%x\n", proc->regs.eflags);
    
//     // Verify the code is mapped
//     __asm__ volatile("mov %0, %%cr3" :: "r"(proc->page_dir_phys));
    
//     // Try to read first instruction
//     u8 first_byte = *(u8*)proc->code_start;
//     printk("  First byte at 0x%x: 0x%x\n", proc->code_start, first_byte);
    
//     current_process = proc;
//     proc->state = PROCESS_RUNNING;
    
//     // Jump to user mode
//     jump_usermode_proc(&proc->regs);
// }

void process_run(struct process *proc) {
    if (!proc) return;
    
    printk("\n========== PROCESS RUN DEBUG ==========\n");
    printk("Process: PID=%u, name=%s\n", proc->pid, proc->name);
    printk("Kernel stack: 0x%x - 0x%x\n", 
           proc->kernel_stack, proc->kernel_stack + KERNEL_STACK_SIZE);
    printk("Page dir phys: 0x%x\n", proc->page_dir_phys);
    printk("Code: 0x%x - 0x%x\n", proc->code_start, proc->code_end);
    printk("Stack: 0x%x - 0x%x\n", proc->stack_start, proc->stack_end);
    
    printk("\nRegisters:\n");
    printk("  EIP: 0x%x\n", proc->regs.eip);
    printk("  ESP: 0x%x\n", proc->regs.esp);
    printk("  EBP: 0x%x\n", proc->regs.ebp);
    printk("  EFLAGS: 0x%x\n", proc->regs.eflags);
    printk("  CS: 0x%x  SS: 0x%x\n", proc->regs.cs, proc->regs.ss);
    printk("  DS: 0x%x  ES: 0x%x\n", proc->regs.ds, proc->regs.es);
    
    // Check TR before running
    u16 tr;
    __asm__ volatile("str %0" : "=r"(tr));
    printk("\nTask Register: 0x%x (should be 0x28)\n", tr);
    
    if (tr == 0) {
        printk("ERROR: TSS not loaded!\n");
        return;
    }
    
    // Update TSS
    tss_set_kernel_stack(proc->kernel_stack + KERNEL_STACK_SIZE);
    // printk("TSS.esp0 updated to: 0x%x\n", tss.esp0);
    // printk("TSS.ss0: 0x%x\n", tss.ss0);
    
    // Verify first instruction
    __asm__ volatile("mov %0, %%cr3" :: "r"(proc->page_dir_phys) : "memory");
    u8 *code = (u8*)proc->code_start;
    printk("\nFirst 8 bytes of code at 0x%x:\n", proc->code_start);
    for (int i = 0; i < 8; i++) {
        printk("  [%d]: 0x%02x\n", i, code[i]);
    }
    
    printk("\n========== JUMPING TO USER MODE ==========\n\n");
    
    current_process = proc;
    proc->state = PROCESS_RUNNING;
    
    jump_usermode_proc(&proc->regs);
}