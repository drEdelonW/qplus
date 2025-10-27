#pragma once

#include "types.h"
#include "enginedefs.h"
#include "Texture.h"

struct SurfCache_s;
typedef struct SurfCache_s SurfCache_t;
typedef SurfCache_t* SurfCache_p;
struct SurfCache_s {
    SurfCache_p  next;
    SurfCache_p*  owner;  // NULL is an empty chunk of memory
    int    lightadj[MAXLIGHTMAPS]; // checked for strobe flush
    int    dlight;
    int    size;  // including header
    uint32_t   width;
    uint32_t   height;  // DEBUG only needed for debug
    float    mipscale;
    Texture_p   texture; // checked for animating textures
    uint8_t   data[4]; // width * height elements
} ;