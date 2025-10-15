#pragma once
/*
Copyright(C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// refresh.h -- public interface to refresh functions
#include "model_effect.h"
#include "enginedefs.h"
#include "vid.h"
// #include "model.h"

#define MAXCLIPPLANES (11)

#define TOP_RANGE  (16)   // soldier uniform colors
#define BOTTOM_RANGE (96)

//=============================================================================
typedef struct r_Entity_s r_Entity_t;
typedef r_Entity_t* r_Entity_p;

// struct efrag_s;
typedef struct efrag_s efrag_t;
typedef efrag_t* efrag_p;
struct efrag_s {
    struct mLeaf_s* leaf;
    efrag_p     leafnext;
    r_Entity_p  entity;
    efrag_p     entnext;
};


// entity effects
typedef enum {
    EF_BRIGHTFIELD  = 1 << 0, // 0x0001
    EF_MUZZLEFLASH  = 1 << 1, // 0x0002
    EF_BRIGHTLIGHT  = 1 << 2, // 0x0004
    EF_DIMLIGHT     = 1 << 3, // 0x0008
#ifdef QUAKE2
    EF_DARKLIGHT    = 1 << 4, // 0x0010
    EF_DARKFIELD    = 1 << 5, // 0x0020
    EF_LIGHT        = 1 << 6, // 0x0040
    EF_NODRAW       = 1 << 7  // 0x0080
#endif
} EntityEffects_t;

struct r_Entity_s {
    qboolean        forcelink;  // model changed
    int             update_type;
    EntityState_t   baseline;  // to fill in defaults in updates
    double          msgtime;  // time of last update
    vec3_t          msg_origins[2]; // last two updates(0 is newest)
    vec3_t          origin;
    vec3_t          msg_angles[2]; // last two updates(0 is newest)
    vec3_t          angles;
    struct Model_s* model;   // NULL = no model
    efrag_p         efrag;   // linked list of efrags
    int             frame;
    float           syncbase;  // for client-side animations
    uint8_p         colormap;
    // int             effects;  // light, particals, etc
    EntityEffects_t effects;  // light, particals, etc
    int             skinnum;  // for Alias models
    int             visframe;  // last frame this entity was
    //  found in an active leaf
    int             dlightframe; // dynamic lighting
    int             dlightbits;

    // FIXME: could turn these into a union
    int             trivial_accept;
    struct mNode_s* topnode;  // for bmodels, first world node
    //  that splits bmodel, or NULL if not split
} ;
typedef r_Entity_t* r_Entity_p;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    vRect_t vrect;    // subwindow in video for refresh
    // FIXME: not need vrect next field here?
    vRect_t aliasvrect;   // scaled Alias version
    int     vrectright, vrectbottom; // right & bottom screen coords
    int     aliasvrectright, aliasvrectbottom; // scaled Alias versions
    float   vrectrightedge;   // rightmost right edge we care about,
    //  for use in edge list
    float   fvrectx, fvrecty;  // for floating-point compares
    float   fvrectx_adj, fvrecty_adj; // left and top edges, for clamping
    int     vrect_x_adj_shift20; //(vrect.x + 0.5 - epsilon) << 20
    int     vrectright_adj_shift20; //(vrectright + 0.5 - epsilon) << 20
    float   fvrectright_adj, fvrectbottom_adj;
    // right and bottom edges, for clamping
    float   fvrectright;   // rightmost edge, for Alias clamping
    float   fvrectbottom;   // bottommost edge, for Alias clamping
    float   horizontalFieldOfView; // at Z = 1.0, this many X is visible
    // 2.0 = 90 degrees
    float   xOrigin;   // should probably allways be 0.5
    float   yOrigin;   // between be around 0.3 to 0.5
    vec3_t  vieworg;
    vec3_t  viewangles;
    float   fov_x, fov_y;
    int     ambientlight;
} refdef_t;


//
// refresh
//
extern int  reinit_surfcache;


extern refdef_t r_refdef;
extern vec3_t r_origin, vpn, vright, vup;

extern struct Texture_s* r_notexture_mip;


void R_Init();
void R_InitTextures();
void R_InitEfrags();
void R_RenderView();  // must set r_refdef first
void R_ViewChanged(vRect_p pvrect, int lineadj, float aspect);
// called whenever r_refdef or vid change
void R_InitSky(struct Texture_s* mt); // called at level load

void R_AddEfrags(r_Entity_p ent);
void R_RemoveEfrags(r_Entity_p ent);

void R_NewMap();


void R_ParseParticleEffect();
void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count);
void R_RocketTrail(vec3_t start, vec3_t end, RocketTrailType type);

#ifdef QUAKE2
void R_DarkFieldParticles(r_Entity_p ent);
#endif
void R_EntityParticles(r_Entity_p ent);
void R_BlobExplosion(vec3_t org);
void R_ParticleExplosion(vec3_t org);
void R_ParticleExplosion2(vec3_t org, int colorStart, int colorLength);
void R_LavaSplash(vec3_t org);
void R_TeleportSplash(vec3_t org);

void R_PushDlights();


//
// surface cache related
//
extern int  reinit_surfcache; // if 1, surface cache is currently empty and
extern qboolean r_cache_thrash; // set if thrashing the surface cache

int  D_SurfaceCacheForRes(int width, int height);
void D_FlushCaches();
void D_DeleteSurfaceCache();
void D_InitCaches(TypeLess_ptr buffer, int size);
void R_SetVrect(vRect_p pvrect, vRect_p pvrectin, int lineadj);

