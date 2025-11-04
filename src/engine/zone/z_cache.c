#include "zone.h"
#include "zone_prv.h"

#include <string.h>
#include "q_tools.h"
#include "sys.h"
#include "console.h"
#include "cmd.h"

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/

struct cache_system_s;
typedef struct cache_system_s cache_system_t;
typedef cache_system_t* cache_system_p;
struct cache_system_s {
    size_t          size;   // including this header
    CacheUser_p     user;
    char            name[16];
    cache_system_p  prev;
    cache_system_p  next;
    cache_system_p  lru_prev;
    cache_system_p  lru_next;	// for LRU flushing
};


cache_system_p Cache_TryAlloc(size_t size, bool nobottom);

cache_system_t	cache_head;

/*
===========
Cache_Move
===========
*/
void Cache_Move(cache_system_p c) {
    // we are clearing up space at the bottom, so only allocate it late
    cache_system_p new = Cache_TryAlloc(c->size, true);
    if (new) {
        //		Con_Printf("cache_move ok\n");

        Q_memcpy(new + 1, c + 1, c->size - sizeof(cache_system_t));
        new->user = c->user;
        Q_memcpy(new->name, c->name, sizeof(new->name));
        Cache_Free(c->user);
        new->user->data = (TypeLess_ptr)(new + 1);
    }
    else {
        //		Con_Printf("cache_move failed\n");

        Cache_Free(c->user);		// tough luck...
    }
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeLow(int new_low_hunk) {
    while (1) {
        cache_system_p c = cache_head.next;
        if (c == &cache_head)                       return;		// nothing in cache at all
        if ((uint8_p)c >= hunk_base + new_low_hunk) return;		// there is space to grow the hunk
        Cache_Move(c);	// reclaim the space
    }
}

/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeHigh(int new_high_hunk) {
    cache_system_p prev = NULL;
    while (1) {
        cache_system_p c = cache_head.prev;
        if (c == &cache_head)
            return;		// nothing in cache at all
        if (((uint8_p)c + c->size) <= (hunk_base + hunk_size - new_high_hunk))
            return;		// there is space to grow the hunk
        if (c == prev)
            Cache_Free(c->user);	// didn't move out of the way
        else {
            Cache_Move(c);	// try to move it
            prev = c;
        }
    }
}

void Cache_UnlinkLRU(cache_system_p cs) {
    if (!cs->lru_next || !cs->lru_prev)
        Sys_Error("Cache_UnlinkLRU: NULL link");

    cs->lru_next->lru_prev = cs->lru_prev;
    cs->lru_prev->lru_next = cs->lru_next;

    cs->lru_prev = cs->lru_next = NULL;
}

void Cache_MakeLRU(cache_system_p cs) {
    if (cs->lru_next || cs->lru_prev)
        Sys_Error("Cache_MakeLRU: active link");

    cache_head.lru_next->lru_prev = cs;
    cs->lru_next = cache_head.lru_next;
    cs->lru_prev = &cache_head;
    cache_head.lru_next = cs;
}

/*
    ============
    Cache_TryAlloc

    Looks for a free block of memory between the high and low hunk marks
    Size should already include the header and padding
    ============
*/
cache_system_p Cache_TryAlloc(size_t size, bool nobottom) {
    // is the cache completely empty?
    if (!nobottom && (cache_head.prev == &cache_head)) {
        if ((hunk_size - hunk_high_used - hunk_low_used) < size)
            Sys_Error("Cache_TryAlloc: %i is greater then free hunk", size);

        cache_system_p new = (cache_system_p)(hunk_base + hunk_low_used);
        memset(new, 0, sizeof(*new));
        new->size = size;

        cache_head.prev = cache_head.next = new;
        new->prev = new->next = &cache_head;

        Cache_MakeLRU(new);
        return new;
    }

    // search from the bottom up for space

    cache_system_p new = (cache_system_p)(hunk_base + hunk_low_used);
    cache_system_p cs = cache_head.next;

    do {
        if ((!nobottom || (cs != cache_head.next)) &&
            (((uint8_p)cs - (uint8_p)new) >= size)
            ) {	// found space
            memset(new, 0, sizeof(*new));
            new->size = size;

            new->next = cs;
            new->prev = cs->prev;
            cs->prev->next = new;
            cs->prev = new;

            Cache_MakeLRU(new);

            return new;
        }

        // continue looking
        new = (cache_system_p)((uint8_p)cs + cs->size);
        cs = cs->next;

    } while (cs != &cache_head);

    // try to allocate one at the very end
    if ((hunk_base + hunk_size - hunk_high_used - (uint8_p)new) >= size) {
        memset(new, 0, sizeof(*new));
        new->size = size;

        new->next = &cache_head;
        new->prev = cache_head.prev;
        cache_head.prev->next = new;
        cache_head.prev = new;

        Cache_MakeLRU(new);

        return new;
    }

    return NULL;		// couldn't allocate
}

/*
    ============
    Cache_Flush

    Throw everything out, so new data will be demand cached
    ============
*/
void Cache_Flush() {
    while (cache_head.next != &cache_head)
        Cache_Free(cache_head.next->user);	// reclaim the space
}


/*
    ============
    Cache_Print

    ============
*/
void Cache_Print() {
    for (cache_system_p cd = cache_head.next; cd != &cache_head; cd = cd->next) {
        Con_Printf("%8i : %s\n", cd->size, cd->name);
    }
}

/*
    ============
    Cache_Report

    ============
*/
void Cache_Report() {
    Con_DPrintf(
        "%4.1f megabyte data cache\n",
        (hunk_size - hunk_high_used - hunk_low_used) / (float)(1024 * 1024)
    );
}

/*
    ============
    Cache_Compact

    ============
*/
void Cache_Compact() {}

/*
    ============
    Cache_Init

    ============
*/
void Cache_Init() {
    cache_head.next = cache_head.prev = &cache_head;
    cache_head.lru_next = cache_head.lru_prev = &cache_head;

    Cmd_AddCommand("flush", Cache_Flush);
}

/*
    ==============
    Cache_Free

    Frees the memory and removes it from the LRU list
    ==============
*/
void Cache_Free(CacheUser_p c) {
    if (!c->data)
        Sys_Error("Cache_Free: not allocated");

    cache_system_p cs = ((cache_system_p)c->data) - 1;

    cs->prev->next = cs->next;
    cs->next->prev = cs->prev;
    cs->next = cs->prev = NULL;

    c->data = NULL;

    Cache_UnlinkLRU(cs);
}



/*
    ==============
    Cache_Check
    ==============
*/
TypeLess_ptr Cache_Check(CacheUser_p c) {
    if (!c->data)
        return NULL;

    cache_system_p cs = ((cache_system_p)c->data) - 1;

    // move to head of LRU
    Cache_UnlinkLRU(cs);
    Cache_MakeLRU(cs);

    return c->data;
}


/*
    ==============
    Cache_Alloc
    ==============
*/
TypeLess_ptr Cache_Alloc(CacheUser_p c, size_t size, cString name) {
    if (c->data)        Sys_Error("Cache_Alloc: allready allocated");
    if (size <= 0)      Sys_Error("Cache_Alloc: size %i", size);

    size = ALIGN_UP((size + sizeof(cache_system_t)), 16);

    // find memory for it
    while (1) {
        cache_system_p cs = Cache_TryAlloc(size, false);
        if (cs) {
            strncpy(cs->name, name, sizeof(cs->name) - 1);
            c->data = (TypeLess_ptr)(cs + 1);
            cs->user = c;
            break;
        }

        // free the least recently used cahedat
        if (cache_head.lru_prev == &cache_head)
            Sys_Error("Cache_Alloc: out of memory");
        // not enough memory at all
        Cache_Free(cache_head.lru_prev->user);
    }

    return Cache_Check(c);
}

//============================================================================

