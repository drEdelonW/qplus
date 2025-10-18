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

#ifndef __MODEL__
#define __MODEL__

#include "modelgen.h"
#include "spritegn.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

// entity effects

#define EF_BRIGHTFIELD   1
#define EF_MUZZLEFLASH    2
#define EF_BRIGHTLIGHT    4
#define EF_DIMLIGHT    8


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

typedef enum {
    SIDE_FRONT = 0, // point is in front of plane
    SIDE_BACK = 1, // point is behind plane
    SIDE_ON = 2  // point is on plane
} Side_t;


// Plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mPlane_s {
    vec3_t normal;
    float dist;
    byte type;   // for texture axis selection and fast side tests
    byte signbits;  // signx + signy<<1 + signz<<1
    byte pad[2];
} mPlane_t;
typedef mPlane_t* mPlane_p;

typedef struct Texture_s Texture_t;
typedef Texture_t* Texture_p;
typedef struct Texture_s {
    char        name[16];
    unsigned    width, height;
    int         gl_texturenum;
    struct mSurface_s* texturechain; // for gl_texsort drawing
    int         anim_total;    // total tenths in sequence ( 0 = no)
    int         anim_min, anim_max;  // time for this frame min <=time< max
    Texture_p   anim_next;  // in the animation sequence
    Texture_p   alternate_anims; // bmodels in frmae 1 use these
    unsigned    offsets[MIPLEVELS];  // four mip maps stored
} Texture_t;


typedef enum {
    SURF_NONE               = 0,
    SURF_PLANEBACK          = 1u << 1, // 0x02
    SURF_DRAWSKY            = 1u << 2, // 0x04
    SURF_DRAWSPRITE         = 1u << 3, // 0x08
    SURF_DRAWTURB           = 1u << 4, // 0x10
    SURF_DRAWTILED          = 1u << 5, // 0x20
    SURF_DRAWBACKGROUND     = 1u << 6  // 0x40
} SurfaceFlags_e;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    unsigned short v[2];
    unsigned int cachededgeoffset;
} mEdge_t;

typedef struct {
    float  vecs[2][4];
    float  mipadjust;
    Texture_p texture;
    int   flags;
} mTexInfo_t;
typedef mTexInfo_t* mTexInfo_p;

#define VERTEXSIZE 7

typedef struct glpoly_s glpoly_t;
typedef glpoly_t* glpoly_p;
struct glpoly_s {
    glpoly_p next;
    glpoly_p chain;
    int  numverts;
    int  flags;   // for SURF_UNDERWATER
    float verts[4][VERTEXSIZE]; // variable sized (xyz s1t1 s2t2)
} ;

typedef struct mSurface_s {
    int   visframe;  // should be drawn when node is crossed

    mPlane_p plane;
    int   flags;

    int   firstedge; // look up in model->surfedges[], negative numbers
    int   numedges; // are backwards edges

    short  texturemins[2];
    short  extents[2];

    int   light_s, light_t; // gl lightmap coordinates

    glpoly_p polys;    // multiple if warped
    struct mSurface_s* texturechain;

    mTexInfo_p texinfo;

    // lighting info
    int   dlightframe;
    int   dlightbits;

    int   lightmaptexturenum;
    byte  styles[MAXLIGHTMAPS];
    int   cached_light[MAXLIGHTMAPS]; // values currently used in lightmap
    qboolean cached_dlight;    // true if dynamic light in cache
    byte* samples;  // [numstyles*surfsize]
} mSurface_t;
typedef mSurface_t* mSurface_p;

typedef struct mNode_s {
    // common with leaf
    int   contents;  // 0, to differentiate from leafs
    int   visframe;  // node needs to be traversed if current

    float  minmaxs[6];  // for bounding box culling

    struct mNode_s* parent;

    // node specific
    mPlane_p plane;
    struct mNode_s* children[2];

    unsigned short  firstsurface;
    unsigned short  numsurfaces;
} mNode_t;



typedef struct mLeaf_s {
    // common with node
    int   contents;  // wil be a negative contents number
    int   visframe;  // node needs to be traversed if current

    float  minmaxs[6];  // for bounding box culling

    struct mNode_s* parent;

    // leaf specific
    byte* compressed_vis;
    efrag_p efrags;

    mSurface_p* firstmarksurface;
    int   nummarksurfaces;
    int   key;   // BSP sequence number for leaf's contents
    byte  ambient_sound_level[NUM_AMBIENTS];
} mLeaf_t;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    dClipNode_p clipnodes;
    mPlane_p planes;
    int   firstclipnode;
    int   lastclipnode;
    vec3_t  clip_mins;
    vec3_t  clip_maxs;
} Hull_t;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mSpriteFrame_s {
    int  width;
    int  height;
    float up, down, left, right;
    int  gl_texturenum;
} mSpriteFrame_t;
typedef mSpriteFrame_t* mSpriteFrame_p;

