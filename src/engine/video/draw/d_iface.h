#pragma once
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// d_iface.h: interface header file for rasterization driver modules
#include "mathlib.h"
#include "model.h"

#define WARP_WIDTH  (320)
#define WARP_HEIGHT (200)

#define MAX_LBM_HEIGHT (480)

typedef struct {
    float u, v;
    float s, t;
    float zi;
} EmitPoint_t;
typedef EmitPoint_t* EmitPoint_p;

typedef enum {
    pt_static,
    pt_grav,
    pt_slowgrav,
    pt_fire,
    pt_explode,
    pt_explode2,
    pt_blob,
    pt_blob2
} ParticleType_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct Particle_s Particle_t;
typedef Particle_t* Particle_p;
struct Particle_s {
    // driver-usable fields
    vec3_t          org;
    float           color;
    // drivers never touch the following fields
    Particle_p      next;
    vec3_t          vel;
    float           ramp;
    float           die;
    ParticleType_t  type;
};

#define PARTICLE_Z_CLIP 8.0

typedef struct PolyVert_s {
    float u, v, zi, s, t;
} PolyVert_t;
typedef PolyVert_t* PolyVert_p;


typedef struct PolyDesc_s {
    int         numverts;
    float       nearzi;
    mSurface_p  pcurrentface;
    PolyVert_p  pverts;
} PolyDesc_t;

// flags in FinalVert_t.flags

typedef enum alias_clip_flags_e {
    ALIAS_LEFT_CLIP = 0x0001,
    ALIAS_TOP_CLIP = 0x0002,
    ALIAS_RIGHT_CLIP = 0x0004,
    ALIAS_BOTTOM_CLIP = 0x0008,
    ALIAS_Z_CLIP = 0x0010,

    // must stay in sync with d_ifacea.h and modelgen.h
    ALIAS_ONSEAM = 0x0020,  // also defined in modelgen.h
    //  must be kept in sync
    ALIAS_XY_CLIP_MASK = 0x000F
} AliasClipFlags_f;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct FinalVert_s {
    int     v[6];   // u, v, s, t, l, 1/z
    // int     flags;  //alias_clip_flags_t
    AliasClipFlags_f    flags;  //alias_clip_flags_t
    float   reserved;
} FinalVert_t;
typedef FinalVert_t* FinalVert_p;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct {
    TypeLess_ptr pskin;
    mAliasSkinDesc_p pskindesc;
    int         skinwidth;
    int         skinheight;
    mTriangle_p ptriangles;
    FinalVert_p pfinalverts;
    int         numtriangles;
    int         drawtype;
    int         seamfixupX16;
} AffineTriDesc_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct {
    float u, v, zi, color;
} ScreenPart_t; // ???

typedef struct {
    int         nump;
    EmitPoint_p pverts; // there's room for an extra element at [nump],
    //  if the driver wants to duplicate element [0] at
    //  element [nump] to avoid dealing with wrapping
    mSpriteFrame_p  pspriteframe;
    vec3_t          vup, vright, vpn; // in worldspace
    float           nearzi;
} SpriteDesc_t;

typedef struct {
    int     u, v;
    float   zi;
    int     color;
} zPointDesc_t;

extern int d_spanpixcount;
extern int r_framecount;            // sequence # of current frame since Quake started
extern qboolean r_drawpolys;        // 1 if driver wants clipped polygons rather than a span list
extern qboolean r_drawculledpolys;  // 1 if driver wants clipped polygons that have been culled by the edge list
extern qboolean r_worldpolysbacktofront;    // 1 if driver wants polygons delivered back to front rather than front to back
extern qboolean r_recursiveaffinetriangles; // true if a driver wants to use
//  recursive triangular subdivison and vertex drawing via D_PolysetDrawFinalVerts() past
//  a certain distance (normally only used by the software driver)
extern float r_aliasuvscale;    // scale-up factor for screen u and v on Alias vertices passed to driver
extern int r_pixbytes;
extern qboolean r_dowarp;

extern AffineTriDesc_t r_affinetridesc;
extern SpriteDesc_t r_spritedesc;
extern zPointDesc_t r_zpointdesc;
extern PolyDesc_t r_polydesc;

extern int d_con_indirect; // if 0, Quake will draw console directly to vid.buffer; if 1, Quake will
//  draw console via D_DrawRect. Must be defined by driver

extern vec3_t r_pright, r_pup, r_ppn;


void D_Aff8Patch(TypeLess_ptr pcolormap);
void D_BeginDirectRect(int x, int y, uint8_p pbitmap, int width, int height);
void D_DisableBackBufferAccess();
void D_EndDirectRect(int x, int y, int width, int height);
void D_PolysetDraw();
void D_PolysetDrawFinalVerts(FinalVert_p fv, int numverts);
void D_DrawParticle(Particle_p pparticle);
void D_DrawPoly();
void D_DrawSprite();
void D_DrawSurfaces();
void D_DrawZPoint();
void D_EnableBackBufferAccess();
void D_EndParticles();
void D_Init();
void D_ViewChanged();
void D_SetupFrame();
void D_StartParticles();
void D_TurnZOn();
void D_WarpScreen();

void D_FillRect(vRect_p vrect, int color);
void D_DrawRect();
void D_UpdateRects(vRect_p prect);

// currently for internal use only, and should be a do-nothing function in hardware drivers
// FIXME: this should go away
void D_PolysetUpdateTables();

// these are currently for internal use only, and should not be used by drivers
extern int r_skydirect;
extern uint8_p r_skysource;

// transparency types for D_DrawRect ()
typedef enum {
    DR_SOLID = 0, // draw solid
    DR_TRANSPARENT = 1  // draw transparent
} drawrect_t;

// !!! must be kept the same as in quakeasm.h !!!
#define TRANSPARENT_COLOR (0xFF)

extern TypeLess_ptr acolormap; // FIXME: should go away

//=======================================================================//

// callbacks to Quake

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

void R_DrawSurface();
void R_GenTile(mSurface_p psurf, TypeLess_ptr pdest);


// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define TURB_TEX_SIZE 64  // base turbulent texture size

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CYCLE       128  // turbulent cycle size

#define TILE_SIZE   128  // size of textures generated by R_GenTiledSurf

#define SKYSHIFT    7
#define SKYSIZE     (1 << SKYSHIFT)
#define SKYMASK     (SKYSIZE - 1)

extern float skyspeed, skyspeed2;
extern float skytime;

extern int c_surf;
extern vRect_t scr_vrect;
extern uint8_p r_warpbuffer;

