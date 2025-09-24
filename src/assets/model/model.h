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

#include "modelgen.h"
#include "spritegn.h"

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

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s{
	vec3_t	normal;
	float	dist;
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;
typedef mplane_t* mplane_p;

struct texture_s;
typedef struct texture_s texture_t;
typedef texture_t* texture_p;
struct texture_s{
	char		name[16];
	unsigned	width, height;
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	texture_p 	anim_next;		// in the animation sequence
	texture_p 	alternate_anims;	// bmodels in frmae 1 use these
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
};


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
	uint16_t	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct {
	float		vecs[2][4];
	float		mipadjust;
	texture_p	texture;
	int			flags;
} mtexinfo_t;

typedef struct msurface_s{
	int			visframe;		// should be drawn when node is crossed

	int			dlightframe;
	int			dlightbits;

	mplane_p	plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges

// surface generation data
	struct surfcache_s*	cachespots[MIPLEVELS];

	int16_t		texturemins[2];
	int16_t		extents[2];

	mtexinfo_t*	texinfo;

// lighting info
	byte		styles[MAXLIGHTMAPS];
	byte*		samples;		// [numstyles*surfsize]
} msurface_t;
typedef msurface_t* msurface_p;

struct mnode_s;
typedef struct mnode_s mnode_t;
typedef mnode_t* mnode_p;
struct mnode_s{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current

	int16_t		minmaxs[6];		// for bounding box culling

	mnode_p	    parent;

// node specific
	mplane_p	plane;
	mnode_p     children[2];

	uint16_t		firstsurface;
	uint16_t		numsurfaces;
};



typedef struct mleaf_s{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	int16_t		minmaxs[6];		// for bounding box culling

	mnode_p     parent;

// leaf specific
	byte*		compressed_vis;
	efrag_t*	efrags;

	msurface_p*	firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	byte		ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;
typedef mleaf_t* mleaf_p;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
	dclipnode_p  clipnodes;
	mplane_p   planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
} hull_t;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s{
	int		width;
	int		height;
	typeless_ptr   pcachespot;			// remove?
	float	up, down, left, right;
	byte	pixels[4];
} mspriteframe_t;
typedef mspriteframe_t* mspriteframe_p;

typedef struct {
	int				numframes;
	float*          intervals;
	mspriteframe_p  frames[1];
} mspritegroup_t;

typedef struct {
	spriteframetype_t	type;
	mspriteframe_p      frameptr;
} mspriteframedesc_t;

typedef struct {
	int					type;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	float				beamlength;		// remove?
	typeless_ptr        cachespot;		// remove?
	mspriteframedesc_t	frames[1];
} msprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct {
	aliasframetype_t	type;
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
	char				name[16];
} maliasframedesc_t;
typedef maliasframedesc_t* maliasframedesc_p;

typedef struct {
	aliasskintype_t type;
	typeless_ptr    pcachespot;
	int             skin;
} maliasskindesc_t;
typedef maliasskindesc_t* maliasskindesc_p;

typedef struct {
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
} maliasgroupframedesc_t;

typedef struct {
	int						numframes;
	int						intervals;
	maliasgroupframedesc_t	frames[1];
} maliasgroup_t;
typedef maliasgroup_t* maliasgroup_p;

typedef struct {
	int					numskins;
	int					intervals;
	maliasskindesc_t	skindescs[1];
} maliasskingroup_t;
typedef maliasskingroup_t* maliasskingroup_p;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s{
	int facesfront;
	int	vertindex[3];
} mtriangle_t;
typedef mtriangle_t* mtriangle_p;

typedef struct {
	int					model;
	int					stverts;
	int					skindesc;
	int					triangles;
	maliasframedesc_t	frames[1];
} aliashdr_t;
typedef aliashdr_t* aliashdr_p;

//===================================================================

//
// Whole model
//

typedef enum{
    mod_brush,
    mod_sprite,
    mod_alias
} modtype_t;

#include "model_effect.h"


typedef struct model_s{
	char		name[MAX_QPATH];
	qboolean	needload;		// bmodels and sprites don't cache normally

	modtype_t	type;
	int			numframes;
	synctype_t	synctype;

	int			flags;

  //
  // volume occupied by the model
  //
	vec3_t		mins, maxs;
	float		radius;

  //
  // brush model
  //
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	dmodel_p    submodels;

	int			numplanes;
	mplane_p   planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_p   leafs;

	int			numvertexes;
	mvertex_t*  vertexes;

	int			numedges;
	medge_t*    edges;

	int			numnodes;
	mnode_t*    nodes;

	int			numtexinfo;
	mtexinfo_t* texinfo;

	int			numsurfaces;
	msurface_p  surfaces;

	int			numsurfedges;
	int*        surfedges;

	int			    numclipnodes;
	dclipnode_p     clipnodes;

	int			    nummarksurfaces;
	msurface_p *    marksurfaces;

	hull_t		hulls[MAX_MAP_HULLS];

	int			numtextures;
	texture_p* textures;

	byte*       visdata;
	byte*       lightdata;
	cstring       entities;

  //
  // additional model data
  //
	cache_user_t	cache;		// only access through Mod_Extradata

} model_t;
typedef model_t* model_p;

//============================================================================

void	Mod_Init();
void	Mod_ClearAll();
model_p Mod_ForName(cstring name, qboolean crash);
void	*Mod_Extradata(model_p mod);	// handles caching
void	Mod_TouchModel(cstring name);

mleaf_p Mod_PointInLeaf(vec3_t p, model_p model);
byte*	Mod_LeafPVS(mleaf_p leaf, model_p model);

