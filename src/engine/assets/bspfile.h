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


#if 0
#   include "quakedef.h"
#else
#   include "enginedefs.h"
#endif

// upper design bounds

#define MAX_MAP_HULLS       (4)

#define MAX_MAP_MODELS      (256)
#define MAX_MAP_BRUSHES     (4096)
#define MAX_MAP_ENTITIES    (1024)
#define MAX_MAP_ENTSTRING   (65536)

#define MAX_MAP_PLANES          (32767)
#define MAX_MAP_NODES           (32767)  // because negative shorts are contents
#define MAX_MAP_CLIPNODES       (32767)  //
#define MAX_MAP_LEAFS           (8192)
#define MAX_MAP_VERTS           (65535)
#define MAX_MAP_FACES           (65535)
#define MAX_MAP_MARKSURFACES    (65535)
#define MAX_MAP_TEXINFO         (4096)
#define MAX_MAP_EDGES           (256000)
#define MAX_MAP_SURFEDGES       (512000)
#define MAX_MAP_TEXTURES        (512)
#define MAX_MAP_MIPTEX          (0x200000)
#define MAX_MAP_LIGHTING        (0x100000)
#define MAX_MAP_VISIBILITY      (0x100000)

#define MAX_MAP_PORTALS  (65536)

// key / value pair sizes

#define MAX_KEY     (32)
#define MAX_VALUE   (1024)

//=============================================================================


#define BSPVERSION  (29)
#define TOOLVERSION (2)

typedef struct {
    int32_t fileofs, filelen;
} lump_t;
typedef lump_t* lump_p;

typedef enum {
    LUMP_ENTITIES     = 0,
    LUMP_PLANES       = 1,
    LUMP_TEXTURES     = 2,
    LUMP_VERTEXES     = 3,
    LUMP_VISIBILITY   = 4,
    LUMP_NODES        = 5,
    LUMP_TEXINFO      = 6,
    LUMP_FACES        = 7,
    LUMP_LIGHTING     = 8,
    LUMP_CLIPNODES    = 9,
    LUMP_LEAFS        = 10,
    LUMP_MARKSURFACES = 11,
    LUMP_EDGES        = 12,
    LUMP_SURFEDGES    = 13,
    LUMP_MODELS       = 14,

    HEADER_LUMPS      = 15  // total count of lumps in BSP header
} lump_type_t;

typedef struct {
    vec3_t  mins;
    vec3_t  maxs;
    vec3_t  origin;

    int32_t headnode[MAX_MAP_HULLS];
    int32_t visleafs;  // not including the solid leaf 0
    int32_t firstface, numfaces;
} dModel_t;
typedef dModel_t* dModel_p;

typedef struct {
    int32_t version;
    lump_t  lumps[HEADER_LUMPS];
} dheader_t;
typedef dheader_t* dheader_p;

typedef struct {
    int32_t nummiptex;
    int32_t dataofs[4]; // [nummiptex]
} dmiptexlump_t;
typedef dmiptexlump_t* dmiptexlump_p;


typedef struct miptex_s {
 char        name[16];
 uint32_t    width, height;
 uint32_t    offsets[MIPLEVELS];  // four mip maps stored
} miptex_t;
typedef miptex_t* miptex_p;


typedef struct {
    vec3_t point;
} dVertex_t;
typedef dVertex_t* dVertex_p;


typedef enum {
    // 0–2 are axial planes
    PLANE_X = 0,
    PLANE_Y = 1,
    PLANE_Z = 2,

    // 3–5 are non-axial planes snapped to the nearest
    PLANE_ANYX = 3,
    PLANE_ANYY = 4,
    PLANE_ANYZ = 5
} PlaneType_t;

typedef struct {
    vec3_t  normal;
    float   dist;
    PlaneType_t    type;  // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dPlane_t;
typedef dPlane_t* dPlane_p;



typedef enum {
    CONTENTS_EMPTY        = -1,
    CONTENTS_SOLID        = -2,
    CONTENTS_WATER        = -3,
    CONTENTS_SLIME        = -4,
    CONTENTS_LAVA         = -5,
    CONTENTS_SKY          = -6,
    CONTENTS_ORIGIN       = -7,  // removed at CSG time
    CONTENTS_CLIP         = -8,  // changed to CONTENTS_SOLID

    CONTENTS_CURRENT_0    = -9,
    CONTENTS_CURRENT_90   = -10,
    CONTENTS_CURRENT_180  = -11,
    CONTENTS_CURRENT_270  = -12,
    CONTENTS_CURRENT_UP   = -13,
    CONTENTS_CURRENT_DOWN = -14
} contents_t;


// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    int32_t     planenum;
    int16_t     children[2]; // negative numbers are -(leafs+1), not nodes
    int16_t     mins[3];  // for sphere culling
    int16_t     maxs[3];
    uint16_t    firstface;
    uint16_t    numfaces; // counting both sides
} dNode_t;
typedef dNode_t* dNode_p;


