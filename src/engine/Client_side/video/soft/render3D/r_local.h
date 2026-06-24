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
// r_local.h -- private refresh defs

#ifdef GLQUAKE
#   error GLQUAKE defined
#endif

#ifndef GLQUAKE
#include "r_shared.h"
#include "client.h"
#include "common.h"
#include "mathlib.h"

#define BMODEL_FULLY_CLIPPED 0x10 // value returned by R_BmodelCheckBBox()
                                     //  if bbox is trivially rejected

//===========================================================================
// viewmodel lighting

typedef struct {
    int     ambientlight;
    int     shadelight;
    vec3_p  plightvec;
} aLight_t;
typedef aLight_t* aLight_p;

//===========================================================================

#define XCENTERING (1.0 / 2.0)
#define YCENTERING (1.0 / 2.0)
#define CLIP_EPSILON  0.001
#define BACKFACE_EPSILON 0.01

//===========================================================================

#define DIST_NOT_SET (98765)

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct ClipPlane_s ClipPlane_t;
typedef ClipPlane_t* ClipPlane_p;
struct ClipPlane_s {
    vec3_t      normal;
    float       dist;
    ClipPlane_p next;
    uint8_t     leftedge;
    uint8_t     rightedge;
    uint8_t     reserved[2];
};
extern ClipPlane_t view_clipplanes[4];

//=============================================================================

void R_RenderWorld();

//=============================================================================

extern mPlane_t screenedge[4];  // r_main.c
extern vec3_t   r_origin;
extern vec3_t   r_entorigin;
extern int      r_visframecount;    // r_main.c

//=============================================================================

//
// current entity info
//
extern bool insubmodel; // r_bsp.c
#if 0   // not needed extern
extern vec3_t  r_worldmodelorg;
#endif


void R_DrawSprite();
void R_RenderFace(mSurface_p fa, int clipflags);
void R_RenderPoly(mSurface_p fa, int clipflags);
void R_RenderBmodelFace(bEdge_p pedges, mSurface_p psurf);
// void R_TransformPlane(mPlane_p p, vec3_p normal, float_p dist);
void R_TransformFrustum();
void R_SetSkyFrame();
void R_DrawSurfaceBlock16();

#if id386
// used for surfmiptable[]
void R_DrawSurfaceBlock8_mip0();
void R_DrawSurfaceBlock8_mip1();
void R_DrawSurfaceBlock8_mip2();
void R_DrawSurfaceBlock8_mip3();
#endif

void R_GenSkyTile(TypeLess_ptr pdest);
void R_GenSkyTile16(TypeLess_ptr pdest);
void R_Surf8Patch();    // we only patch code on Intel
void R_Surf16Patch();   // we only patch code on Intel
void R_DrawSubmodelPolygons(Model_p pmodel, int clipflags);
void R_DrawSolidClippedSubmodelPolygons(Model_p pmodel);

void R_AliasDrawModel(aLight_p plighting);
void R_BeginEdgeFrame();
void R_ScanEdges();
void D_DrawSurfaces();

#if id386
extern void R_Surf8Start();
extern void R_Surf8End();
extern void R_Surf16Start();
extern void R_Surf16End();
extern void R_EdgeCodeStart();
extern void R_EdgeCodeEnd();
#endif

extern void R_RotateBmodel();

#if 1   // Debug counters
extern int c_faceclip;
extern int r_polycount;
extern int r_wholepolycount;
extern int r_amodels_drawn;
extern int r_bmodelactive;
extern int r_outofsurfaces;
extern int r_outofedges;
#endif 

extern int* pfrustum_indexes[4];    // TODO: avoid int*

// !!! if this is changed, it must be changed in asm_draw.h too !!!
#define NEAR_CLIP 0.01

#define MAXBVERTINDEXES 1000 // new clipped vertices when clipping bmodels
//  to the world BSP


extern int r_currentkey;    // r_edge.c
extern int r_currentbkey;   // r_bsp.c

#ifdef STM32
    typedef uint8_t ClipFlag_t ;
#else
    typedef int     ClipFlag_t;
#endif

typedef struct btofpoly_s {
    mSurface_p  psurf;
    ClipFlag_t  clipflags;
} btofpoly_t;
typedef btofpoly_t* btofpoly_p;

#ifdef STM32
#   define MAX_BTOFPOLYS   500
#else
#   define MAX_BTOFPOLYS   5000 /* FIXME: tune this */
#endif

extern btofpoly_p   pbtofpolys;     // r_main.c
extern int          numbtofpolys;   // r_main.c

void R_ZDrawSubmodelPolys(Model_p clmodel);

//=========================================================
// Alias models
//=========================================================

#define MAXALIASVERTS  2000 // TODO: tune this
#define ALIAS_Z_CLIP_PLANE 5

extern FinalVert_p  pfinalverts;
extern AuxVert_p    pauxverts;

bool R_AliasCheckBBox();

//=========================================================
// turbulence stuff

#define AMP  (8 * 0x10000)
#define AMP2 3
#define SPEED 20

//=========================================================
// particle stuff

#if 1   // Public functions for Soft and OpenGL render
void R_DrawParticles();
void R_InitParticles();
void R_ClearParticles();
void R_ReadPointFile_f();
#endif
void R_SurfacePatch();

extern int      r_numallocatededges;
extern Edge_p   auxedges;
extern Edge_p   r_edges;
extern Edge_p   edge_p;
extern Edge_p   edge_max;
extern Edge_p   newedges[MAXHEIGHT];
extern Edge_p   removeedges[MAXHEIGHT];

extern float    aliasxscale, aliasyscale, aliasxcenter, aliasycenter;   // r_alias.c
extern float    r_aliastransition, r_resfudge;

extern mVertex_p    r_pcurrentvertbase; // r_main.c

void R_AliasClipTriangle(mTriangle_p ptri);

extern float    r_time1; // r_main.c // TODO: move this to time specific code
extern int      r_frustum_indexes[];   // r_main.c
extern int      r_maxsurfsseen, r_maxedgesseen;
extern int      r_cnumsurfs;
extern bool     r_dowarpold, r_viewchanged;
extern mLeaf_p  r_viewleaf, r_oldviewleaf;
extern vec3_t   r_emins, r_emaxs;
extern mNode_p  r_pefragtopnode;
extern int      r_clipflags;
extern int      r_dlightframecount;

void R_StoreEfrags(efrag_ar ppefrag);
#if 1   // Public functions for Soft and OpenGL render
void R_TimeRefresh_f();
void R_AnimateLight();
int  R_LightPoint(vec3_t p);
#endif
void R_TimeGraph();
void R_PrintAliasStats();
void R_PrintTimes();
void R_SetupFrame();
void R_SplitEntityOnNode2(mNode_p node);

#include "Light.h"
void R_MarkLights(dLight_p light, int bit, mNode_p node);

#endif