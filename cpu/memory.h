#ifndef MEM_H
#define MEM_H

#include "types.h"

//! 8 blocks per byte
#define PMMNGR_BLOCKS_PER_BYTE 8

//! block size (4k)
#define PMMNGR_BLOCK_SIZE	4096

//! block alignment
#define PMMNGR_BLOCK_ALIGN	PMMNGR_BLOCK_SIZE

void init_mem_mngr(u32 addr, u32 size);
u32	pmmngr_get_memory_size ();
u32 pmmngr_get_block_count ();
u32 pmmngr_get_use_block_count ();
u32 pmmngr_get_free_block_count ();
u32 pmmngr_get_block_size ();

#endif

