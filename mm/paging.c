#include "paging.h"

// current directory table
struct pdirectory* _cur_directory = 0;

// current page directory base register
u32 _cur_pdbr = 0;

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

inline u32 vmmngr_switch_pdirectory(struct pdirectory* dir) {

	if (!dir)
		return 0;

	_cur_directory = dir;
   _cur_pdbr = (u32)&dir->m_entries;

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

	// allocate a free physical frame
	void* p = alloc_memory_block();
   void *virt = p;

	if (!p)
		return 0;

   vmmngr_map_page(p, virt);

	return virt;
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

      // page table not present, allocate it
      struct ptable* table = (struct ptable*)alloc_memory_block();

      if (!table)
         return;

      // clear page table
      memory_set((u8*)table, 0, sizeof(struct ptable));

      // create a new entry
      pd_entry* entry =
         &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((u32)virt)];

      // map in the table (Can also just do *entry |= 3) to enable these bits
      pd_entry_add_attrib(entry, I86_PDE_PRESENT);
      pd_entry_add_attrib(entry, I86_PDE_WRITABLE);
      pd_entry_add_attrib(entry, I86_PDE_USER);
      pd_entry_set_frame(entry, (u32)table);
   }

   struct ptable* table = (struct ptable*)PAGE_GET_PHYSICAL_ADDRESS(e);

   pt_entry* page = &table->m_entries[PAGE_TABLE_INDEX((u32)virt)];

   // map it in (Can also do (*page |= 3 to enable..)
   pt_entry_set_frame( page, (u32) phys);
   pt_entry_add_attrib( page, I86_PTE_PRESENT);
   pt_entry_add_attrib(page, I86_PTE_WRITABLE);
   pt_entry_add_attrib(page, I86_PTE_USER);  
}

void vmmngr_initialize() {

   struct ptable* table[5];
   u32 frame = 0, virt = 0, page_directory_addr = 0;
   
   for (int i = 0; i < 5; i++)
   {
      table[i] = (struct ptable*)alloc_memory_block();
      memory_set((u8*)table[i], 0, sizeof (struct ptable)); // clear page table
   }
   
   // Idenitity mapped
   int count  = 0;
   for (int i = 0; i < 5; i++)
   {
      for (int j=0; j<1024; j++) {
         // create a new page
         pt_entry page = 0;
         pt_entry_add_attrib(&page, I86_PTE_PRESENT);
         pt_entry_add_attrib(&page, I86_PTE_USER);
         pt_entry_add_attrib(&page, I86_PTE_WRITABLE);
         pt_entry_set_frame(&page, frame);

         table[i]->m_entries[j] = page;
         frame+=4096;
      }
   }

   struct pdirectory* dir = (struct pdirectory*)alloc_memory_block();

   if (!dir)
      return;

   memory_set((u8*)dir, 0, sizeof (struct pdirectory));

   for (int i = 0; i < 5; i++)
   {
      // get first entry in dir table and set it up to point to our table
      pd_entry* entry = &dir->m_entries[PAGE_DIRECTORY_INDEX(page_directory_addr)];
      pd_entry_add_attrib(entry, I86_PDE_PRESENT);
      pd_entry_add_attrib(entry, I86_PDE_USER);
      pd_entry_add_attrib(entry, I86_PDE_WRITABLE);
      pd_entry_set_frame(entry, (u32)table[i]);
      page_directory_addr += 4194304; //incrementing 4MB
   }

   vmmngr_switch_pdirectory(dir);

   pmmngr_paging_enable(1);

}