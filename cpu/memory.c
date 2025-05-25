#include "memory.h"
#include "types.h"

static u32 *mem_mngr = 0;
static u32 mem_mngr_max_blocks = 0;
static u32 mem_mngr_used_blocks = 0;
static u32 mem_mngr_size = 0;


void init_mem_mngr(u32 addr, u32 size)
{

	mem_mngr_size	=	size;
	mem_mngr	    =	(u32*) addr;
	mem_mngr_max_blocks	=	(pmmngr_get_memory_size()*1024) / PMMNGR_BLOCK_SIZE;
	mem_mngr_used_blocks	=	mem_mngr_max_blocks;

	//! By default, all of memory is in use
	memory_set(mem_mngr, 0xf, pmmngr_get_block_count() / PMMNGR_BLOCKS_PER_BYTE );
}

void pmmngr_init_region (u32 base, u32 size) {

	int align = base / PMMNGR_BLOCK_SIZE;
	int blocks = size / PMMNGR_BLOCK_SIZE;

	for (; blocks>=0; blocks--) {
		mmap_unset (align++);
		mem_mngr_used_blocks--;
	}

	mmap_set (0);	//first block is always set. This insures allocs cant be 0
}

//! set any bit (frame) within the memory map bit array
inline void mmap_set (int bit) {

  mem_mngr[bit / 32] |= (1 << (bit % 32));
}

//! unset any bit (frame) within the memory map bit array
inline void mmap_unset (int bit) {

  mem_mngr[bit / 32] &= ~ (1 << (bit % 32));
}

//! test if any bit (frame) is set within the memory map bit array
inline int mmap_test (int bit) {

	return mem_mngr[bit / 32] &  (1 << (bit % 32));
}


//! finds first free frame in the bit array and returns its index
int mmap_first_free () {

	//! find the first free bit
	for (u32 i=0; i< pmmngr_get_block_count() /32; i++)
		if (mem_mngr[i] != 0xffffffff)
			for (int j=0; j<32; j++) {				//! test each bit in the dword

				int bit = 1 << j;
				if (! (mem_mngr[i] & bit) )
					return i*4*8+j;
			}

	return -1;
}

//! finds first free "size" number of frames and returns its index
int mmap_first_free_s (u32 size) {

	if (size==0)
		return -1;

	if (size==1)
		return mmap_first_free ();

	for (u32 i=0; i<pmmngr_get_block_count() /32; i++)
		if (mem_mngr[i] != 0xffffffff)
			for (int j=0; j<32; j++) {	//! test each bit in the dword

				int bit = 1<<j;
				if (! (mem_mngr[i] & bit) ) {

					int startingBit = i*32;
					startingBit+=bit;		//get the free bit in the dword at index i

					u32 free=0; //loop through each bit to see if its enough space
					for (u32 count=0; count<=size;count++) {

						if (! mmap_test (startingBit+count) )
							free++;	// this bit is clear (free frame)

						if (free==size)
							return i*4*8+j; //free count==size needed; return index
					}
				}
			}

	return -1;
}


u32	pmmngr_get_memory_size () {

	return mem_mngr_size;
}

u32 pmmngr_get_block_count () {

	return mem_mngr_max_blocks;
}

u32 pmmngr_get_use_block_count () {

	return mem_mngr_used_blocks;
}

u32 pmmngr_get_free_block_count () {

	return mem_mngr_max_blocks - mem_mngr_used_blocks;
}

u32 pmmngr_get_block_size () {

	return PMMNGR_BLOCK_SIZE;
}
