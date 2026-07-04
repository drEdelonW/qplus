#pragma once

#include "types.h"
// #include "enginedefs.h"
#ifdef GLQUAKE
# include "Surface_pre.h"
#endif
#include "Lump.h"
#include "assert.h"


#include "Texture_pre.h"    // Texture_p

typedef enum {
    Mip0,        // full size
    Mip1,        // half size
    Mip2,        // quarter size
    Mip3,        // eighth size
    MIPLEVELS    // count, used as array size
} MipLevel_t;

struct Texture_s {
    char        name[16];
    uint32_t    width;
    uint32_t    height;
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
    int32_t dataOfs[MIPLEVELS]; // [nummiptex]
} dMipTexLump_t;        STATIC_ASSERT_SIZE(dMipTexLump_t, 4 + 4 * 4); // 20
typedef dMipTexLump_t* dMipTexLump_p;

typedef struct MipTex_s {
    char        name[16];
    uint32_t    width;
    uint32_t    height;
    uint32_t    offsets[MIPLEVELS];  // four mip maps stored
} MipTex_t;
typedef MipTex_t* MipTex_p;


extern Texture_p r_notexture_mip;

#ifdef __cplusplus
extern "C" {
#endif

    Texture_p R_TextureAnimation(Texture_p base);
    void Mod_LoadTextures(Lump_p Lump_in);

#ifdef __cplusplus
}
#endif

#include "vid.h"  // pixel_p

static inline pixel_p GetMipPtr(Texture_p mt, MipLevel_t level) {
    return (pixel_p)((uint8_p)mt + mt->offsets[level]);
}
