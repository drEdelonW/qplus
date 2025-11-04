#pragma once

#include "types.h"
#include "Plane.h"
#include "TexInfo.h"
#include "enginedefs.h"

typedef enum {
    SURF_NONE           = 0,
    SURF_PLANEBACK      = 1u << 1, // 0x02
    SURF_DRAWSKY        = 1u << 2, // 0x04
    SURF_DRAWSPRITE     = 1u << 3, // 0x08
    SURF_DRAWTURB       = 1u << 4, // 0x10
    SURF_DRAWTILED      = 1u << 5, // 0x20
    SURF_DRAWBACKGROUND = 1u << 6  // 0x40
} SurfaceFlags_e;

typedef struct mSurface_s {
    int32_t visframe;  // should be drawn when node is crossed
    int32_t dlightframe;
    int32_t dlightbits;
    mPlane_p    plane;
    // int32_t  flags;
    SurfaceFlags_e    flags;
    int32_t firstedge; // look up in model->surfedges[], negative numbers
    int32_t numedges; // are backwards edges

    // surface generation data
    struct SurfCache_s* cachespots[MIPLEVELS];
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