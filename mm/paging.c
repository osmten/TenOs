#include "paging.h"

#define P2V(addr) ((u32)((addr) + 0xC0000000))
#define V2P(addr) ((u32)(addr) - 0xC0000000)

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

// inline u32 vmmngr_switch_pdirectory(struct pdirectory* dir) {

// 	if (!dir)
// 		return 0;

// 	_cur_directory = dir;
//    _cur_pdbr = (u32)&dir->m_entries;

// 	load_PDBR(_cur_pdbr);

// 	return 1;
// }

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

// u32* vmmngr_alloc_page() {

// 	// allocate a free physical frame
// 	void* p = alloc_memory_block();
//    void *virt = p;

// 	if (!p)
// 		return 0;

//    vmmngr_map_page(p, virt);

// 	return virt;
// }

u32* vmmngr_alloc_page() {
    // Allocate a free physical frame
    u32 phys = (u32)alloc_memory_block();
    if (!phys)
        return 0;
    
    // For now, just return virtual address
    // (You'd normally specify where to map it)
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

// void vmmngr_map_page(void* phys, void* virt) {

//    if (!phys || !virt)
//       return;

//    struct pdirectory* pageDirectory = vmmngr_get_directory();

//    pd_entry* e = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32)virt)];

//    if (!(*e & I86_PTE_PRESENT)) {

//       // page table not present, allocate it
//       struct ptable* table = (struct ptable*)alloc_memory_block();

//       if (!table)
//          return;

//       // clear page table
//       memset((u8*)table, 0, sizeof(struct ptable));

//       // create a new entry
//       pd_entry* entry =
//          &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32)virt)];

//       // map in the table (Can also just do *entry |= 3) to enable these bits
//       pd_entry_add_attrib(entry, I86_PDE_PRESENT);
//       pd_entry_add_attrib(entry, I86_PDE_WRITABLE);
//       pd_entry_add_attrib(entry, I86_PDE_USER);
//       pd_entry_set_frame(entry, (u32)table);
//    }

//    struct ptable* table = (struct ptable*)PAGE_GET_PHYSICAL_ADDRESS(e);

//    pt_entry* page = &table->m_entries[PAGE_TABLE_INDEX((u32)virt)];

//    // map it in (Can also do (*page |= 3 to enable..)
//    pt_entry_set_frame( page, (u32) phys);
//    pt_entry_add_attrib( page, I86_PTE_PRESENT);
//    pt_entry_add_attrib(page, I86_PTE_WRITABLE);
//    pt_entry_add_attrib(page, I86_PTE_USER);  
// }

void vmmngr_map_page(void* phys, void* virt) {
    if (!phys || !virt)
        return;
    
    struct pdirectory* pageDirectory = vmmngr_get_directory();
    pd_entry* e = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32)virt)];

    printk("Phys is %x, virt is %x\n", phys, virt);
    
    if (!(*e & I86_PTE_PRESENT)) {
        printk("Not mapped\n");
        // Allocate page table (returns physical address)
        u32 table_phys = (u32)alloc_memory_block();
        struct ptable* table = (struct ptable*)P2V(table_phys);  // Convert to virtual!
        
        if (!table)
            return;
        
        // Clear page table (via virtual address)
        memset((u8*)table, 0, sizeof(struct ptable));
        
        // Set PDE (store physical address)
        pd_entry_set_frame(e, table_phys);
        pd_entry_add_attrib(e, I86_PDE_PRESENT);
        pd_entry_add_attrib(e, I86_PDE_WRITABLE);
        pd_entry_add_attrib(e, I86_PDE_USER);
    }
    
    // Get page table physical address
    u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(e);
    
    // Convert to virtual address to access it
    struct ptable* table = (struct ptable*)P2V(table_phys);
    
    // Set page table entry
    pt_entry* page = &table->m_entries[PAGE_TABLE_INDEX((u32)virt)];
    pt_entry_set_frame(page, (u32)phys);
    pt_entry_add_attrib(page, I86_PTE_PRESENT);
    pt_entry_add_attrib(page, I86_PTE_WRITABLE);
    pt_entry_add_attrib(page, I86_PTE_USER);
}

void vmm_map_page_early(struct pdirectory *dir, u32 virt, 
                       u32 phys, u32 flags) 
{
   u32 pd_index = PAGE_DIRECTORY_INDEX(virt);
   u32 pt_index = PAGE_TABLE_INDEX(virt);
   
   // Debug first page only
   static int first_page = 1;
   if (first_page) {
       printk("First mapping: virt=0x%x, pd_index=%u, pt_index=%u\n", 
              virt, pd_index, pt_index);
       if (virt == 0xC0000000 && pd_index != 768) {
           printk("ERROR: Macro broken! Expected pd_index=768, got %u\n", pd_index);
       }
       first_page = 0;
   }
   
   pd_entry* e = &dir->m_entries[pd_index];  // Use calculated index

   if (!(*e & I86_PDE_PRESENT)) {
      // page table not present, allocate it
      struct ptable* table = (struct ptable*)alloc_memory_block();

      if (!table)
         return;

      // clear page table
      memset((u8*)table, 0, sizeof(struct ptable));

      // Use the SAME 'e' pointer, don't recalculate!
      *e = (u32)table | flags;
      // pd_entry_set_frame(e, (u32)table);   // ← Use 'e', not 'entry'!
   }

   // Get page table
   u32 table_phys = PAGE_GET_PHYSICAL_ADDRESS(e);
   struct ptable* table = (struct ptable*)table_phys;

   // Map the page
   pt_entry* page = &table->m_entries[pt_index];  // Use calculated index
   pt_entry_set_frame(page, phys);
   *page |= flags;
   // printk("PTE is %x\n", *page);
}

