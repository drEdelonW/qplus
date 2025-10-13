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

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

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
    vec3_t		position;
} mvertex_t;
typedef mvertex_t* mvertex_p;

typedef enum {
    SIDE_FRONT = 0, // point is in front of plane
    SIDE_BACK  = 1, // point is behind plane
    SIDE_ON    = 2  // point is on plane
} side_t;

// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s {
    vec3_t	normal;
    float	dist;
    uint8_t	type;			// for texture axis selection and fast side tests
    uint8_t	signbits;		// signx + signy<<1 + signz<<1
    uint8_t	pad[2];
} mplane_t;
typedef mplane_t* mplane_p;

struct texture_s;
typedef struct texture_s texture_t;
typedef texture_t* texture_p;
struct texture_s {
    char        name[16];
    uint32_t    width, height;
    int32_t     anim_total;				// total tenths in sequence ( 0 = no)
    int32_t     anim_min, anim_max;		// time for this frame min <=time< max
    texture_p   anim_next;		// in the animation sequence
    texture_p   alternate_anims;	// bmodels in frmae 1 use these
    uint32_t    offsets[MIPLEVELS];		// four mip maps stored
};


typedef enum surf_flags_e {
    SURF_NONE           = 0,
    SURF_PLANEBACK      = 1u << 1, // 0x02
    SURF_DRAWSKY        = 1u << 2, // 0x04
    SURF_DRAWSPRITE     = 1u << 3, // 0x08
    SURF_DRAWTURB       = 1u << 4, // 0x10
    SURF_DRAWTILED      = 1u << 5, // 0x20
    SURF_DRAWBACKGROUND = 1u << 6  // 0x40
} surf_flags_e;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
    uint16_t	v[2];
    uint32_t	cachededgeoffset;
} medge_t;
typedef medge_t* medge_p;

typedef struct {
    float		vecs[2][4];
    float		mipadjust;
    texture_p	texture;
    int32_t			flags;
} mtexinfo_t;
typedef mtexinfo_t* mtexinfo_p;

typedef struct msurface_s {
    int32_t			visframe;		// should be drawn when node is crossed

    int32_t			dlightframe;
    int32_t			dlightbits;

    mplane_p	plane;
    // int32_t			flags;
    surf_flags_e    flags;

    int32_t			firstedge;	// look up in model->surfedges[], negative numbers
    int32_t			numedges;	// are backwards edges

    // surface generation data
    struct surfcache_s* cachespots[MIPLEVELS];

    int16_t		texturemins[2];
    int16_t		extents[2];

    mtexinfo_p texinfo;

    // lighting info
    uint8_t		styles[MAXLIGHTMAPS];
    uint8_p samples;		// [numstyles*surfsize]
} msurface_t;
typedef msurface_t* msurface_p;

struct mnode_s;
typedef struct mnode_s mnode_t;
typedef mnode_t* mnode_p;
struct mnode_s {
    // common with leaf
    int32_t			contents;		// 0, to differentiate from leafs
    int32_t			visframe;		// node needs to be traversed if current

    int16_t		minmaxs[6];		// for bounding box culling

    mnode_p	    parent;

    // node specific
    mplane_p	plane;
    mnode_p     children[2];

    uint16_t		firstsurface;
    uint16_t		numsurfaces;
};



typedef struct mleaf_s {
    // common with node
    int32_t     contents;       // wil be a negative contents number
    int32_t     visframe;       // node needs to be traversed if current
    int16_t     minmaxs[6];     // for bounding box culling
    mnode_p     parent;
    // leaf specific
    uint8_p     compressed_vis;
    efrag_p     efrags;
    msurface_p* firstmarksurface;
    int32_t     nummarksurfaces;
    int32_t     key;            // BSP sequence number for leaf's contents
    uint8_t     ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;
typedef mleaf_t* mleaf_p;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    dclipnode_p clipnodes;
    mplane_p    planes;
    int32_t firstclipnode;
    int32_t lastclipnode;
    vec3_t  clip_mins;
    vec3_t  clip_maxs;
} hull_t;
typedef hull_t* hull_p;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s {
    int32_t width;
    int32_t height;
    typeless_ptr    pcachespot;			// remove?
    float   up, down, left, right;
    uint8_t	pixels[4];
} mspriteframe_t;
typedef mspriteframe_t* mspriteframe_p;