typedef struct {
    int    numframes;
    float_p intervals;
    mSpriteFrame_p frames[1];
} mSpriteGroup_t;

typedef struct {
    SpriteFrameType_t type;
    mSpriteFrame_p frameptr;
} mSpriteFrameDesc_t;

typedef struct {
    int     type;
    int     maxwidth;
    int     maxheight;
    int     numframes;
    float    beamlength;  // remove?
    TypeLess_ptr cachespot;  // remove?
    mSpriteFrameDesc_t frames[1];
} mSprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct {
    int     firstpose;
    int     numposes;
    float    interval;
    TriVertx_t   bboxmin;
    TriVertx_t   bboxmax;
    int     frame;
    char    name[16];
} mAliasFrameDesc_t;

typedef struct {
    TriVertx_t   bboxmin;
    TriVertx_t   bboxmax;
    int     frame;
} mAliasGroupFrameDesc_t;

typedef struct {
    int      numframes;
    int      intervals;
    mAliasGroupFrameDesc_t frames[1];
} mAliasGroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mTriangle_s {
    int     facesfront;
    int     vertindex[3];
} mTriangle_t;


#define MAX_SKINS 32
typedef struct {
    int     ident;
    int     version;
    vec3_t  scale;
    vec3_t  scale_origin;
    float   boundingradius;
    vec3_t  eyeposition;
    int     numskins;
    int     skinwidth;
    int     skinheight;
    int     numverts;
    int     numtris;
    int     numframes;
    SyncType_t synctype;
    int     flags;
    float   size;

    int     numposes;
    int     poseverts;
    int     posedata; // numposes*poseverts trivert_t
    int     commands; // gl command list with embedded s/t
    int     gl_texturenum[MAX_SKINS][4];
    int     texels[MAX_SKINS]; // only for player skins
    mAliasFrameDesc_t frames[1]; // variable sized
} AliasHdr_t;
typedef AliasHdr_t* AliasHdr_p;

#define MAXALIASVERTS 1024
#define MAXALIASFRAMES 256
#define MAXALIASTRIS 2048
extern AliasHdr_p pheader;
extern stvert_t stverts[MAXALIASVERTS];
extern mTriangle_t triangles[MAXALIASTRIS];
extern TriVertx_p poseverts[MAXALIASFRAMES];

//===================================================================

//
// Whole model
//

typedef enum { mod_brush, mod_sprite, mod_alias } ModType_t;

#define EF_ROCKET 1   // leave a trail
#define EF_GRENADE 2   // leave a trail
#define EF_GIB  4   // leave a trail
#define EF_ROTATE 8   // rotate (bonus items)
#define EF_TRACER 16   // green split trail
#define EF_ZOMGIB 32   // small blood trail
#define EF_TRACER2 64   // orange split trail + rotate
#define EF_TRACER3 128   // purple trail

typedef struct Model_s {
    char  name[MAX_QPATH];
    qboolean needload;  // bmodels and sprites don't cache normally

    ModType_t type;
    int   numframes;
    SyncType_t synctype;

    int   flags;

    //
    // volume occupied by the model graphics
    //  
    vec3_t  mins, maxs;
    float  radius;

    //
    // solid volume for clipping 
    //
    qboolean clipbox;
    vec3_t  clipmins, clipmaxs;

    //
    // brush model
    //
    int   firstmodelsurface, nummodelsurfaces;

    int   numsubmodels;
    dModel_p submodels;

    int   numplanes;
    mPlane_p planes;

    int   numleafs;  // number of visible leafs, not counting 0
    mLeaf_p leafs;

    int   numvertexes;
    mVertex_p vertexes;

    int   numedges;
    mEdge_p edges;

    int   numnodes;
    mNode_p nodes;

    int   numtexinfo;
    mTexInfo_p texinfo;

    int   numsurfaces;
    mSurface_p surfaces;

    int   numsurfedges;
    int* surfedges;

    int   numclipnodes;
    dClipNode_p clipnodes;

    int   nummarksurfaces;
    mSurface_p* marksurfaces;

    Hull_t  hulls[MAX_MAP_HULLS];

    int   numtextures;
    Texture_p* textures;

    byte* visdata;
    byte* lightdata;
    char* entities;

    //
    // additional model data
    //
    CacheUser_t cache;  // only access through Mod_Extradata

} Model_t;

//============================================================================

void Mod_Init(void);
void Mod_ClearAll(void);
Model_p Mod_ForName(char* name, qboolean crash);
TypeLess_ptr Mod_Extradata(Model_p mod); // handles caching
void Mod_TouchModel(char* name);

mLeaf_p Mod_PointInLeaf(float_p p, Model_p model);
byte* Mod_LeafPVS(mLeaf_p leaf, Model_p model);

#endif // __MODEL__
