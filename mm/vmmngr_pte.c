#include "vmmngr_pte.h"

inline void pt_entry_add_attrib (pt_entry* e, u32 attrib) {
	*e |= attrib;
}

inline void pt_entry_del_attrib (pt_entry* e, u32 attrib) {
	*e &= ~attrib;
}

inline void pt_entry_set_frame (pt_entry* e, u32 addr) {
	*e = (*e & ~I86_PTE_FRAME) | addr;
}

inline u32 pt_entry_is_present (pt_entry e) {
	return e & I86_PTE_PRESENT;
}

inline u32 pt_entry_is_writable (pt_entry e) {
	return e & I86_PTE_WRITABLE;
}

inline u32 pt_entry_pfn (pt_entry e) {
	return e & I86_PTE_FRAME;
}

