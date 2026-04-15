#pragma once

#include "vector.h"
#include "types.h"
// entity effects
typedef enum {
    EF_BRIGHTFIELD  = 1u << 0, // 0x0001
    EF_MUZZLEFLASH  = 1u << 1, // 0x0002
    EF_BRIGHTLIGHT  = 1u << 2, // 0x0004
    EF_DIMLIGHT     = 1u << 3, // 0x0008
#ifdef QUAKE2
    EF_DARKLIGHT    = 1u << 4, // 0x0010
    EF_DARKFIELD    = 1u << 5, // 0x0020
    EF_LIGHT        = 1u << 6, // 0x0040
    EF_NODRAW       = 1u << 7  // 0x0080
#endif
} EntityEffects_t;

// #pragma pack(push, 1)
typedef struct {
    vec3_t  origin;
    vec3_t  angles;
    uint8_t modelindex;
    uint8_t frame;
    uint8_t colormap;
    uint8_t skin;
    uint8_t effects; // EntityEffects_t
} EntityState_t;
// #pragma pack(pop)

STATIC_ASSERT_SIZE(EntityState_t, ((sizeof(vec3_t) * 2) + (sizeof(uint8_t) * 5)) + 3); // why 3 ???