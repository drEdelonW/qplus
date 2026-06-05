#pragma once

/**
 * ============================================================================
 * CACHE ALLOCATOR PROFILE (z_cache.h)
 * ============================================================================
 * ROLE:     Dynamic LRU media pool. Relocatable and purgable blocks.
 * CAPACITY: 4MB - 6MB (PC original) / 1MB - 2MB (Downsampled assets).
 * TARGET:   External SDRAM (Dedicated section separate from Hunk).
 * RUNTIME:  !COLD! Low/Waveform frequency. Handled by DMA2D / Audio IRQ.
 * SCOPE:    Client side media: UI pics (.lmp), sprites (.spr), sounds (.wav).
 * ============================================================================
 */

#include "types.h"
//========================[z_cache.c]========================//
typedef struct CacheUser_s {
    TypeLess_ptr   data;
} CacheUser_t;
typedef CacheUser_t* CacheUser_p;

#ifdef __cplusplus
extern "C" {
#endif

    void Cache_Flush();

    // returns the cached data, and moves to the head of the LRU list if present, otherwise returns NULL
    TypeLess_ptr Cache_Check(CacheUser_p c);

    void Cache_Free(CacheUser_p c);

    // Returns NULL if all purgable data was tossed and there still wasn't enough room.
    TypeLess_ptr Cache_Alloc(CacheUser_p c, size_t size, cString name);

    void Cache_Report();

    void COM_LoadCacheFile(cStringRO path, CacheUser_p cu);

#ifdef __cplusplus
}
#endif