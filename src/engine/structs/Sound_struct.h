#pragma once

typedef enum {
    AMBIENT_WATER = 0,
    AMBIENT_SKY   = 1,
    AMBIENT_SLIME = 2,
    AMBIENT_LAVA  = 3,

    NUM_AMBIENTS  = 4   // automatic ambient sounds
} ambient_type_t;