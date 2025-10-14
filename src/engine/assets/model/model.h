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
#include "enginedefs.h"
#include "modelgen.h"
#include "spritegn.h"
#include "bspfile.h"
#include "render.h"
#include "platformdefs.h"
#include "zone.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    vec3_t  position;
} mVertex_t;
typedef mVertex_t* mVertex_p;

typedef enum {
    SIDE_FRONT = 0, // point is in front of plane
    SIDE_BACK  = 1, // point is behind plane
    SIDE_ON    = 2  // point is on plane
} Side_t;

// Plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mPlane_s {
    vec3_t  normal;
    float   dist;
    uint8_t type;   // for texture axis selection and fast side tests
    uint8_t signbits;  // signx + signy<<1 + signz<<1
    uint8_t pad[2];
} mPlane_t;
typedef mPlane_t* mPlane_p;

struct Texture_s;
typedef struct Texture_s Texture_t;
typedef Texture_t* Texture_p;
struct Texture_s {
    char        name[16];
    uint32_t    width, height;
    int32_t     anim_total;    // total tenths in sequence ( 0 = no)
    int32_t     anim_min, anim_max;  // time for this frame min <=time< max
    Texture_p   anim_next;  // in the animation sequence
    Texture_p   alternate_anims; // bmodels in frmae 1 use these
    uint32_t    offsets[MIPLEVELS];  // four mip maps stored
};


typedef enum {
    SURF_NONE           = 0,
    SURF_PLANEBACK      = 1u << 1, // 0x02
    SURF_DRAWSKY        = 1u << 2, // 0x04
    SURF_DRAWSPRITE     = 1u << 3, // 0x08
    SURF_DRAWTURB       = 1u << 4, // 0x10
    SURF_DRAWTILED      = 1u << 5, // 0x20
    SURF_DRAWBACKGROUND = 1u << 6  // 0x40
} SurfaceFlags_e;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    uint16_t v[2];
    uint32_t cachededgeoffset;
} mEdge_t;
typedef mEdge_t* mEdge_p;


typedef struct {
    float   vecs[2][4];
    float  mipadjust;
    Texture_p texture;
    int32_t   flags;
} mTexInfo_t;
typedef mTexInfo_t* mTexInfo_p;


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


struct mNode_s;
typedef struct mNode_s mNode_t;
typedef mNode_t* mNode_p;
struct mNode_s {
    // common with leaf
    int32_t     contents;  // 0, to differentiate from leafs
    int32_t     visframe;  // node needs to be traversed if current
    int16_t     minmaxs[6];  // for bounding box culling
    mNode_p     parent;
    // node specific
    mPlane_p    plane;
    mNode_p     children[2];
    uint16_t    firstsurface;
    uint16_t    numsurfaces;
};



typedef struct mLeaf_s {
    // common with node
    int32_t contents;   // wil be a negative contents number
    int32_t visframe;   // node needs to be traversed if current
    int16_t minmaxs[6]; // for bounding box culling
    mNode_p parent;

    // leaf specific
    uint8_p compressed_vis;
    efrag_p efrags;
    mSurface_p* firstmarksurface;
    int32_t nummarksurfaces;
    int32_t key;        // BSP sequence number for leaf's contents
    uint8_t ambient_sound_level[NUM_AMBIENTS];
} mLeaf_t;
typedef mLeaf_t* mLeaf_p;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    dClipNode_p clipnodes;
    mPlane_p    planes;
    int32_t     firstclipnode;
    int32_t     lastclipnode;
    vec3_t      clip_mins;
    vec3_t      clip_maxs;
} Hull_t;
typedef Hull_t* Hull_p;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mSpriteFrame_s {
    int32_t         width;
    int32_t         height;
    typeless_ptr    pcachespot;   // remove?
    float           up, down, left, right;
    uint8_t         pixels[4];
} mSpriteFrame_t;
typedef mSpriteFrame_t* mSpriteFrame_p;


typedef struct {
    int32_t         numframes;
    float_p         intervals;
    mSpriteFrame_p  frames[1];
} mSpriteGroup_t;
typedef mSpriteGroup_t* mSpriteGroup_p;


