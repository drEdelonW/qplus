#include "zone.h"
#include "zone_prv.h"

#include "q_tools.h"
#include "sys.h"
#include "console.h"


#define	ZONEID      (0x1D4A11)
#define MINFRAGMENT (64)

memzone_p  mainzone;

/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/



/*
	========================
	Z_ClearZone
	========================
*/
void Z_ClearZone(memzone_p zone, size_t size) {
	// set the entire zone to one free block

	memblock_p block = zone->blocklist.next = zone->blocklist.prev =
		(memblock_p)((uint8_p)zone + sizeof(memzone_t));
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(memzone_t);
}


/*
	========================
	Z_Free
	========================
*/
void Z_Free(typeless_ptr ptr) {
	if (!ptr)
		Sys_Error("Z_Free: NULL pointer");

	memblock_p block = (memblock_p)((uint8_p)ptr - sizeof(memblock_t));
	if (block->id != ZONEID)
		Sys_Error("Z_Free: freed a pointer without ZONEID");
	if (block->tag == 0)
		Sys_Error("Z_Free: freed a freed pointer");

	block->tag = 0;		// mark as free

	memblock_p other = block->prev;
	if (!other->tag) {	// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if (block == mainzone->rover)
			mainzone->rover = other;
		block = other;
	}

	other = block->next;
	if (!other->tag) {	// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if (other == mainzone->rover)
			mainzone->rover = block;
	}
}


/*
	========================
	Z_Malloc
	========================
*/
typeless_ptr Z_Malloc(size_t size) {
	Z_CheckHeap();	// DEBUG
	typeless_ptr buf = Z_TagMalloc(size, 1);
	if (!buf)
		Sys_Error("Z_Malloc: failed on allocation of %i bytes", size);
	Q_memset(buf, 0, size);

	return buf;
}

typeless_ptr Z_TagMalloc(size_t size, int tag) {
	if (!tag)
		Sys_Error("Z_TagMalloc: tried to use a 0 tag");

	//
	// scan through the block list looking for the first free block
	// of sufficient size
	//
	size += sizeof(memblock_t);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = ALIGN_UP(size, 8);	// align to 8-byte boundary

	memblock_p rover;
	memblock_p base = rover = mainzone->rover;
	memblock_p start = base->prev;

	do {
		if (rover == start)	// scaned all the way around the list
			return NULL;
		if (rover->tag)
			base = rover = rover->next;
		else
			rover = rover->next;
	} while (base->tag || base->size < size);

	//
	// found a block big enough
	//
	int extra = base->size - size;
	if (extra > MINFRAGMENT) {	// there will be a free fragment after the allocated block
		memblock_p new = (memblock_p)((uint8_p)base + size);
		new->size = extra;
		new->tag = 0;			// free block
		new->prev = base;
		new->id = ZONEID;
		new->next = base->next;
		new->next->prev = new;
		base->next = new;
		base->size = size;
	}

	base->tag = tag;				// no longer a free block

	mainzone->rover = base->next;	// next allocation will start looking here

	base->id = ZONEID;

	// marker for memory trash testing
	*(uint32_p)((uint8_p)base + base->size - sizeof(uint32_t)) = ZONEID;

	return (typeless_ptr)((uint8_p)base + sizeof(memblock_t));
}


/*
	========================
	Z_Print
	========================
*/
void Z_Print(memzone_p zone) {
	Con_Printf(
		"zone size: %i  location: %p\n",
		mainzone->size, mainzone
	);

	for (memblock_p block = zone->blocklist.next; ; block = block->next) {
		Con_Printf("block:%p    size:%7i    tag:%3i\n",
			block, block->size, block->tag
		);

		if (block->next == &zone->blocklist)
			break;			// all blocks have been hit
		if (((uint8_p)block + block->size) != (uint8_p)block->next)
			Con_Printf("ERROR: block size does not touch the next block\n");
		if (block->next->prev != block)
			Con_Printf("ERROR: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			Con_Printf("ERROR: two consecutive free blocks\n");
	}
}


/*
	========================
	Z_CheckHeap
	========================
*/
void Z_CheckHeap(void) {
	for (memblock_p block = mainzone->blocklist.next; ; block = block->next) {
		if (block->next == &mainzone->blocklist)
			break;			// all blocks have been hit
		if (((uint8_p)block + block->size) != (uint8_p)block->next)
			Sys_Error("Z_CheckHeap: block size does not touch the next block\n");
		if (block->next->prev != block)
			Sys_Error("Z_CheckHeap: next block doesn't have proper back link\n");
		if (!block->tag && !block->next->tag)
			Sys_Error("Z_CheckHeap: two consecutive free blocks\n");
	}
}

//============================================================================

