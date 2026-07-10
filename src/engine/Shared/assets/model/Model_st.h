#pragma once

#include "vector.h"
#include "BBox.h"
#include "types.h"

#include "z_cache.h"    // CacheUser_t
// #include "Plane.h"
#include "Leaf.h"
#include "Vertex.h"
#include "Edge.h"
#include "ClipNode.h"
#include "Hull.h"

#include "enginedefs.h"
#include "SyncType.h"
#include "TexInfo.h"

#define MAX_MAP_HULLS   (4)

//
// Whole model
//
typedef struct {
#if 0   // TODO: rework to BoundingBox
    vec3_t  mins;
    vec3_t  maxs;
#else
    BBox_t  bb;
#endif
    vec3_t  origin;

    int32_t headnode[MAX_MAP_HULLS];
    int32_t visleafs;  // not including the solid leaf 0
    int32_t firstface;
    int32_t numfaces;
} dModel_t;     STATIC_ASSERT_SIZE(dModel_t, 6*4 + 3*4 + 4*4 + 3*4); // 64
typedef dModel_t* dModel_p;

typedef enum {
    NL_PRESENT = 0u, // model is already loaded
    NL_NEEDS_LOADED, // model must be loaded
    NL_UNREFERENCED  // model is not referenced
} NeedLoad_t;

typedef enum {
    mod_brush,  //  .bsp
    mod_sprite, //  .spr
    mod_alias   //  .mdl
} ModType_t;

struct Model_s {
    qPath_t     name;
    NeedLoad_t  needload;   // bmodels and sprites don't cache normally
    ModType_t   type;       // kind of content
    int32_t     numframes;
    SyncType_t  synctype;
    int32_t     flags;

    BBox_t      BB;     // volume occupied by the model
    float       radius;
#ifdef GLQUAKE
    bool        clipbox;    // solid volume for clipping
    BBox_t      clip;
#endif
    uint32_t numModelSurfaces;   uint32_t    firstModelSurface;     // brush model
    uint32_t numSubModels;       dModel_p    SubModels;
    uint32_t numplanes;          mPlane_p    planes;
    uint32_t numleafs;           mLeaf_p     leafs;         // number of visible leafs, not counting 0
    uint32_t numvertexes;        mVertex_p   vertexes;
    uint32_t numedges;           mEdge_p     edges;
    uint32_t numnodes;           mNode_p     nodes;
    uint32_t numtexinfo;         mTexInfo_p  texinfo;
    uint32_t numsurfaces;        mSurface_p  surfaces;
    uint32_t numsurfedges;       int32_p     surfedges;     // TODO: find type of surfedges index
    uint32_t numclipnodes;       dClipNode_p clipnodes;
    uint32_t nummarksurfaces;    mSurface_ar marksurfaces;
    uint32_t numtextures;        Texture_ar  textures;

    Hull_t  hulls[MAX_MAP_HULLS];
    uint8_p visdata;
    uint8_p lightdata;  // TODO: pointer get index type
    cString entities;
    // additional model data
    CacheUser_t cache;  // only access through Mod_Extradata
};
// must match definition in spritegn.h
