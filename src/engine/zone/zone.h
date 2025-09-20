#pragma once
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
 memory allocation


H_??? The hunk manages the entire memory block given to quake.  It must be
contiguous.  Memory can be allocated from either the low or high end in a
stack fashion.  The only way memory is released is by resetting one of the
pointers.

Hunk allocations should be given a name, so the Hunk_Print () function
can display usage.

Hunk allocations are guaranteed to be 16 byte aligned.

The video buffers are allocated high to avoid leaving a hole underneath
server allocations when changing to a higher video mode.


Z_??? Zone memory functions used for small, dynamic allocations like text
strings from command input.  There is only about 48K for it, allocated at
the very bottom of the hunk.

Cache_??? Cache memory is for objects that can be dynamically loaded and
can usefully stay persistant between levels.  The size of the cache
fluctuates from level to level.

To allocate a cachable object


Temp_??? Temp memory is used for file loading and surface caching.  The size
of the cache memory is adjusted so that there is a minimum of 512k remaining
for temp memory.


------ Top of Memory -------

high hunk allocations

<--- high hunk reset point held by vid

video buffer

z buffer

surface cache

<--- high hunk used

cachable memory

<--- low hunk used

client and server low hunk allocations

<-- low hunk reset point held by host

startup hunk allocations

Zone block

----- Bottom of Memory -----



*/
#include <stddef.h>

typedef char* cstring;
typedef const char* cstring_ro;  // read-only
typedef void* typeless_ptr;

void Memory_Init(typeless_ptr buf, size_t size);

//========================[z_hulk.c]========================//
void 		 Z_Free(typeless_ptr ptr);
typeless_ptr Z_Malloc(size_t size);   // returns 0 filled memory
typeless_ptr Z_TagMalloc(size_t size, int tag);

void Z_DumpHeap();
void Z_CheckHeap();
int  Z_FreeMemory();


//========================[z_hulk.c]========================//
typeless_ptr Hunk_Alloc(size_t size); // returns 0 filled memory
typeless_ptr Hunk_AllocName(size_t size, cstring name);

typeless_ptr Hunk_HighAllocName(size_t size, cstring name);

size_t  Hunk_LowMark();
void    Hunk_FreeToLowMark(size_t mark);

size_t  Hunk_HighMark();
void    Hunk_FreeToHighMark(size_t mark);

typeless_ptr Hunk_TempAlloc(size_t size);

void Hunk_Check();


//========================[z_cache.c]========================//
typedef struct cache_user_s{
	typeless_ptr   data;
} cache_user_t;
typedef cache_user_t* cache_user_p;


void Cache_Flush();

// returns the cached data, and moves to the head of the LRU list if present, otherwise returns NULL
typeless_ptr Cache_Check(cache_user_p c);

void Cache_Free(cache_user_p c);

// Returns NULL if all purgable data was tossed and there still wasn't enough room.
typeless_ptr Cache_Alloc(cache_user_p c, size_t size, cstring name);

void Cache_Report();



