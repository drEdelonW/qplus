#pragma once

#include "vector.h"
#include "types.h"

#include "Plane.h"
#include "Leaf.h"
#include "Vertex.h"
#include "Edge.h"
#include "ClipNode.h"
#include "Hull.h"

#include "platformdefs.h"
#include "zone.h"

#define MAX_MAP_HULLS   (4)
#define ALIAS_VERSION   6

#define IDPOLYHEADER    (uint32_t)(('O' << 24) + ('P' << 16) + ('D' << 8) + 'I')
// little-endian "IDPO"

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
    mod_brush,
    mod_sprite,
    mod_alias
} ModType_t;


#include "SyncType.h"
#include "z_cache.h"

typedef struct Model_s Model_t;  //  TODO: fix in render.h
typedef Model_t* Model_p;
struct Model_s {
    char        name[MAX_QPATH];
    NeedLoad_t  needload;  // bmodels and sprites don't cache normally
    ModType_t   type;
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


// must match definition in spritegn.h
typedef struct {
    int32_t     ident;
    int32_t     version;
    vec3_t      scale;
    vec3_t      scale_origin;
    float       boundingradius;
    vec3_t      eyeposition;
    int32_t     numskins;
    int32_t     skinwidth;
    int32_t     skinheight;
    int32_t     numverts;
    int32_t     numtris;
    int32_t     numframes;
    SyncType_t  synctype;
    int32_t     flags;
    float       size;
} Mdl_t;
typedef Mdl_t* Mdl_p;

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


