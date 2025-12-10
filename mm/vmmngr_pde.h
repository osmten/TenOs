#ifndef _MMNGR_VIRT_PDE_H
#define _MMNGR_VIRT_PDE_H

#include "memory.h"	//physical_addr

enum PAGE_PDE_FLAGS {

	I86_PDE_PRESENT			=	1,			//0000000000000000000000000000001
	I86_PDE_WRITABLE		=	2,			//0000000000000000000000000000010
	I86_PDE_USER			=	4,			//0000000000000000000000000000100
	I86_PDE_PWT				=	8,			//0000000000000000000000000001000
	I86_PDE_PCD				=	0x10,		//0000000000000000000000000010000
	I86_PDE_ACCESSED		=	0x20,		//0000000000000000000000000100000
	I86_PDE_DIRTY			=	0x40,		//0000000000000000000000001000000
	I86_PDE_4MB				=	0x80,		//0000000000000000000000010000000
	I86_PDE_CPU_GLOBAL		=	0x100,		//0000000000000000000000100000000
	I86_PDE_LV4_GLOBAL		=	0x200,		//0000000000000000000001000000000
   	I86_PDE_FRAME			=	0x7FFFF000 	//1111111111111111111000000000000
};

//! a page directery entry
typedef u32 pd_entry;

//! sets a flag in the page table entry
void pd_entry_add_attrib (pd_entry* e, u32 attrib);

//! clears a flag in the page table entry
void pd_entry_del_attrib (pd_entry* e, u32 attrib);

//! sets a frame to page table entry
void pd_entry_set_frame (pd_entry*, u32);

//! test if page is present
u32 pd_entry_is_present (pd_entry e);

//! test if directory is user mode
u32 pd_entry_is_user (pd_entry);

//! test if directory contains 4mb pages
u32 pd_entry_is_4mb (pd_entry);

//! test if page is writable
u32 pd_entry_is_writable (pd_entry e);

//! get page table entry frame address
u32 pd_entry_pfn (pd_entry e);

//! enable global pages
void pd_entry_enable_global (pd_entry e);

#endif
