#pragma once

#include "types.h"
#include "enginedefs.h"
#ifdef GLQUAKE
#   include "Surface_pre.h"
#endif

// it was [texture_t]
typedef struct Texture_s Texture_t;
typedef Texture_t* Texture_p;
struct Texture_s {
    char        name[16];
    uint32_t    width, height;
#ifdef GLQUAKE
    int         gl_texturenum;
    mSurface_p  texturechain; // for gl_texsort drawing
#endif
    int32_t     anim_total;    // total tenths in sequence ( 0 = no)
    int32_t     anim_min, anim_max;  // time for this frame min <=time< max
    Texture_p   anim_next;  // in the animation sequence
    Texture_p   alternate_anims; // bmodels in frmae 1 use these
    uint32_t    offsets[MIPLEVELS];  // four mip maps stored
};

Texture_p R_TextureAnimation(Texture_p base);

