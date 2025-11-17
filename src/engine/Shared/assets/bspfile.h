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
#include "vector.h"
#include "types.h"

// upper design bounds


// #define MAX_MAP_MODELS      (256)
// #define MAX_MAP_BRUSHES     (4096)
#define MAX_MAP_ENTITIES    (1024)
// #define MAX_MAP_ENTSTRING   (65536)

// #define MAX_MAP_PLANES          (32767)
// #define MAX_MAP_NODES           (32767)  // because negative shorts are contents
// #define MAX_MAP_CLIPNODES       (32767)  //
#define MAX_MAP_LEAFS           (8192)
// #define MAX_MAP_VERTS           (65535)
// #define MAX_MAP_FACES           (65535)
// #define MAX_MAP_MARKSURFACES    (65535)
// #define MAX_MAP_TEXINFO         (4096)
// #define MAX_MAP_EDGES           (256000)
// #define MAX_MAP_SURFEDGES       (512000)
// #define MAX_MAP_TEXTURES        (512)
// #define MAX_MAP_MIPTEX          (0x200000)
// #define MAX_MAP_LIGHTING        (0x100000)
// #define MAX_MAP_VISIBILITY      (0x100000)

// #define MAX_MAP_PORTALS  (65536)

// key / value pair sizes

// #define MAX_KEY     (32)
// #define MAX_VALUE   (1024)

//=============================================================================


#define BSPVERSION  (29)
#define TOOLVERSION (2)



typedef struct {
    int32_t nummiptex;
    int32_t dataofs[4]; // [nummiptex]
} dMipTexLump_t;
typedef dMipTexLump_t* dmiptexlump_p;


typedef struct MipTex_s {
    char        name[16];
    uint32_t    width, height;
    uint32_t    offsets[MIPLEVELS];  // four mip maps stored
} MipTex_t;
typedef MipTex_t* MipTex_p;


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

#define TEX_SPECIAL  1  // sky or slime, no lightmap or 256 subdivision
typedef struct {
    int16_t planenum;
    int16_t side;

    int32_t firstedge;  // we must support > 64k edges
    int16_t numedges;
    int16_t texinfo;

    // lighting info
    uint8_t styles[MAXLIGHTMAPS];
    int32_t lightofs;   // start of [numstyles*surfsize] samples
} dFace_t;
typedef dFace_t* dFace_p;



//============================================================================

// #ifndef QUAKE_GAME
// #warning QUAKE_GAME NOT DEFINED!!!
// #define ANGLE_UP -1
// #define ANGLE_DOWN -2


// the utilities get to be lazy and just use large static arrays

// extern int32_t  nummodels;
// extern dModel_t dmodels[MAX_MAP_MODELS];

// extern int32_t  visdatasize;
// extern uint8_t  dvisdata[MAX_MAP_VISIBILITY];

// extern int32_t  lightdatasize;
// extern uint8_t  dlightdata[MAX_MAP_LIGHTING];

// extern int32_t  texdatasize;
// extern uint8_t  dtexdata[MAX_MAP_MIPTEX]; // (dMipTexLump_t)

// extern int32_t  entdatasize;
// extern char     dentdata[MAX_MAP_ENTSTRING];

// extern int32_t  numleafs;
// extern dLeaf_t  dleafs[MAX_MAP_LEAFS];

// extern int32_t  numplanes;
// extern dPlane_t dplanes[MAX_MAP_PLANES];

// extern int32_t   numvertexes;
// extern dVertex_t dvertexes[MAX_MAP_VERTS];

// extern int32_t  numnodes;
// extern dNode_t  dnodes[MAX_MAP_NODES];

// extern int32_t   numtexinfo;
// extern TexInfo_t texinfo[MAX_MAP_TEXINFO];

// extern int32_t  numfaces;
// extern dFace_t  dfaces[MAX_MAP_FACES];

// extern int32_t      numclipnodes;
// extern dClipNode_t  dclipnodes[MAX_MAP_CLIPNODES];

// extern int32_t  numedges;
// extern dEdge_t  dedges[MAX_MAP_EDGES];

// extern int32_t  nummarksurfaces;
// extern uint16_t dmarksurfaces[MAX_MAP_MARKSURFACES];

// extern int32_t  numsurfedges;
// extern int32_t  dsurfedges[MAX_MAP_SURFEDGES];


void DecompressVis(uint8_p in, uint8_p decompressed);
int32_t CompressVis(uint8_p vis, uint8_p dest);

void LoadBSPFile(cString filename);
void WriteBSPFile(cString filename);
void PrintBSPFileSizes();

//===============

typedef struct ePair_s ePair_t;
typedef ePair_t* ePair_p;
struct ePair_s {
    ePair_p next;
    cString key;
    cString value;
};

typedef struct {
    vec3_t  origin;
    int32_t firstbrush;
    int32_t numbrushes;
    ePair_p epairs;
} Entity_t;
typedef Entity_t* Entity_p;


extern int32_t  num_entities;
extern Entity_t entities[MAX_MAP_ENTITIES];

void ParseEntities();
void UnparseEntities();

void  SetKeyValue(Entity_p ent, cString key, cString value);
cString ValueForKey(Entity_p ent, cString key);
// will return "" if not present

vec_t FloatForKey(Entity_p ent, cString key);
void  GetVectorForKey(Entity_p ent, cString key, vec3_t vec);

ePair_p ParseEpair();

// #endif
