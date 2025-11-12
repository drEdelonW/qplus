#pragma once

#include "types.h"
#include "Plane.h"
#include "TexInfo.h"
#include "enginedefs.h"
#include "SurfCache.h"

typedef enum {
    SURF_NONE           = 0u,
    SURF_PLANEBACK      = 1u << 1, // 0x02
    SURF_DRAWSKY        = 1u << 2, // 0x04
    SURF_DRAWSPRITE     = 1u << 3, // 0x08
    SURF_DRAWTURB       = 1u << 4, // 0x10
    SURF_DRAWTILED      = 1u << 5, // 0x20
    SURF_DRAWBACKGROUND = 1u << 6  // 0x40
} SurfaceFlags_e;

#ifdef GLQUAKE

#define VERTEXSIZE 7

typedef struct glpoly_s glpoly_t;
typedef glpoly_t* glpoly_p;
struct glpoly_s {
    glpoly_p next;
    glpoly_p chain;
    int  numverts;
    int  flags;   // for SURF_UNDERWATER
    float verts[4][VERTEXSIZE]; // variable sized (xyz s1t1 s2t2)
} ;
#endif

typedef struct mSurface_s {
    int32_t visframe;  // should be drawn when node is crossed
    int32_t dlightframe;
    int32_t dlightbits;
    mPlane_p    plane;
    SurfaceFlags_e    flags;
    int32_t firstedge; // look up in model->surfedges[], negative numbers
    int32_t numedges; // are backwards edges

    // surface generation data
#ifdef GLQUAKE
    mSurface_p  texturechain;
    int         light_s, light_t; // gl lightmap coordinates
    glpoly_p    polys;    // multiple if warped
    int         lightmaptexturenum;
    int         cached_light[MAXLIGHTMAPS]; // values currently used in lightmap
    bool        cached_dlight;    // true if dynamic light in cache
#else
    SurfCache_p cachespots[MIPLEVELS];
#endif
    int16_t texturemins[2];
    int16_t extents[2];
    mTexInfo_p texinfo;

    // lighting info
    uint8_t styles[MAXLIGHTMAPS];
    uint8_p samples;  // [numstyles*surfsize]
} mSurface_t;
typedef mSurface_t* mSurface_p;
typedef mSurface_p* mSurface_ar;

#include "vid.h"  //    pixel_p
typedef struct {
    pixel_p     surfdat;                // destination for generated surface
    int         rowbytes;               // destination logical width in bytes
    mSurface_p  surf;                   // description for surface to generate
    fixed8_t    lightadj[MAXLIGHTMAPS]; // adjust for lightmap levels for dynamic lighting
    Texture_p   texture;                // corrected for animating textures
    int         surfmip;                // mipmapped ratio of surface texels / world pixels
    int         surfwidth;              // in mipmapped texels
    int         surfheight;             // in mipmapped texels
} DrawSurf_t;
extern DrawSurf_t r_drawsurf;