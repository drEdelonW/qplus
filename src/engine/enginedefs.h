#pragma once

#include "platformdefs.h"
typedef char fsPathStr_t[MAX_OSPATH];   // filesystem pathname

typedef char qPathStr_t[MAX_QPATH];     // quake game pathname

#define NAME_LENGTH         64
typedef char nameStr_t[NAME_LENGTH];

#define	MAX_STYLESTRING	64
typedef char styleStr_t[MAX_STYLESTRING];

//
// per-level limits
//

#define MAX_LIGHTSTYLES     64
#define MAX_MODELS          256   /* these are sent over the net as bytes */

#define MAX_FILES_IN_PACK   2048

#define MAXLIGHTMAPS        4

#define SAVEGAME_COMMENT_LENGTH 39
typedef char saveComment_t[SAVEGAME_COMMENT_LENGTH + 1];

#ifdef STM32
#   define MAX_PARTICLES           200 /* default max # of particles at one time */
#else
#   define MAX_PARTICLES           2048 /* default max # of particles at one time */
#endif
#define ABSOLUTE_MIN_PARTICLES  512  /* no fewer than this no matter what's on the command line */

#define MAX_EFRAGS              640

#define MAX_MAPSTRING           2048
typedef char mapStr_t[MAX_MAPSTRING];

#define MAX_DEMOS               8
#define MAX_DEMONAME            16

#define MAX_TEMP_ENTITIES       64   /* lightning bolts, etc */
#define MAX_STATIC_ENTITIES     128   /* torches, etc */

#define MAX_VISEDICTS           256

#if 0   // NOT USED?
//=============================================================================
// upper design bounds

#define TOOLVERSION (2)

#define MAX_MAP_MODELS      (256)
#define MAX_MAP_BRUSHES     (4096)
#define MAX_MAP_ENTITIES    (1024)
#define MAX_MAP_ENTSTRING   (65536)

#define MAX_MAP_PLANES          (32767)
#define MAX_MAP_NODES           (32767)  // because negative shorts are contents
#define MAX_MAP_CLIPNODES       (32767)  //
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


//============================================================================

#ifndef QUAKE_GAME
#warning QUAKE_GAME NOT DEFINED!!!
#define ANGLE_UP -1
#define ANGLE_DOWN -2


// the utilities get to be lazy and just use large static arrays

extern int32_t  nummodels;
extern dModel_t dmodels[MAX_MAP_MODELS];

extern int32_t  visdatasize;
extern uint8_t  dvisdata[MAX_MAP_VISIBILITY];

extern int32_t  lightdatasize;
extern uint8_t  dlightdata[MAX_MAP_LIGHTING];

extern int32_t  texdatasize;
extern uint8_t  dtexdata[MAX_MAP_MIPTEX]; // (dMipTexLump_t)

extern int32_t  entdatasize;
extern char     dentdata[MAX_MAP_ENTSTRING];

extern int32_t  numleafs;
extern dLeaf_t  dleafs[MAX_MAP_LEAFS];

extern int32_t  numplanes;
extern dPlane_t dplanes[MAX_MAP_PLANES];

extern int32_t   numvertexes;
extern dVertex_t dvertexes[MAX_MAP_VERTS];

extern int32_t  numnodes;
extern dNode_t  dnodes[MAX_MAP_NODES];

extern int32_t   numtexinfo;
extern TexInfo_t texinfo[MAX_MAP_TEXINFO];

extern int32_t  numfaces;
extern dFace_t  dfaces[MAX_MAP_FACES];

extern int32_t      numclipnodes;
extern dClipNode_t  dclipnodes[MAX_MAP_CLIPNODES];

extern int32_t  numedges;
extern dEdge_t  dedges[MAX_MAP_EDGES];

extern int32_t  nummarksurfaces;
extern uint16_t dmarksurfaces[MAX_MAP_MARKSURFACES];

extern int32_t  numsurfedges;
extern int32_t  dsurfedges[MAX_MAP_SURFEDGES];


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
will return "" if not present

vec_t FloatForKey(Entity_p ent, cString key);
void  GetVectorForKey(Entity_p ent, cString key, vec3_t vec);

ePair_p ParseEpair();
#endif

#endif
