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

#include "Vertex.h"



#define TOP_RANGE  (16)   // soldier uniform colors
#define BOTTOM_RANGE (96)
/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/



typedef enum {
    SIDE_FRONT = 0, // point is in front of plane
    SIDE_BACK  = 1, // point is behind plane
    SIDE_ON    = 2  // point is on plane
} Side_t;




// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    uint16_t v[2];
    uint32_t cachededgeoffset;
} mEdge_t;
typedef mEdge_t* mEdge_p;



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
    TypeLess_ptr    pcachespot;   // remove?
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
    TypeLess_ptr        cachespot;  // remove?
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
    TypeLess_ptr    pcachespot;
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


typedef struct Model_s Model_t;
typedef Model_t* Model_p;
struct Model_s {
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
    cString entities;
    // additional model data
    CacheUser_t cache;  // only access through Mod_Extradata
} ;

//============================================================================

void Mod_Init();
void Mod_ClearAll();
Model_p Mod_ForName(cString name, bool crash);
TypeLess_ptr Mod_Extradata(Model_p mod); // handles caching
void Mod_TouchModel(cString name);

mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model);
uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model);