typedef struct {
    int32_t numframes;
    float_p intervals;
    mspriteframe_p  frames[1];
} mspritegroup_t;
typedef mspritegroup_t* mspritegroup_p;


typedef struct {
    spriteframetype_t   type;
    mspriteframe_p      frameptr;
} mspriteframedesc_t;
// typedef mspriteframe_t* mspriteframe_p;

typedef struct {
    sprite_type_t type;
    int32_t maxwidth;
    int32_t maxheight;
    int32_t numframes;
    float   beamlength;		// remove?
    typeless_ptr    cachespot;		// remove?
    mspriteframedesc_t  frames[1];
} msprite_t;
typedef msprite_t* msprite_p;



/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct {
    aliasframetype_t    type;
    trivertx_t  bboxmin;
    trivertx_t  bboxmax;
    int32_t     frame;
    char        name[16];
} maliasframedesc_t;
typedef maliasframedesc_t* maliasframedesc_p;

typedef struct {
    aliasskintype_t type;
    typeless_ptr    pcachespot;
    int32_t         skin;
} maliasskindesc_t;
typedef maliasskindesc_t* maliasskindesc_p;

typedef struct {
    trivertx_t  bboxmin;
    trivertx_t  bboxmax;
    int32_t     frame;
} maliasgroupframedesc_t;

typedef struct {
    int32_t numframes;
    int32_t intervals;
    maliasgroupframedesc_t	frames[1];
} maliasgroup_t;
typedef maliasgroup_t* maliasgroup_p;

typedef struct {
    int32_t numskins;
    int32_t intervals;
    maliasskindesc_t    skindescs[1];
} maliasskingroup_t;
typedef maliasskingroup_t* maliasskingroup_p;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s {
    int32_t facesfront;
    int32_t	vertindex[3];
} mtriangle_t;
typedef mtriangle_t* mtriangle_p;

typedef struct {
    int32_t model;
    int32_t stverts;
    int32_t skindesc;
    int32_t triangles;
    maliasframedesc_t	frames[1];
} aliashdr_t;
typedef aliashdr_t* aliashdr_p;

//===================================================================

//
// Whole model
//

typedef enum {
    mod_brush,
    mod_sprite,
    mod_alias
} modtype_t;

#include "model_effect.h"

typedef enum {
    NL_PRESENT = 0,      // model is already loaded
    NL_NEEDS_LOADED = 1, // model must be loaded
    NL_UNREFERENCED = 2  // model is not referenced
} needload_t;


typedef struct model_s {
    char		name[MAX_QPATH];
    needload_t	needload;		// bmodels and sprites don't cache normally
    modtype_t   type;
    int32_t     numframes;
    synctype_t  synctype;
    int32_t     flags;
    vec3_t		mins, maxs; // volume occupied by the model
    float		radius;
    int32_t firstmodelsurface, nummodelsurfaces;    // brush model
    int32_t numsubmodels;       dmodel_p    submodels;
    int32_t numplanes;          mplane_p    planes;
    int32_t numleafs;           mleaf_p     leafs;  // number of visible leafs, not counting 0
    int32_t numvertexes;        mvertex_p   vertexes;
    int32_t numedges;           medge_p     edges;
    int32_t numnodes;           mnode_p     nodes;
    int32_t numtexinfo;         mtexinfo_p  texinfo;
    int32_t numsurfaces;        msurface_p  surfaces;
    int32_t numsurfedges;       int32_p     surfedges;
    int32_t numclipnodes;       dclipnode_p clipnodes;
    int32_t nummarksurfaces;    msurface_p* marksurfaces;
    hull_t  hulls[MAX_MAP_HULLS];
    int32_t numtextures;        texture_p* textures;
    uint8_p visdata;
    uint8_p lightdata;
    cstring entities;
    // additional model data
    cache_user_t	cache;		// only access through Mod_Extradata

} model_t;
typedef model_t* model_p;

//============================================================================

void	Mod_Init();
void	Mod_ClearAll();
model_p Mod_ForName(cstring name, bool crash);
typeless_ptr Mod_Extradata(model_p mod);	// handles caching
void	Mod_TouchModel(cstring name);

mleaf_p Mod_PointInLeaf(vec3_t p, model_p model);
uint8_p Mod_LeafPVS(mleaf_p leaf, model_p model);

