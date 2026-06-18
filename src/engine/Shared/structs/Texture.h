#pragma once

#include "types.h"
#include "enginedefs.h"
#ifdef GLQUAKE
#   include "Surface_pre.h"
#endif
#include "assert.h"

#include "Texture_pre.h"

extern Texture_p r_notexture_mip;

struct Texture_s {
    char        name[16];
    uint32_t    width, height;
#ifdef GLQUAKE
    int         gl_texturenum;
    mSurface_p  texturechain;       // for gl_texsort drawing
#endif
    int32_t     anim_total;         // total tenths in sequence ( 0 = no)
    int32_t     anim_min, anim_max; // time for this frame min <=time< max
    Texture_p   anim_next;          // in the animation sequence
    Texture_p   alternate_anims;    // bmodels in frmae 1 use these
    uint32_t    offsets[MIPLEVELS]; // four mip maps stored
};

#define TEX_SPECIAL  1  // sky or slime, no lightmap or 256 subdivision

typedef struct {
    int32_t nummiptex;
    int32_t dataofs[4]; // [nummiptex]
} dMipTexLump_t;
typedef dMipTexLump_t* dMipTexLump_p;
STATIC_ASSERT_SIZE(dMipTexLump_t, 4 + 4*4); // 20

typedef struct MipTex_s {
    char        name[16];
    uint32_t    width, height;
    uint32_t    offsets[MIPLEVELS];  // four mip maps stored
} MipTex_t;
typedef MipTex_t* MipTex_p;


Texture_p R_TextureAnimation(Texture_p base);