typedef struct {
    int32_t planenum;
    int16_t children[2]; // negative numbers are contents
} dClipNode_t;
typedef dClipNode_t* dClipNode_p;


typedef struct TexInfo_s {
    float   vecs[2][4];  // [s/t][xyz offset]
    int32_t miptex;
    int32_t flags;
} TexInfo_t;
typedef TexInfo_t* TexInfo_p;
#define TEX_SPECIAL  1  // sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct {
    uint16_t    v[2];  // Vertex numbers
} dEdge_t;
typedef dEdge_t* dEdge_p;

typedef struct {
    int16_t planenum;
    int16_t side;

    int32_t firstedge;  // we must support > 64k edges
    int16_t numedges;
    int16_t texinfo;

    // lighting info
    uint8_t styles[MAXLIGHTMAPS];
    int32_t lightofs;   // start of [numstyles*surfsize] samples
} dface_t;
typedef dface_t* dface_p;




// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct {
 int32_t contents;
 int32_t visofs;    // -1 = no visibility info

 int16_t mins[3];   // for frustum culling
 int16_t maxs[3];

 uint16_t    firstmarksurface;
 uint16_t    nummarksurfaces;

 uint8_t ambient_level[NUM_AMBIENTS];
} dLeaf_t;
typedef dLeaf_t* dLeaf_p;


//============================================================================

#ifndef QUAKE_GAME
// #warning QUAKE_GAME NOT DEFINED!!!
#define ANGLE_UP -1
#define ANGLE_DOWN -2


// the utilities get to be lazy and just use large static arrays

extern int32_t   nummodels;
extern dModel_t dmodels[MAX_MAP_MODELS];

extern int32_t   visdatasize;
extern uint8_t  dvisdata[MAX_MAP_VISIBILITY];

extern int32_t   lightdatasize;
extern uint8_t  dlightdata[MAX_MAP_LIGHTING];

extern int32_t   texdatasize;
extern uint8_t  dtexdata[MAX_MAP_MIPTEX]; // (dmiptexlump_t)

extern int32_t   entdatasize;
extern char  dentdata[MAX_MAP_ENTSTRING];

extern int32_t   numleafs;
extern dLeaf_t  dleafs[MAX_MAP_LEAFS];

extern int32_t   numplanes;
extern dPlane_t dplanes[MAX_MAP_PLANES];

extern int32_t   numvertexes;
extern dVertex_t dvertexes[MAX_MAP_VERTS];

extern int32_t   numnodes;
extern dNode_t  dnodes[MAX_MAP_NODES];

extern int32_t   numtexinfo;
extern TexInfo_t texinfo[MAX_MAP_TEXINFO];

extern int32_t   numfaces;
extern dface_t  dfaces[MAX_MAP_FACES];

extern int32_t   numclipnodes;
extern dClipNode_t dclipnodes[MAX_MAP_CLIPNODES];

extern int32_t   numedges;
extern dEdge_t  dedges[MAX_MAP_EDGES];

extern int32_t   nummarksurfaces;
extern uint16_t dmarksurfaces[MAX_MAP_MARKSURFACES];

extern int32_t   numsurfedges;
extern int32_t   dsurfedges[MAX_MAP_SURFEDGES];


void DecompressVis(uint8_p in, uint8_p decompressed);
int32_t CompressVis(uint8_p vis, uint8_p dest);

void LoadBSPFile(cstring filename);
void WriteBSPFile(cstring filename);
void PrintBSPFileSizes();

//===============


typedef struct epair_s {
 struct epair_s* next;
 cstring key;
 cstring value;
} epair_t;

typedef struct {
 vec3_t  origin;
 int32_t firstbrush;
 int32_t numbrushes;
 epair_t* epairs;
} Entity_t;

extern int32_t   num_entities;
extern Entity_t entities[MAX_MAP_ENTITIES];

void ParseEntities();
void UnparseEntities();

void  SetKeyValue(Entity_t* ent, cstring key, cstring value);
cstring ValueForKey(Entity_t* ent, cstring key);
// will return "" if not present

vec_t FloatForKey(Entity_t* ent, cstring key);
void  GetVectorForKey(Entity_t* ent, cstring key, vec3_t vec);

epair_t* ParseEpair();

#endif