// void vmm_map_page_early(struct pdirectory *dir, u32 virt, 
//                        u32 phys, u32 flags) 
// {
//    pd_entry* e = &dir->m_entries[PAGE_DIRECTORY_INDEX(virt)];

//    if (virt == 0xC0000000 && PAGE_DIRECTORY_INDEX(virt) != 768) {
//         pr_err("PAGING", "ERROR: Macro broken! virt=0xC0000000 but pd_index=%u (should be 768)\n", PAGE_DIRECTORY_INDEX(virt));
//     }
    

//    if (!(*e & I86_PDE_PRESENT)) {
//       // page table not present, allocate it
//       struct ptable* table = (struct ptable*)alloc_memory_block();

//       if (!table)
//          return;

//       // clear page table
//       memset((u8*)table, 0, sizeof(struct ptable));

//       // create a new entry
//       pd_entry* entry =
//          &dir->m_entries[PAGE_DIRECTORY_INDEX((u32)virt)];

//       // map in the table (Can also just do *entry |= 3) to enable these bits
//       // *entry |= flags;
//       pd_entry_set_frame(e, (u32)table);  
//       pd_entry_add_attrib(e, I86_PDE_PRESENT);
//       pd_entry_add_attrib(e, I86_PDE_WRITABLE);
//       pd_entry_add_attrib(e, I86_PDE_USER); 
//    }

//    struct ptable* table = (struct ptable*)PAGE_GET_PHYSICAL_ADDRESS(e);

//    pt_entry* page = &table->m_entries[PAGE_TABLE_INDEX((u32)virt)];
//    pt_entry_set_frame(page, (u32)phys);
//    *page |= flags;

// }

void vmmngr_initialize(u32 total_memory) 
{
   int count = 0;
   total_memory *= 1024;
    printk("=== VMM INITIALIZATION START ===\n");
    printk("VMM: total_memory = 0x%x (%u MB)\n",
            total_memory, total_memory / (1024*1024));
    
    // Step 1: Allocate kernel page directory
    printk("VMM: Allocating kernel page directory...\n");
    kernel_directory_physical = (u32)alloc_memory_block();
    
    if (!kernel_directory_physical) {
        printk("ERROR: Failed to allocate kernel page directory!\n");
        return;
    }
    
    printk("VMM: Kernel directory at physical 0x%x\n", kernel_directory_physical);
    
    // Step 2: Clear it
    kernel_directory = (struct pdirectory*)kernel_directory_physical;
    printk("VMM: Clearing page directory...\n");
    memset(kernel_directory, 0, sizeof(struct pdirectory));
    printk("VMM: Page directory cleared\n");
    
    // Step 3: Cap at 1GB
    if (total_memory > 0x40000000) {
        printk("VMM: Memory > 1GB, capping at 1GB\n");
        total_memory = 0x40000000;
    }
    
    u32 num_pages = total_memory / PAGE_SIZE;
    printk("VMM: Will map %u pages (%u MB)\n", num_pages, total_memory / (1024*1024));
    
    // Step 4: Map all physical RAM to 0xC0000000+
    printk("VMM: Starting mapping loop...\n");
    u32 pages_mapped = 0;

    printk("VMM: Identity mapping first 4MB...\n");
    for (u32 phys = 0; phys < 0x400000; phys += PAGE_SIZE) {
        vmm_map_page_early(kernel_directory, phys, phys,  // virt = phys
                          I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PDE_USER);
    }
    printk("VMM: Identity mapping complete\n");
    
    
    for (u32 phys = 0; phys < total_memory; phys += PAGE_SIZE)
    {
        u32 virt = 0xC0000000 + phys;
        
        // Debug every 1000th page
        if (pages_mapped % 1000 == 0) {
            printk("  Mapping page %u: phys 0x%x -> virt 0x%x\n", 
                   pages_mapped, phys, virt);
        }
        
        vmm_map_page_early(kernel_directory, virt, phys, 
                          I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PDE_USER);
        pages_mapped++;
    }
    
    printk("VMM: Mapped %u pages successfully\n", pages_mapped);
    
    // Step 5: Verify page directory has entries
    u32 pde_count = 0;
    for (int i = 0; i < 1024; i++) {
        if (kernel_directory->m_entries[i] & I86_PDE_PRESENT) {
            pde_count++;
            if (pde_count <= 5) {
                printk("  PDE[%d] = 0x%x\n", i, kernel_directory->m_entries[i]);
            }
        }
    }
    printk("VMM: Total PDEs present: %u\n", pde_count);
    
    // Step 6: Switch to kernel directory
    printk("VMM: Switching to page directory (phys 0x%x)...\n", 
           kernel_directory_physical);
    vmmngr_switch_pdirectory(kernel_directory_physical);
    printk("VMM: Page directory loaded into CR3\n");
    
    // Step 7: Verify CR3
    u32 cr3_value;
    asm volatile("mov %%cr3, %0" : "=r"(cr3_value));
    printk("VMM: CR3 = 0x%x (should be 0x%x)\n", cr3_value, kernel_directory_physical);
    
    // Step 8: Enable paging
    printk("VMM: Enabling paging...\n");
    pmmngr_paging_enable(1);

    // Step 9: Verify paging is enabled
    u32 cr0_value;
    asm volatile("mov %%cr0, %0" : "=r"(cr0_value));
    printk("VMM: CR0 = 0x%x (PG bit = %d)\n", cr0_value, !!(cr0_value & 0x80000000));
    
    if (!(cr0_value & 0x80000000)) {
        printk("ERROR: Paging not enabled!\n");
        return;
    }
    
    printk("VMM: Paging enabled successfully!\n");
    
    asm volatile("hlt");
    // Step 10: Test virtual address access
   //  printk("VMM: Testing virtual address access...\n");
    volatile u32 *test_ptr = (volatile u32*)0xC0100000;
    u32 test_value = *test_ptr;
    printk("VMM: Read from 0xC0100000 = 0x%x\n", test_value);
    
    // Step 11: Update to virtual addresses
   //  printk("VMM: Updating pointers to virtual addresses...\n");
    _cur_directory = (struct pdirectory*)P2V(kernel_directory_physical);
    kernel_directory = _cur_directory;
    
    printk("VMM: kernel_directory now at virtual 0x%x\n", (u32)kernel_directory);
   //  printk("=== VMM INITIALIZATION COMPLETE ===\n");
}

