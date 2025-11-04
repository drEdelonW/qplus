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

#ifndef GLQUAKE
#include "r_shared.h"
#include "client.h"
#include "common.h"
#include "console.h"
#include "sys.h"
#include "mathlib.h"
#include "Alias.h"

#define BMODEL_FULLY_CLIPPED 0x10 // value returned by R_BmodelCheckBBox ()
                                     //  if bbox is trivially rejected

//===========================================================================
// viewmodel lighting

typedef struct {
    int     ambientlight;
    int     shadelight;
    float_p plightvec;
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
struct ClipPlane_s;
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

extern mPlane_t screenedge[4];
extern vec3_t   r_origin;
extern vec3_t   r_entorigin;
extern float    screenAspect;
extern float    verticalFieldOfView;
extern float    xOrigin, yOrigin;
extern int      r_visframecount;

//=============================================================================

extern int vstartscan;


void R_ClearPolyList();
void R_DrawPolyList();

//
// current entity info
//
extern bool insubmodel;
extern vec3_t  r_worldmodelorg;


void R_DrawSprite();
void R_RenderFace(mSurface_p fa, int clipflags);
void R_RenderPoly(mSurface_p fa, int clipflags);
void R_RenderBmodelFace(bEdge_p pedges, mSurface_p psurf);
void R_TransformPlane(mPlane_p p, float_p normal, float_p dist);
void R_TransformFrustum();
void R_SetSkyFrame();
void R_DrawSurfaceBlock16();
void R_DrawSurfaceBlock8();
Texture_p R_TextureAnimation(Texture_p base);

#if id386

void R_DrawSurfaceBlock8_mip0();
void R_DrawSurfaceBlock8_mip1();
void R_DrawSurfaceBlock8_mip2();
void R_DrawSurfaceBlock8_mip3();

#endif

void R_GenSkyTile(TypeLess_ptr pdest);
void R_GenSkyTile16(TypeLess_ptr pdest);
void R_Surf8Patch();
void R_Surf16Patch();
void R_DrawSubmodelPolygons(Model_p pmodel, int clipflags);
void R_DrawSolidClippedSubmodelPolygons(Model_p pmodel);

void R_AddPolygonEdges(EmitPoint_p pverts, int numverts, int miplevel);
Surf_p R_GetSurf();
void R_AliasDrawModel(aLight_p plighting);
void R_BeginEdgeFrame();
void R_ScanEdges();
void D_DrawSurfaces();
void R_InsertNewEdges(Edge_p edgestoadd, Edge_p edgelist);
void R_StepActiveU(Edge_p pedge);
void R_RemoveEdges(Edge_p pedge);

extern void R_Surf8Start();
extern void R_Surf8End();
extern void R_Surf16Start();
extern void R_Surf16End();
extern void R_EdgeCodeStart();
extern void R_EdgeCodeEnd();

extern void R_RotateBmodel();

extern int c_faceclip;
extern int r_polycount;
extern int r_wholepolycount;

extern Model_p cl_worldmodel;

extern int* pfrustum_indexes[4];

// !!! if this is changed, it must be changed in asm_draw.h too !!!
#define NEAR_CLIP 0.01

extern int  ubasestep, errorterm, erroradjustup, erroradjustdown;
extern int  vstartscan;

extern fixed16_t    sadjust, tadjust;
extern fixed16_t    bbextents, bbextentt;

#define MAXBVERTINDEXES 1000 // new clipped vertices when clipping bmodels
//  to the world BSP
extern mVertex_p r_ptverts, r_ptvertsmax;

extern vec3_t   sbaseaxis[3], tbaseaxis[3];
extern float    entity_rotation[3][3];

extern int reinit_surfcache;

extern int r_currentkey;
extern int r_currentbkey;

typedef struct btofpoly_s {
    int         clipflags;
    mSurface_p  psurf;
} btofpoly_t;
typedef btofpoly_t* btofpoly_p;

#define MAX_BTOFPOLYS   5000 // FIXME: tune this

extern int          numbtofpolys;
extern btofpoly_p   pbtofpolys;

void R_InitTurb();
void R_ZDrawSubmodelPolys(Model_p clmodel);

//=========================================================
// Alias models
//=========================================================

#define MAXALIASVERTS  2000 // TODO: tune this
#define ALIAS_Z_CLIP_PLANE 5

extern int          numverts;
extern int          a_skinwidth;
extern mTriangle_p  ptriangles;
extern int          numtriangles;
extern AliasHdr_p   paliashdr;
extern Mdl_p        pmdl;
extern float        leftclip, topclip, rightclip, bottomclip;
extern int          r_acliptype;
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

void R_DrawParticles();
void R_InitParticles();
void R_ClearParticles();
void R_ReadPointFile_f();
void R_SurfacePatch();

extern int      r_amodels_drawn;
extern Edge_p   auxedges;
extern int      r_numallocatededges;
extern Edge_p   r_edges, edge_p, edge_max;
extern Edge_p   newedges[MAXHEIGHT];
extern Edge_p   removeedges[MAXHEIGHT];

extern int screenwidth;

// FIXME: make stack vars when debugging done
extern Edge_t   edge_head;
extern Edge_t   edge_tail;
extern Edge_t   edge_aftertail;
extern int      r_bmodelactive;
// extern vRect_p  pconupdate;

extern float    aliasxscale, aliasyscale, aliasxcenter, aliasycenter;
extern float    r_aliastransition, r_resfudge;

extern int      r_outofsurfaces;
extern int      r_outofedges;

extern mVertex_p    r_pcurrentvertbase;
extern int      r_maxvalidedgeoffset;

void R_AliasClipTriangle(mTriangle_p ptri);

extern float    r_time1;
extern float    dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
extern float    se_time1, se_time2, de_time1, de_time2, dv_time1, dv_time2;
extern int      r_frustum_indexes[4 * 6];
extern int      r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
extern bool r_surfsonstack;
extern ColorShift_t cshift_water;
extern bool r_dowarpold, r_viewchanged;

extern mLeaf_p  r_viewleaf, r_oldviewleaf;

extern vec3_t   r_emins, r_emaxs;
extern mNode_p  r_pefragtopnode;
extern int      r_clipflags;
extern int      r_dlightframecount;
extern bool r_fov_greater_than_90;

void R_StoreEfrags(efrag_p* ppefrag);
void R_TimeRefresh_f();
void R_TimeGraph();
void R_PrintAliasStats();
void R_PrintTimes();
void R_PrintDSpeeds();
void R_AnimateLight();
int  R_LightPoint(vec3_t p);
void R_SetupFrame();
void R_cshift_f();
void R_EmitEdge(mVertex_p pv0, mVertex_p pv1);
void R_ClipEdge(mVertex_p pv0, mVertex_p pv1, ClipPlane_p clip);
void R_SplitEntityOnNode2(mNode_p node);
void R_MarkLights(dLight_p light, int bit, mNode_p node);

#endif