typedef struct {
    SpriteFrameType_t   type;
    mSpriteFrame_p      frameptr;
} mSpriteFrameDesc_t;
// typedef mSpriteFrame_t* mSpriteFrame_p;

typedef struct {
    SpriteType_t        type;
    int32_t             maxwidth;
    int32_t             maxheight;
    int32_t             numframes;
    float               beamlength;  // remove?
    typeless_ptr        cachespot;  // remove?
    mSpriteFrameDesc_t  frames[1];
} mSprite_t;
typedef mSprite_t* mSprite_p;



/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct {
    AliasFrameType_t    type;
    TriVertx_t          bboxmin;
    TriVertx_t          bboxmax;
    int32_t             frame;
    char                name[16];
} mAliasFrameDesc_t;
typedef mAliasFrameDesc_t* mAliasFrameDesc_p;


typedef struct {
    AliasSkinType_t type;
    typeless_ptr    pcachespot;
    int32_t         skin;
} mAliasSkinDesc_t;
typedef mAliasSkinDesc_t* mAliasSkinDesc_p;

typedef struct {
    TriVertx_t  bboxmin;
    TriVertx_t  bboxmax;
    int32_t     frame;
} mAliasGroupFrameDesc_t;

typedef struct {
    int32_t                 numframes;
    int32_t                 intervals;
    mAliasGroupFrameDesc_t  frames[1];
} mAliasGroup_t;
typedef mAliasGroup_t* mAliasGroup_p;

typedef struct {
    int32_t numskins;
    int32_t intervals;
    mAliasSkinDesc_t    skindescs[1];
} mAliasSkinGroup_t;
typedef mAliasSkinGroup_t* mAliasSkinGroup_p;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mTriangle_s {
    int32_t facesfront;
    int32_t vertindex[3];
} mTriangle_t;
typedef mTriangle_t* mTriangle_p;

typedef struct {
    int32_t model;
    int32_t stverts;
    int32_t skindesc;
    int32_t triangles;
    mAliasFrameDesc_t frames[1];
} AliasHdr_t;
typedef AliasHdr_t* AliasHdr_p;

//===================================================================

//
// Whole model
//

typedef enum {
    mod_brush,
    mod_sprite,
    mod_alias
} ModType_t;

#include "model_effect.h"

typedef enum {
    NL_PRESENT = 0,      // model is already loaded
    NL_NEEDS_LOADED = 1, // model must be loaded
    NL_UNREFERENCED = 2  // model is not referenced
} NeedLoad_t;


typedef struct Model_s {
    char  name[MAX_QPATH];
    NeedLoad_t needload;  // bmodels and sprites don't cache normally
    ModType_t   type;
    int32_t     numframes;
    SyncType_t  synctype;
    int32_t     flags;
    vec3_t  mins, maxs; // volume occupied by the model
    float  radius;
    int32_t firstmodelsurface, nummodelsurfaces;    // brush model
    int32_t numsubmodels;       dModel_p    submodels;
    int32_t numplanes;          mPlane_p    planes;
    int32_t numleafs;           mLeaf_p     leafs;  // number of visible leafs, not counting 0
    int32_t numvertexes;        mVertex_p   vertexes;
    int32_t numedges;           mEdge_p     edges;
    int32_t numnodes;           mNode_p     nodes;
    int32_t numtexinfo;         mTexInfo_p  texinfo;
    int32_t numsurfaces;        mSurface_p  surfaces;
    int32_t numsurfedges;       int32_p     surfedges;
    int32_t numclipnodes;       dClipNode_p clipnodes;
    int32_t nummarksurfaces;    mSurface_p* marksurfaces;
    Hull_t  hulls[MAX_MAP_HULLS];
    int32_t numtextures;        Texture_p* textures;
    uint8_p visdata;
    uint8_p lightdata;
    cstring entities;
    // additional model data
    cache_user_t cache;  // only access through Mod_Extradata

} Model_t;
typedef Model_t* Model_p;

//============================================================================

void Mod_Init();
void Mod_ClearAll();
Model_p Mod_ForName(cstring name, bool crash);
typeless_ptr Mod_Extradata(Model_p mod); // handles caching
void Mod_TouchModel(cstring name);

mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model);
uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model);