// void vmmngr_initialize(u32 total_memory) 
// {
//    kernel_directory_physical = (u32)alloc_memory_block();
//    kernel_directory = (struct pdirectory*)kernel_directory_physical;

//    memset(kernel_directory, 0, sizeof(struct pdirectory));

//    for (u32 phys = 0; phys < total_memory; phys += PAGE_SIZE)
//    {
//       u32 virt = 0xC0000000 + phys;

//         vmm_map_page_early(kernel_directory, virt, phys, 
//                           I86_PTE_PRESENT | I86_PTE_WRITABLE);
//    }
//    vmmngr_switch_pdirectory(kernel_directory_physical);
//    pmmngr_paging_enable(1);
   

//    _cur_directory = (struct pdirectory*)P2V(kernel_directory_physical);
//    kernel_directory = _cur_directory;
// }

// void vmmngr_initialize() {

//    struct ptable* table[30];
//    u32 frame = 0, virt = 0, page_directory_addr = 0;
   
//    for (int i = 0; i < 30; i++)
//    {
//       table[i] = (struct ptable*)alloc_memory_block();
//       memset((u8*)table[i], 0, sizeof(struct ptable)); // clear page table
//    }
   
//    // Idenitity mapped
//    int count  = 0;
//    for (int i = 0; i < 30; i++)
//    {
//       for (int j=0; j<1024; j++) {
//          // create a new page
//          pt_entry page = 0;
//          pt_entry_add_attrib(&page, I86_PTE_PRESENT);
//          pt_entry_add_attrib(&page, I86_PTE_USER);
//          pt_entry_add_attrib(&page, I86_PTE_WRITABLE);
//          pt_entry_set_frame(&page, frame);

//          table[i]->m_entries[j] = page;
//          frame+=4096;
//       }
//    }

//    struct pdirectory* dir = (struct pdirectory*)alloc_memory_block();

//    if (!dir)
//       return;

//    memset((u8*)dir, 0, sizeof(struct pdirectory));

//    for (int i = 0; i < 30; i++)
//    {
//       // get first entry in dir table and set it up to point to our table
//       pd_entry* entry = &dir->m_entries[PAGE_DIRECTORY_INDEX(page_directory_addr)];
//       pd_entry_add_attrib(entry, I86_PDE_PRESENT);
//       pd_entry_add_attrib(entry, I86_PDE_USER);
//       pd_entry_add_attrib(entry, I86_PDE_WRITABLE);
//       pd_entry_set_frame(entry, (u32)table[i]);
//       page_directory_addr += 4194304; //incrementing 4MB
//    }

//    vmmngr_switch_pdirectory(dir);

//    pmmngr_paging_enable(1);

// }