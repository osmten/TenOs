#include "paging.h"

// current directory table
struct pdirectory* _cur_directory = 0;

// current page directory base register
u32 _cur_pdbr = 0;

// kernel page directory
struct pdirectory* kernel_directory = NULL;
u32 kernel_directory_physical = 0;

inline pt_entry* vmmngr_ptable_lookup_entry(struct ptable* p,virtual_addr addr) {
	if (p)
		return &p->m_entries[PAGE_TABLE_INDEX(addr)];

	return 0;
}

inline pd_entry* vmmngr_pdirectory_lookup_entry(struct pdirectory* p, virtual_addr addr) {
	if (p)
		return &p->m_entries[PAGE_DIRECTORY_INDEX(addr)];

	return 0;
}

inline u32 vmmngr_switch_pdirectory(u32 dir_phys) {

    if (!dir_phys)
        return 0;
    
    _cur_pdbr = dir_phys;
    load_PDBR(_cur_pdbr);
    return 1;
}

void vmmngr_flush_tlb_entry(virtual_addr addr) {
    asm volatile (
        "invlpg (%0)"
        :
        : "r" (addr)
        : "memory"
    );
}

struct pdirectory* vmmngr_get_directory() {
	return _cur_directory;
}

u32* vmmngr_alloc_page() {
    // Allocate a free physical frame
    u32 phys = (u32)alloc_memory_block();
    if (!phys)
        return 0;

    void* virt = P2V(phys);
    vmmngr_map_page((void*)phys, virt);
    
    return (u32*)virt;
}


void vmmngr_free_page(pt_entry* e) {

   if (!e)
      return;

	void* p = (void*)pt_entry_pfn(*e);

	if (p)
		free_memory_block(p);

	pt_entry_del_attrib(e, I86_PTE_PRESENT);
}

void vmmngr_map_page(void* phys, void* virt) {

    if (!phys || !virt)
        return;
    
    struct pdirectory* pageDirectory = vmmngr_get_directory();
    pd_entry* e = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32)virt)];
    
    if (!(*e & I86_PTE_PRESENT)) {
        u32 table_phys = (u32)alloc_memory_block();
        struct ptable* table = (struct ptable*)P2V(table_phys);  // Convert to virtual!
        if (!table)
            return;
        
        memset((u8*)table, 0, sizeof(struct ptable));
        
        pd_entry_set_frame(e, table_phys);
        pd_entry_add_attrib(e, I86_PDE_PRESENT);
        pd_entry_add_attrib(e, I86_PDE_WRITABLE);
        pd_entry_add_attrib(e, I86_PDE_USER);
    }
    
    u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(e);
    
    struct ptable* table = (struct ptable*)P2V(table_phys);
    
    pt_entry* page = &table->m_entries[PAGE_TABLE_INDEX((u32)virt)];
    pt_entry_set_frame(page, (u32)phys);
    pt_entry_add_attrib(page, I86_PTE_PRESENT);
    pt_entry_add_attrib(page, I86_PTE_WRITABLE);
    pt_entry_add_attrib(page, I86_PTE_USER);
}

void vmmngr_initialize(u32 total_memory) {
    pr_info("PAGING","=== VMM INITIALIZATION START ===\n");
    
    // Get existing page directory from CR3
    u32 current_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(current_cr3));
    
    kernel_directory_physical = current_cr3;
    kernel_directory = (struct pdirectory*)P2V(current_cr3);
    
    pr_info("VMM", "Using boot PD: phys=0x%x, virt=%x\n",
            kernel_directory_physical, kernel_directory);
    
    if (total_memory > 0x40000000) {
        pr_warn("VMM", "Memory > 1GB, capping at 1GB\n");
        total_memory = 0x40000000;
    }
    
    // Extend mappings beyond 4MB if needed
    pr_info("VMM", "Mapping rest of memory...\n");
    for (u32 phys = 0x400000; phys < total_memory; phys += PAGE_SIZE) {
        u32 virt = 0xC0000000 + phys;
        
        // Check if already mapped
        u32 pd_idx = PAGE_DIRECTORY_INDEX(virt);
        if (kernel_directory->m_entries[pd_idx] & I86_PDE_PRESENT) {
            continue;  // Already mapped by boot
        }
        
        vmmngr_map_page(phys, virt);
    }

    /* Remove identity mapping done at boot time */
    pd_entry* e = &kernel_directory->m_entries[0];
    u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(e);
    struct ptable* table = (struct ptable*)P2V(table_phys);

    for (int i = 0; i < 1024; i++)
    {
        table->m_entries[i] = 0;
    }

    _cur_directory = kernel_directory;
    _cur_pdbr = kernel_directory_physical;
    
    pr_info("PAGING","=== VMM INITIALIZATION COMPLETE ===\n");
}