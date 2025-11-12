#pragma once

typedef enum {
    AMBIENT_WATER = 0u,
    AMBIENT_SKY   = 1u,
    AMBIENT_SLIME = 2u,
    AMBIENT_LAVA  = 3u,

    NUM_AMBIENTS  = 4u   // automatic ambient sounds
} ambient_type_t;