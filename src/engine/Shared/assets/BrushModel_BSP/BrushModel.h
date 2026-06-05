#pragma once

#include "platformdefs.h"
#include "z_cache.h"    // CacheUser_t
#include "SyncType.h"

#include "Plane.h"
#include "Leaf.h"
#include "Vertex.h"
#include "Edge.h"
#include "ClipNode.h"
#include "Hull.h"

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

typedef struct Model_s Model_t;  //  TODO: fix in render.h
typedef Model_t* Model_p;
struct Model_s {
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
} ;


#include "assert.h"
#include "Lump.h"

typedef enum {      // BSP Lumps
    LUMP_ENTITIES     = 0u,     // Mod_LoadEntities
    LUMP_PLANES       = 1u,     // Mod_LoadPlanes
    LUMP_TEXTURES     = 2u,     // Mod_LoadTextures
    LUMP_VERTEXES     = 3u,     // Mod_LoadVertexes
    LUMP_VISIBILITY   = 4u,     // Mod_LoadVisibility
    LUMP_NODES        = 5u,     // Mod_LoadNodes
    LUMP_TEXINFO      = 6u,     // Mod_LoadTexinfo
    LUMP_FACES        = 7u,     // Mod_LoadFaces
    LUMP_LIGHTING     = 8u,     // Mod_LoadLighting
    LUMP_CLIPNODES    = 9u,     // Mod_LoadClipnodes
    LUMP_LEAFS        = 10u,    // Mod_LoadLeafs
    LUMP_MARKSURFACES = 11u,    // Mod_LoadMarksurfaces
    LUMP_EDGES        = 12u,    // Mod_LoadEdges
    LUMP_SURFEDGES    = 13u,    // Mod_LoadSurfedges
    LUMP_MODELS       = 14u,    // Mod_LoadSubmodels

    HEADER_LUMPS      = 15u  // total count of lumps in BSP header
} LumpType;

typedef struct {
    int32_t version;
    Lump_t  lumps[HEADER_LUMPS];    // LumpType
} dHeader_t;
typedef dHeader_t* dHeader_p;
STATIC_ASSERT_SIZE(dHeader_t, 4 + 15*8 ); // 124

#ifdef GLQUAKE
#   define _loadModel loadmodel
#endif
extern Model_p _loadModel;

#ifdef __cplusplus
extern "C" {
#endif

    void Mod_Init();
    void Mod_ClearAll();

    Model_p Mod_ForName(cString name, bool crash);
    TypeLess_ptr Mod_Extradata(Model_p mod); // handles caching

    mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model);
    uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model);
    void Mod_TouchModel(cString name);
    void Mod_Print();

    void    PrintFrameName(Model_p mdl, int frame);

#ifdef __cplusplus
}
#endif