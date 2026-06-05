#pragma once
#include "types.h"

// Align value 'x' up to nearest multiple of 'a' (a must be power of two)
#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))

//========================[z_hulk.c]========================//
extern uint8_p  hunk_base;
extern size_t   hunk_size;
extern size_t   hunk_low_used;
extern size_t   hunk_high_used;

#ifdef __cplusplus
extern "C" {
#endif

    void Hulk_Init(TypeLess_ptr buf, size_t size);

#ifdef __cplusplus
}
#endif
//========================[z_cache.c]========================//

#ifdef __cplusplus
extern "C" {
#endif

    void Cache_FreeHigh(int new_high_hunk);
    void Cache_FreeLow(int new_low_hunk);

    void Cache_Init();

#ifdef __cplusplus
}
#endif

//========================[z_zone.c]========================//

typedef struct memblock_s memblock_t;
typedef memblock_t* memblock_p;
struct memblock_s {
    memblock_p  next;
    memblock_p  prev;
    size_t      size;   // including the header and possibly tiny fragments
    int32_t     tag;    // a tag of 0 is a free block
    int32_t     id;     // should be ZONEID
    int32_t     pad;    // pad to 64 bit boundary
};

typedef struct {
    size_t      size;       // total bytes malloced, including header
    memblock_t  blocklist;  // start / end cap for linked list
    memblock_p  rover;
} memzone_t;
typedef memzone_t* memzone_p;

extern memzone_p  mainzone;

#ifdef __cplusplus
extern "C" {
#endif

    void Z_ClearZone(memzone_p zone, size_t size);

#ifdef __cplusplus
}
#endif

