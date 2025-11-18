#ifndef _MMNGR_VIRT_H
#define _MMNGR_VIRT_H
//****************************************************************************
//**
//**    mmngr_virtual.h
//**		-Virtual Memory Manager
//**
//****************************************************************************
//============================================================================
//    INTERFACE REQUIRED HEADERS
//============================================================================

#include <stdint.h>
#include "vmmngr_pte.h"
#include "vmmngr_pde.h"

//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

//! virtual address
typedef u32 virtual_addr;

//! i86 architecture defines 1024 entries per table--do not change
#define PAGES_PER_TABLE 1024
#define PAGES_PER_DIR	1024

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0xfff)

//============================================================================
//    INTERFACE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    INTERFACE STRUCTURES / UTILITY CLASSES
//============================================================================

//! page table
struct ptable {

	pt_entry m_entries[PAGES_PER_TABLE];
};

//! page directory
struct pdirectory {

	pd_entry m_entries[PAGES_PER_DIR];
};

//============================================================================
//    INTERFACE DATA DECLARATIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

//! maps phys to virtual address
// extern void MmMapPage (void* phys, void* virt);

//! initialize the memory manager
void vmmngr_initialize (void);

//! allocates a page in physical memory
u32* vmmngr_alloc_page (void);

//! frees a page in physical memory
void vmmngr_free_page (pt_entry* e);

//! switch to a new page directory
u32 vmmngr_switch_pdirectory (struct pdirectory*);

//! get current page directory
struct pdirectory* vmmngr_get_directory (void);

//! flushes a cached translation lookaside buffer (TLB) entry
void vmmngr_flush_tlb_entry (virtual_addr addr);
void vmmngr_map_page (void* phys, void* virt);

//! clears a page table
// extern void vmmngr_ptable_clear (struct ptable* p);

//! convert virtual address to page table index
// extern u32 vmmngr_ptable_virt_to_index (virtual_addr addr);

//! get page entry from page table
pt_entry* vmmngr_ptable_lookup_entry (struct ptable* p,virtual_addr addr);

//! convert virtual address to page directory index
// extern u32 vmmngr_pdirectory_virt_to_index (virtual_addr addr);

//! clears a page directory table
// extern void vmmngr_pdirectory_clear (struct pdirectory* dir);

//! get directory entry from directory table
pd_entry* vmmngr_pdirectory_lookup_entry (struct pdirectory* p, virtual_addr addr);

//============================================================================
//    INTERFACE OBJECT CLASS DEFINITIONS
//============================================================================
//============================================================================
//    INTERFACE TRAILING HEADERS
//============================================================================
//****************************************************************************
//**
//**    END [mmngr_virtual.h]
//**
//****************************************************************************

#endif
