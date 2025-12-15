#ifndef MEM_H
#define MEM_H

#include<lib/lib.h>

#define MEMORY_BLOCKS_PER_BYTE 8
#define MEMORY_BLOCK_SIZE	4096
#define PMMNGR_BLOCK_ALIGN	MEMORY_BLOCK_SIZE

void init_mem_mngr(u32 addr, u32 size);
static u32	get_memory_size(void);
static u32 get_block_count(void);
static u32 get_use_block_count(void);
static u32 get_free_block_count(void);

void init_memory_region(u32 base, u32 size);
void* alloc_memory_blocks(u32 size);
void* alloc_memory_block(void);
void pmmngr_paging_enable(u32 b);
void load_PDBR(u32 addr);
void free_memory_blocks(void* p, u32 size);
void free_memory_block(void* p);

void mmap_set(int bit);
void mmap_unset(int bit);
int mmap_test(int bit);
int mmap_first_free();
int mmap_first_free_s(u32 size);

#endif

