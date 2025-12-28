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
void vmmngr_unmap_page(void *virt)
{
    struct pdirectory* pageDirectory = vmmngr_get_directory();
    pd_entry* e = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32)virt)];
    
    u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(e);
    struct ptable* table = (struct ptable*)P2V(table_phys);
    pt_entry* page = &table->m_entries[PAGE_TABLE_INDEX((u32)virt)];

    pt_entry_del_attrib(page, I86_PTE_PRESENT);
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

void vmm_map_page_late(struct pdirectory *dir, u32 virt, 
                       u32 phys, u32 flags) 
{
   u32 pd_index = PAGE_DIRECTORY_INDEX(virt);
   u32 pt_index = PAGE_TABLE_INDEX(virt);
   
   pd_entry* e = &dir->m_entries[pd_index];
   
   if (!(*e & I86_PDE_PRESENT)) {
      // Allocate page table
      u32 table_phys = (u32)alloc_memory_block();
      if (!table_phys)
         return;
      
      // Access via higher half mapping
      struct ptable* table = (struct ptable*)P2V(table_phys);
      memset((u8*)table, 0, sizeof(struct ptable));
      
      // Store physical address in PD entry
      *e = table_phys | flags;
   }
   
   // Get page table physical address
   u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(e);
   
   // Access via higher half mapping
   struct ptable* table = (struct ptable*)P2V(table_phys);
   
   pt_entry* page = &table->m_entries[pt_index];
   pt_entry_set_frame(page, phys);
   *page |= flags;
}


void vmm_map_page_early(struct pdirectory *dir, u32 virt, 
                       u32 phys, u32 flags) 
{
   u32 pd_index = PAGE_DIRECTORY_INDEX(virt);
   u32 pt_index = PAGE_TABLE_INDEX(virt);
   
   pd_entry* e = &dir->m_entries[pd_index];

   if (!(*e & I86_PDE_PRESENT)) {
      struct ptable* table = (struct ptable*)alloc_memory_block();

      if (!table)
         return;

      memset((u8*)table, 0, sizeof(struct ptable));

      *e = (u32)table | flags;
   }

   u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(e);
   struct ptable* table = (struct ptable*)table_phys;

   pt_entry* page = &table->m_entries[pt_index];
   pt_entry_set_frame(page, phys);
   *page |= flags;
}

void vmmngr_initialize(u32 total_memory) 
{
    pr_info("PAGING","=== VMM INITIALIZATION START ===\n");
    pr_debug("PAGING","VMM: total_memory = 0x%x (%u MB)\n",
            total_memory, total_memory / (1024*1024));
    
    kernel_directory_physical = (u32)alloc_memory_block();
    
    pr_info("VMM", "Kernel Directory Physical Address is at %x\n", kernel_directory_physical);

    if (!kernel_directory_physical) {
        pr_err("PAGING","Failed to allocate kernel page directory!\n");
        return;
    }
        
    kernel_directory = (struct pdirectory*)kernel_directory_physical;
    memset(kernel_directory, 0, sizeof(struct pdirectory));
    
    if (total_memory > 0x40000000) {
        pr_warn("VMM", "Memory > 1GB, capping at 1GB\n");
        total_memory = 0x40000000;
    }
    
    pr_info("VMM", "Identity Mapping first 4MB\n");
    for (u32 phys = 0; phys < 0x400000; phys += PAGE_SIZE) {
        vmm_map_page_early(kernel_directory, phys, phys,
                          I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PDE_USER);
    }
    
    for (u32 phys = 0; phys < total_memory; phys += PAGE_SIZE) {
        u32 virt = 0xC0000000 + phys;        
        vmm_map_page_early(kernel_directory, virt, phys, 
                          I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PDE_USER);
    }
    
    vmmngr_switch_pdirectory(kernel_directory_physical);
    pmmngr_paging_enable(1);

    _cur_directory = (struct pdirectory*)P2V(kernel_directory_physical);
    kernel_directory = _cur_directory;
    
    pr_info("PAGING","=== VMM INITIALIZATION COMPLETE ===\n");
}