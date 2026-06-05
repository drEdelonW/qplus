#pragma once

/**
 * ============================================================================
 * HUNK ALLOCATOR PROFILE (z_hunk.h)
 * ============================================================================
 * ROLE:     Monolithic dual-direction stack arena. Write-once, read-always.
 * CAPACITY: 8MB - 12MB (PC original) / 2MB - 4MB (Compressed embedded maps).
 * TARGET:   External SDRAM (via FMC) + L1 Data Cache ENABLED.
 * RUNTIME:  !WARM! High-density read-only loops, zero runtime writes.
 * SCOPE:    Static world geometry (BSP planes/nodes), collision clipnodes.
 * ============================================================================
 */

#include "types.h"

//========================[z_hulk.c]========================//
#ifdef __cplusplus
extern "C" {
#endif

    TypeLess_ptr Hunk_Alloc(size_t size); // returns 0 filled memory
    TypeLess_ptr Hunk_AllocName(size_t size, cStringRO name);

    TypeLess_ptr Hunk_HighAllocName(size_t size, cStringRO name);

    size_t  Hunk_LowMark();
    void    Hunk_FreeToLowMark(size_t mark);

    size_t  Hunk_HighMark();
    void    Hunk_FreeToHighMark(size_t mark);

    TypeLess_ptr Hunk_TempAlloc(size_t size);

    void Hunk_Check();

#ifdef __cplusplus
}
#endif