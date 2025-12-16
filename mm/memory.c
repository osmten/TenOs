#include "memory.h"

static u32 *mem_mngr = 0;
static u32 mem_mngr_max_blocks = 0;
static u32 mem_mngr_used_blocks = 0;
static u32 mem_mngr_size = 0;

void init_mem_mngr(u32 addr, u32 size)
{
	mem_mngr_size			=	size;
	mem_mngr	    		=	(u32*)addr;
	mem_mngr_max_blocks		=	(get_memory_size() * 1024) / MEMORY_BLOCK_SIZE;
	mem_mngr_used_blocks	=	mem_mngr_max_blocks;

	// By default, all of memory is in use
	memset((u8*)mem_mngr, 0xf, get_block_count() / MEMORY_BLOCKS_PER_BYTE );
}

void init_memory_region(u32 base, u32 size) {

	int align = base / MEMORY_BLOCK_SIZE;
	int blocks = size / MEMORY_BLOCK_SIZE;

	for (; blocks>=0; blocks--) {
		mmap_unset(align++);
		mem_mngr_used_blocks--;
	}

	mmap_set(0);
}

// set any bit (frame) within the memory map bit array
void mmap_set(int bit) {
  mem_mngr[bit / 32] |= (1 << (bit % 32));
}

// unset any bit (frame) within the memory map bit array
void mmap_unset(int bit) {
  mem_mngr[bit / 32] &= ~(1 << (bit % 32));
}

// test if any bit (frame) is set within the memory map bit array
int mmap_test(int bit) {
	return mem_mngr[bit / 32] &  (1 << (bit % 32));
}

// finds first free frame in the bit array and returns its index
int mmap_first_free() {
	for (u32 i = 0; i < get_block_count() / 32; i++)
	{
		if (mem_mngr[i] != 0xffffffff)
			for (int j=0; j<32; j++) {
				int bit = 1 << j;
				if (!(mem_mngr[i] & bit))
					return i*4*8+j;
			}
	}

	return -1;
}

// finds first free "size" number of frames and returns its index
int mmap_first_free_s(u32 size) {

	if (size == 0)
		return -1;

	if (size == 1)
		return mmap_first_free();

	for (u32 i = 0; i < get_block_count() / 32; i++)
	{
		if (mem_mngr[i] != 0xffffffff)
		{
			for (int j = 0; j < 32; j++) {
				int bit = 1<<j;
				if (!(mem_mngr[i] & bit)) {
					int starting_bit = i*32;
					starting_bit += j;

					u32 free = 0; //loop through each bit to see if its enough space
					for (u32 count = 0; count <= size; count++) {
						if (!mmap_test(starting_bit + count))
							free++;

						if (free == size)
							return i*4*8+j;
					}
				}
			}
		}
	}

	return -1;
}


void* alloc_memory_block() {

	if (get_free_block_count() <= 0)
		return 0;	//out of memory

	int frame = mmap_first_free();

	if (frame == -1)
		return 0;	//out of memory

	mmap_set(frame);

	u32 addr = frame * MEMORY_BLOCK_SIZE;
	mem_mngr_used_blocks++;

	return (void*)addr;
}


void free_memory_block(void* p) {

	u32 addr = (u32)p;
	int frame = addr / MEMORY_BLOCK_SIZE;

	mmap_unset(frame);
	mem_mngr_used_blocks--;
}

void* alloc_memory_blocks(u32 size) {

	if (get_free_block_count() <= size)
		return 0;	//not enough space

	int frame = mmap_first_free_s(size);

	if (frame == -1)
		return 0;	//not enough space

	for (u32 i = 0; i < size; i++)
		mmap_set(frame + i);

	u32 addr = frame * MEMORY_BLOCK_SIZE;
	mem_mngr_used_blocks += size;

	return (void*)addr;
}

void free_memory_blocks(void* p, u32 size) {

	u32 addr = (u32)p;
	int frame = addr / MEMORY_BLOCK_SIZE;

	for (u32 i = 0; i < size; i++)
		mmap_unset(frame + i);

	mem_mngr_used_blocks-=size;
}

/*--------------Helper Functions---------------*/
static u32	get_memory_size() {
	return mem_mngr_size;
}

static u32 get_block_count() {
	return mem_mngr_max_blocks;
}

static u32 get_use_block_count() {
	return mem_mngr_used_blocks;
}

static u32 get_free_block_count() {
	return mem_mngr_max_blocks - mem_mngr_used_blocks;
}

void pmmngr_paging_enable(u32 b) {
    asm volatile (
        "mov %%cr0, %%eax\n\t"
        "cmp $1, %0\n\t"
        "je 1f\n\t"
        "and $0x7FFFFFFF, %%eax\n\t"  // Clear PG bit (bit 31)
        "jmp 2f\n"
        "1:\n\t"
        "or $0x80000000, %%eax\n\t"   // Set PG bit (bit 31)
        "2:\n\t"
        "mov %%eax, %%cr0"
        : /* no outputs */
        : "r" (b)
        : "eax", "memory"
    );
}

u32 is_paging() {
    u32 res;
    asm volatile (
        "mov %%cr0, %0"
        : "=r" (res)
        : /* no inputs */
        : "memory"
    );
    return (res & 0x80000000) ? 1 : 0;  // Fixed logic (returns 1 if paging enabled)
}

void load_PDBR(u32 addr) {
    asm volatile (
        "mov %0, %%cr3"
        : /* no outputs */
        : "r" (addr)
        : "memory"
    );
}

u32 get_PDBR() {
    u32 val;
    asm volatile (
        "mov %%cr3, %0"
        : "=r" (val)
        : /* no inputs */
        : "memory"
    );
    return val;
}