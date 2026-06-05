#pragma once

#include "vector.h"
#include "types.h"

#include "z_cache.h"    // CacheUser_t
#include "Plane.h"
#include "Leaf.h"
#include "Vertex.h"
#include "Edge.h"
#include "ClipNode.h"
#include "Hull.h"

#include "platformdefs.h"
// #include "zone.h"

#include "SyncType.h"


#define ALIAS_VERSION   6
#define MAX_MAP_HULLS   (4)




//
// Whole model
//
typedef struct {
    vec3_t  mins;
    vec3_t  maxs;
    vec3_t  origin;

    int32_t headnode[MAX_MAP_HULLS];
    int32_t visleafs;  // not including the solid leaf 0
    int32_t firstface, numfaces;
} dModel_t;
typedef dModel_t* dModel_p;

typedef enum {
    NL_PRESENT      = 0u, // model is already loaded
    NL_NEEDS_LOADED = 1u, // model must be loaded
    NL_UNREFERENCED = 2u  // model is not referenced
} NeedLoad_t;

typedef enum {
    mod_brush,  //  .BSP
    mod_sprite, //  .spr
    mod_alias   //  .mdl
} ModType_t;

typedef struct {
    char        name[MAX_QPATH];
    NeedLoad_t  needload;  // bmodels and sprites don't cache normally
    ModType_t   type;       // TODO: check is it needed?
    int32_t     numframes;
    SyncType_t  synctype;
    int32_t     flags;
    vec3_t  mins, maxs; // volume occupied by the model
    float   radius;
#ifdef GLQUAKE
    bool    clipbox;    // solid volume for clipping
    vec3_t  clipmins;
    vec3_t  clipmaxs;
#endif
    uint32_t numModelSurfaces,   firstModelSurface;    // brush model
    uint32_t numSubModels;       dModel_p    SubModels;
    uint32_t numplanes;          mPlane_p    planes;
    uint32_t numleafs;           mLeaf_p     leafs;  // number of visible leafs, not counting 0
    uint32_t numvertexes;        mVertex_p   vertexes;
    uint32_t numedges;           mEdge_p     edges;
    uint32_t numnodes;           mNode_p     nodes;
    uint32_t numtexinfo;         mTexInfo_p  texinfo;
    uint32_t numsurfaces;        mSurface_p  surfaces;
    uint32_t numsurfedges;       int32_p     surfedges;
    uint32_t numclipnodes;       dClipNode_p clipnodes;
    uint32_t nummarksurfaces;    mSurface_p* marksurfaces;
    Hull_t  hulls[MAX_MAP_HULLS];
    int32_t numtextures;         Texture_p* textures;
    uint8_p visdata;
    uint8_p lightdata;
    cString entities;
    // additional model data
    CacheUser_t cache;  // only access through Mod_Extradata
} Model_t;
typedef Model_t* Model_p;


// must match definition in spritegn.h



