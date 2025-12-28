#ifndef _MMNGR_VIRT_H
#define _MMNGR_VIRT_H	

#include "memory.h"
#include "vmmngr_pte.h"
#include "vmmngr_pde.h"

// virtual address
typedef u32 virtual_addr;

#define PAGES_PER_TABLE 1024
#define PAGES_PER_DIR	1024

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0xfff)

#define PAGE_SIZE 4096

#define P2V(addr) ((u32)((addr) + 0xC0000000))
#define V2P(addr) ((u32)(addr) - 0xC0000000)

// page table
struct ptable {
	pt_entry m_entries[PAGES_PER_TABLE];
};

// page directory
struct pdirectory {
	pd_entry m_entries[PAGES_PER_DIR];
};


// initialize the memory manager
void vmmngr_initialize(u32 memSize);

// allocates a page in physical memory
u32* vmmngr_alloc_page(void);

// frees a page in physical memory
void vmmngr_free_page(pt_entry* e);

// switch to a new page directory
// u32 vmmngr_switch_pdirectory(struct pdirectory*);

u32 vmmngr_switch_pdirectory(u32);


// get current page directory
struct pdirectory* vmmngr_get_directory(void);

// flushes a cached translation lookaside buffer (TLB) entry
void vmmngr_flush_tlb_entry(virtual_addr addr);

void vmmngr_map_page(void* phys, void* virt);

// get page entry from page table
pt_entry* vmmngr_ptable_lookup_entry(struct ptable* p,virtual_addr addr);

// get directory entry from directory table
pd_entry* vmmngr_pdirectory_lookup_entry(struct pdirectory* p, virtual_addr addr);

// unmap a specific page
void vmmngr_unmap_page(void *virt);

#endif
