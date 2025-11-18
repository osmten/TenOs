#ifndef MEM_H
#define MEM_H

#include<lib/lib.h>

//! 8 blocks per byte
#define PMMNGR_BLOCKS_PER_BYTE 8

//! block size (4k)
#define PMMNGR_BLOCK_SIZE	4096

//! block alignment
#define PMMNGR_BLOCK_ALIGN	PMMNGR_BLOCK_SIZE

void init_mem_mngr(u32 addr, u32 size);
u32	pmmngr_get_memory_size(void);
u32 pmmngr_get_block_count (void);
u32 pmmngr_get_use_block_count (void);
u32 pmmngr_get_free_block_count (void);
u32 pmmngr_get_block_size (void);
void pmmngr_init_region (u32 base, u32 size);
void* pmmngr_alloc_blocks (u32 size);
void* pmmngr_alloc_block (void);
void pmmngr_paging_enable(u32 b);
void pmmngr_load_PDBR(u32 addr);
void pmmngr_free_blocks (void* p, u32 size);
void pmmngr_free_block(void* p);

#endif

