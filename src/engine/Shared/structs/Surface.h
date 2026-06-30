#pragma once

#include "types.h"
#include "Plane.h"
#include "TexInfo.h"
#include "enginedefs.h"
#include "SurfCache.h"
#include "Face.h"

typedef enum {
    SURF_NONE           = 0u,
    SURF_PLANEBACK      = 1u << 1, // 0x02
    SURF_DRAWSKY        = 1u << 2, // 0x04
    SURF_DRAWSPRITE     = 1u << 3, // 0x08
    SURF_DRAWTURB       = 1u << 4, // 0x10
    SURF_DRAWTILED      = 1u << 5, // 0x20
    SURF_DRAWBACKGROUND = 1u << 6, // 0x40
    SURF_UNDERWATER     = 1u << 7  // 0x80  for GLQUAKE
} SurfaceFlags_e;

#ifdef GLQUAKE
typedef union {
    struct {
        vec3_t  v;      // 0, 1, 2,
        vec2_t  tx;     // 3, 4,
        vec2_t  lMap;   // 5, 6
    };
    float vf[7];
} glVert_t;
typedef glVert_t* glVert_p;
STATIC_ASSERT_SIZE(glVert_t, 7 * sizeof(vec_t));


typedef struct glpoly_s glpoly_t;
typedef glpoly_t* glpoly_p;
struct glpoly_s {
    glpoly_p    next;
    glpoly_p    chain;
    int         numverts;
    int         flags;      // for SURF_UNDERWATER
    glVert_t    verts[4];
};
#endif

#include "Surface_pre.h"
// it was [msurface_t]
struct mSurface_s {
    int32_t     visframe;   // should be drawn when node is crossed
    int32_t     dlightframe;
    fixed8_t     dlightbits;
    mPlane_p        plane;
    SurfaceFlags_e  flags;
    int32_t     firstedge;  // look up in model->surfedges[], negative numbers
    int32_t     numedges;   // are backwards edges

    // surface generation data
#ifdef GLQUAKE
    mSurface_p  texturechain;
    int         light_s;
    int         light_t;           // gl lightmap coordinates
    glpoly_p    polys;                      // multiple if warped
    int         lightmaptexturenum;
    int         cached_light[MAXLIGHTMAPS]; // values currently used in lightmap
    bool        cached_dlight;              // true if dynamic light in cache
#else
    SurfCache_p cachespots[MIPLEVELS];
#endif
    fixed4_t    texturemins[VECT_TX_DIM];   // TODO: check is it fixed4_ jh fixed16_t
    fixed4_t    extents[VECT_TX_DIM];
    mTexInfo_p  texinfo;

    // lighting info
    uint8_t     styles[MAXLIGHTMAPS];
    uint8_p     samples;                // [numstyles*surfsize]
};

#include "vid.h"  // pixel_p
// it was [drawsurf_t]

#include "Texture_pre.h"    // Texture_p
#include "fixed.h"          // fixed8_t
typedef struct {
    pixel_p     surfdat;                // destination for generated surface
    int         rowbytes;               // destination logical width in bytes
    mSurface_p  surf;                   // description for surface to generate
    fixed8_t    lightadj[MAXLIGHTMAPS]; // adjust for lightmap levels for dynamic lighting
    Texture_p   texture;                // corrected for animating textures
    MipLevel_t  surfmip;                // mipmapped ratio of surface texels / world pixels
#if 1
    int         surfwidth;              // in mipmapped texels
    int         surfheight;             // in mipmapped texels
#else
    vRect_t     surf;       // TODO: solve the name collision
#endif
} DrawSurf_t;
extern DrawSurf_t r_drawsurf;