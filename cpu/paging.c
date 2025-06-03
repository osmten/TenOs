
//****************************************************************************
//**
//**    mmngr_virtual.cpp
//**		-Virtual Memory Manager
//**
//****************************************************************************

//============================================================================
//    IMPLEMENTATION HEADERS
//============================================================================

// #include <string.h>
#include "paging.h"
#include "memory.h"

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

//! page table represents 4mb address space
#define PTABLE_ADDR_SPACE_SIZE 0x400000

//! directory table represents 4gb address space
#define DTABLE_ADDR_SPACE_SIZE 0x100000000

//! page sizes are 4k
#define PAGE_SIZE 4096

//============================================================================
//    IMPLEMENTATION PRIVATE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE STRUCTURES / UTILITY CLASSES
//============================================================================
//============================================================================
//    IMPLEMENTATION REQUIRED EXTERNAL REFERENCES (AVOID)
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

//! current directory table
struct pdirectory*		_cur_directory=0;

//! current page directory base register
u32	_cur_pdbr=0;

//============================================================================
//    INTERFACE DATA
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

inline pt_entry* vmmngr_ptable_lookup_entry (struct ptable* p,virtual_addr addr) {

	if (p)
		return &p->m_entries[ PAGE_TABLE_INDEX (addr) ];
	return 0;
}

inline pd_entry* vmmngr_pdirectory_lookup_entry (struct pdirectory* p, virtual_addr addr) {

	if (p)
		return &p->m_entries[ PAGE_TABLE_INDEX (addr) ];
	return 0;
}

inline u32 vmmngr_switch_pdirectory (struct pdirectory* dir) {

	if (!dir)
		return 0;

	_cur_directory = dir;
	pmmngr_load_PDBR (_cur_pdbr);
	return 1;
}

void vmmngr_flush_tlb_entry (virtual_addr addr) {

#ifdef _MSC_VER
	_asm {
		cli
		invlpg	addr
		sti
	}
#endif
}

struct pdirectory* vmmngr_get_directory () {

	return _cur_directory;
}

u32* vmmngr_alloc_page () {

	//! allocate a free physical frame
	void* p = pmmngr_alloc_block ();
   void *virt = p;
	if (!p)
		return 0;

	//! map it to the page
	// pt_entry_set_frame (e, (u32)p);
	// pt_entry_add_attrib (e, I86_PTE_PRESENT);
	//doesent set WRITE flag...

   // vmmngr_map_page(p, virt);

	return virt;
}

void vmmngr_free_page (pt_entry* e) {

	void* p = (void*)pt_entry_pfn (*e);
	if (p)
		pmmngr_free_block (p);

	pt_entry_del_attrib (e, I86_PTE_PRESENT);
}

void vmmngr_map_page (void* phys, void* virt) {

   //! get page directory
   struct pdirectory* pageDirectory = vmmngr_get_directory ();

   //! get page table
   pd_entry* e = &pageDirectory->m_entries [PAGE_DIRECTORY_INDEX ((u32) virt) ];
   if ( (*e & I86_PTE_PRESENT) != I86_PTE_PRESENT) {

      //! page table not present, allocate it
      struct ptable* table = (struct ptable*) pmmngr_alloc_block ();
      if (!table)
         return;

      //! clear page table
      memory_set (table, 0, sizeof(struct ptable));

      //! create a new entry
      pd_entry* entry =
         &pageDirectory->m_entries [PAGE_DIRECTORY_INDEX ( (u32) virt) ];

      //! map in the table (Can also just do *entry |= 3) to enable these bits
      pd_entry_add_attrib (entry, I86_PDE_PRESENT);
      pd_entry_add_attrib (entry, I86_PDE_WRITABLE);
      pd_entry_set_frame (entry, (u32)table);
   }

   //! get table
   struct ptable* table = (struct ptable*) PAGE_GET_PHYSICAL_ADDRESS ( e );

   //! get page
   pt_entry* page = &table->m_entries [ PAGE_TABLE_INDEX ( (u32) virt) ];

   // ! map it in (Can also do (*page |= 3 to enable..)
   pt_entry_set_frame ( page, (u32) phys);
   pt_entry_add_attrib ( page, I86_PTE_PRESENT);
}

void vmmngr_initialize () {
   //! allocate default page table
   struct ptable* table[5];
   u32 frame = 0, virt = 0, page_directory_addr = 0;
   
   for (int i = 0; i < 5; i++)
   {
      table[i] = (struct ptable*)pmmngr_alloc_block();
       //! clear page table
      memory_set (table[i], 0, sizeof (struct ptable));
   }
   
   // Idenitity mapped
   int count  = 0;
   for(int i = 0; i < 5; i++)
   {
      for (int j=0; j<1024; j++) {

         //! create a new page
         pt_entry page=0;
         pt_entry_add_attrib (&page, I86_PTE_PRESENT);
         pt_entry_set_frame (&page, frame);

         //! ...and add it to the page table
         table[i]->m_entries[j] = page;
         frame+=4096;
         // virt+=4096;
      }
   }

   //! create default directory table
   struct pdirectory* dir = (struct pdirectory*)pmmngr_alloc_blocks(3);
   if (!dir)
      return;

   //! clear directory table and set it as current
   memory_set (dir, 0, sizeof (struct pdirectory));

   for(int i = 0; i < 5; i++)
   {
      //! get first entry in dir table and set it up to point to our table
      pd_entry* entry = &dir->m_entries[PAGE_DIRECTORY_INDEX(page_directory_addr)];
      pd_entry_add_attrib (entry, I86_PDE_PRESENT);
      pd_entry_add_attrib (entry, I86_PDE_WRITABLE);
      pd_entry_set_frame (entry, (u32)table[i]);
      page_directory_addr += 4194304; //incrementing 4MB
   }
   // //! store current PDBR
   _cur_pdbr = (u32)&dir->m_entries;

   // //! switch to our page directory
   vmmngr_switch_pdirectory (dir);

   // //! enable paging
   pmmngr_paging_enable(1);

}

//============================================================================
//    INTERFACE CLASS BODIES
//============================================================================
//****************************************************************************
//**
//**    END[mmngr_virtual.cpp]
//**
//****************************************************************************
