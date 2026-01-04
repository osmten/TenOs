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

    u32 boot_pd_phys;
    __asm__ volatile("mov %%cr3, %0" : "=r"(boot_pd_phys));
    struct pdirectory* boot_pd = (struct pdirectory*)P2V(boot_pd_phys);

    pr_info("VMM", "Boot PD: phys=0x%x, virt=0x%x\n", boot_pd_phys, (u32)boot_pd);

    if (total_memory > 0x40000000) {
        pr_warn("VMM", "Memory > 1GB, capping at 1GB\n");
        total_memory = 0x40000000;
    }

    u32 new_pd_phys = (u32)alloc_memory_block();
    if (!new_pd_phys) {
        pr_err("VMM", "Failed to allocate new page directory!\n");
        return;
    }

    struct pdirectory* new_pd = (struct pdirectory*)P2V(new_pd_phys);
    memset((u8*)new_pd, 0, sizeof(struct pdirectory));

    pr_info("VMM", "Allocated new PD: phys=0x%x, virt=0x%x\n",
            new_pd_phys, (u32)new_pd);

    pr_info("VMM", "Copying boot mappings to new PD...\n");

    for (int i = 0; i < 1024; i++) {
        pd_entry* boot_pde = &boot_pd->m_entries[i];

        if (!(*boot_pde & I86_PDE_PRESENT))
            continue;

        u32 new_table_phys = (u32)alloc_memory_block();
        struct ptable* new_table = (struct ptable*)P2V(new_table_phys);
        memset((u8*)new_table, 0, sizeof(struct ptable));

        // Get old table
        u32 old_table_phys = PAGE_GET_PHYSICAL_ADDRESS(boot_pde);
        struct ptable* old_table = (struct ptable*)P2V(old_table_phys);

        for (int j = 0; j < 1024; j++) {
            new_table->m_entries[j] = old_table->m_entries[j];
        }

        pd_entry_set_frame(&new_pd->m_entries[i], new_table_phys);
        pd_entry_add_attrib(&new_pd->m_entries[i], I86_PDE_PRESENT);
        pd_entry_add_attrib(&new_pd->m_entries[i], I86_PDE_WRITABLE);
        pd_entry_add_attrib(&new_pd->m_entries[i], I86_PDE_USER);

        pr_debug("VMM", "Copied PD[%d]: old_table=0x%x, new_table=0x%x\n",
                i, old_table_phys, new_table_phys);
    }

    pr_info("VMM", "Switching to new page directory...\n");

    kernel_directory = new_pd;
    kernel_directory_physical = new_pd_phys;
    _cur_directory = new_pd;
    _cur_pdbr = new_pd_phys;

    vmmngr_switch_pdirectory(new_pd_phys);

    pr_info("VMM", "Successfully switched to new PD!\n");

    pr_info("VMM", "Extending memory mappings...\n");

    for (u32 phys = 0x400000; phys < total_memory; phys += PAGE_SIZE) {
        u32 virt = 0xC0000000 + phys;

        u32 pd_idx = PAGE_DIRECTORY_INDEX(virt);
        pd_entry* pde = &kernel_directory->m_entries[pd_idx];

        if (*pde & I86_PDE_PRESENT) {
            u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(pde);
            struct ptable* table = (struct ptable*)P2V(table_phys);
            u32 pt_idx = PAGE_TABLE_INDEX(virt);

            if (table->m_entries[pt_idx] & I86_PTE_PRESENT)
                continue;
        }

        vmmngr_map_page((void*)phys, (void*)virt);
    }

    pr_info("VMM", "Removing identity mapping...\n");

    pd_entry* identity_pde = &kernel_directory->m_entries[0];
    if (*identity_pde & I86_PDE_PRESENT) {
        u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(identity_pde);
        struct ptable* table = (struct ptable*)P2V(table_phys);

        // Clear all entries
        for (int i = 0; i < 1024; i++) {
            table->m_entries[i] = 0;
        }

        // Clear PDE
        *identity_pde = 0;

        // Flush TLB for identity mapped region
        // for (u32 addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        //     vmmngr_flush_tlb_entry(addr);
        // }

        // __asm__ volatile(
        // "mov %%cr3, %%eax\n"   // Read CR3
        // "mov %%eax, %%cr3\n"   // Write it back
        // ::: "eax"
        // );

        pr_info("VMM", "Identity mapping removed\n");
    }

    pr_info("PAGING","=== VMM INITIALIZATION COMPLETE ===\n");
    pr_info("VMM", "Kernel PD at 0x%x (phys: 0x%x)\n", 
            (u32)kernel_directory, kernel_directory_physical);
}