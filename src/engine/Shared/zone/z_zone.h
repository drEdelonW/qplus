#pragma once

/**
 * ============================================================================
 * ZONE ALLOCATOR PROFILE (z_zone.h)
 * ============================================================================
 * ROLE:     Dynamic heap (malloc/free type) for transient data.
 * CAPACITY: 64KB - 128KB in peak.
 * TARGET:   DTCM RAM (Data Tightly Coupled Memory) -> 216MHz, zero wait-states.
 * RUNTIME:  !HOT! Maximum frequency. Hundreds of hits per frame.
 * SCOPE:    PhysSim runtime states, QC VM string/edict parsing, net queues.
 * ============================================================================
 */

#include "types.h"

//========================[z_zone.c]========================//
#ifdef __cplusplus
extern "C" {
#endif

    void         Z_Free(TypeLess_ptr ptr);
    TypeLess_ptr Z_Malloc(size_t size);   // returns 0 filled memory
    TypeLess_ptr Z_TagMalloc(size_t size, int tag);

    void Z_DumpHeap();
    void Z_CheckHeap();
    int  Z_FreeMemory();

#ifdef __cplusplus
}
#endif
