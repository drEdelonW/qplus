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

#define MAX_MAP_HULLS       (4)
#define ALIAS_VERSION 6

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
    NL_PRESENT      = 0, // model is already loaded
    NL_NEEDS_LOADED = 1, // model must be loaded
    NL_UNREFERENCED = 2  // model is not referenced
} NeedLoad_t;

typedef enum {
    mod_brush,
    mod_sprite,
    mod_alias
} ModType_t;

typedef enum {
    ST_SYNC = 0,
    ST_RAND
} SyncType_t;

typedef struct Model_s Model_t;
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
    vec3_t  clipmins, clipmaxs;
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

typedef struct {
    int32_t fileOfs;
    int32_t fileLen;
} Lump_t;
typedef Lump_t* Lump_p;

typedef enum {
    LUMP_ENTITIES     = 0u,
    LUMP_PLANES       = 1u,
    LUMP_TEXTURES     = 2u,
    LUMP_VERTEXES     = 3u,
    LUMP_VISIBILITY   = 4u,
    LUMP_NODES        = 5u,
    LUMP_TEXINFO      = 6u,
    LUMP_FACES        = 7u,
    LUMP_LIGHTING     = 8u,
    LUMP_CLIPNODES    = 9u,
    LUMP_LEAFS        = 10u,
    LUMP_MARKSURFACES = 11u,
    LUMP_EDGES        = 12u,
    LUMP_SURFEDGES    = 13u,
    LUMP_MODELS       = 14u,

    HEADER_LUMPS      = 15u  // total count of lumps in BSP header
} LumpType;

typedef struct {
    int32_t version;
    Lump_t  lumps[HEADER_LUMPS];    // LumpType
} dHeader_t;
typedef dHeader_t* dHeader_p;


