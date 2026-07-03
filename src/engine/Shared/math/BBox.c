#include "BBox.h"

const BBox_t bbNull = {
    .mins = { .x =  9999.f, .y =  9999.f, .z =  9999.f },
    .maxs = { .x = -9999.f, .y = -9999.f, .z = -9999.f }
};

const BBox_t bbZero = {
    .mins = { .x = 0.f, .y = 0.f, .z = 0.f }, // v3Zero
    .maxs = { .x = 0.f, .y = 0.f, .z = 0.f }  // v3Zero
};