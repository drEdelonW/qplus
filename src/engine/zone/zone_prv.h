#pragma once
// #include <stddef.h>
#include "byte_t.h"
#include "qboolean.h"

// Align value 'x' up to nearest multiple of 'a' (a must be power of two)
#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))

//========================[z_hulk.c]========================//
extern byte*       hunk_base;
extern size_t      hunk_size;

extern size_t      hunk_low_used;
extern size_t      hunk_high_used;

void Hulk_Init(typeless_ptr buf, size_t size);

//========================[z_cache.c]========================//

struct memblock_s;
typedef struct memblock_s memblock_t;
typedef memblock_t* memblock_p;
struct memblock_s{
	size_t      size;   // including the header and possibly tiny fragments
	int         tag;    // a tag of 0 is a free block
	int         id;     // should be ZONEID
	memblock_p  next;
    memblock_p  prev;
	int		    pad;    // pad to 64 bit boundary
};


void Cache_FreeHigh(int new_high_hunk);
void Cache_FreeLow(int new_low_hunk);

void Cache_Init();

//========================[z_cache.c]========================//
typedef struct{
	size_t 		size;       // total bytes malloced, including header
	memblock_t  blocklist;  // start / end cap for linked list
	memblock_p  rover;
} memzone_t;
typedef memzone_t* memzone_p;

extern memzone_p  mainzone;

void Z_ClearZone(memzone_p zone, size_t size);



