#include "vmmngr_pde.h"

inline void pd_entry_add_attrib (pd_entry* e, u32 attrib) {
	*e |= attrib;
}

inline void pd_entry_del_attrib (pd_entry* e, u32 attrib) {
	*e &= ~attrib;
}

inline void pd_entry_set_frame (pd_entry* e, u32 addr) {
	*e = (*e & ~I86_PDE_FRAME) | addr;
}

inline u32 pd_entry_is_present (pd_entry e) {
	return e & I86_PDE_PRESENT;
}

inline u32 pd_entry_is_writable (pd_entry e) {
	return e & I86_PDE_WRITABLE;
}

inline u32 pd_entry_pfn (pd_entry e) {
	return e & I86_PDE_FRAME;
}

inline u32 pd_entry_is_user (pd_entry e) {
	return e & I86_PDE_USER;
}

inline u32 pd_entry_is_4mb (pd_entry e) {
	return e & I86_PDE_4MB;
}
