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


/*
===============================================================================

                    BRUSHMODEL LOADING

===============================================================================
*/

#include "BrushModel.h"
#include "bspfile.h"
#include "endian_tools.h"
#include "host.h"
#include "z_hunk.h"
#include "q_tools.h"
#ifdef GLQUAKE
#   include "qOpenGL.h"
// #else
#endif
#include "render.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/*
    =================
    Mod_LoadVertexes
    =================
*/
void Mod_LoadVertexes(Lump_p l) {
    dVertex_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    mVertex_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->vertexes = out;
    _loadModel->numvertexes = count;

    for (int i = 0; i < count; i++, in++, out++) {
        out->position[0] = LittleFloat(in->point[0]);
        out->position[1] = LittleFloat(in->point[1]);
        out->position[2] = LittleFloat(in->point[2]);
    }
}


/*
    =================
    Mod_LoadEdges
    =================
*/
void Mod_LoadEdges(Lump_p l) {
    dEdge_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    mEdge_p out = Hunk_AllocName((count + 1) * sizeof(*out), Mod_loadName);

    _loadModel->edges = out;
    _loadModel->numedges = count;

    for (int i = 0; i < count; i++, in++, out++) {
        out->v[0] = (uint16_t)LittleShort(in->v[0]);
        out->v[1] = (uint16_t)LittleShort(in->v[1]);
    }
}


/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges(Lump_p l) {
    int32_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    int32_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->surfedges = out;
    _loadModel->numsurfedges = count;

    for (int i = 0; i < count; i++)
        out[i] = LittleLong(in[i]);
}


/*
=================
Mod_LoadTextures
=================
*/
void Mod_LoadTextures(Lump_p l) {
    if (!l->fileLen) { _loadModel->textures = NULL; return; }

    dMipTexLump_p m = (dMipTexLump_p)(mod_base + l->fileOfs);

    m->nummiptex = LittleLong(m->nummiptex);

    _loadModel->numtextures = m->nummiptex;
    _loadModel->textures = Hunk_AllocName(
        m->nummiptex * sizeof(*_loadModel->textures), Mod_loadName
    );

    for (int i = 0; i < m->nummiptex; i++) {
        m->dataofs[i] = LittleLong(m->dataofs[i]);
        if (m->dataofs[i] == -1)        continue;

        MipTex_p mt = (MipTex_p)((uint8_p)m + m->dataofs[i]);
        mt->width = LittleLong(mt->width);
        mt->height = LittleLong(mt->height);

        for (int j = 0; j < MIPLEVELS; j++)
            mt->offsets[j] = LittleLong(mt->offsets[j]);

        if ((mt->width & 0x0F) || (mt->height & 0x0F))      Host_SysError("Texture %s is not 16 aligned", mt->name);

        int pixels = mt->width * mt->height / 64 * 85;
        Texture_p tx = Hunk_AllocName(sizeof(Texture_t) + pixels, Mod_loadName);
        _loadModel->textures[i] = tx;

        memcpy(tx->name, mt->name, sizeof(tx->name));
        tx->width = mt->width;
        tx->height = mt->height;
        for (int j = 0; j < MIPLEVELS; j++)
            tx->offsets[j] = mt->offsets[j] + sizeof(Texture_t) - sizeof(MipTex_t);
        // the pixels immediately follow the structures
        memcpy(tx + 1, mt + 1, pixels);

        if (!Q_strncmp(mt->name, "sky", 3))
            R_InitSky(tx);
#ifdef GLQUAKE
        else {
            texture_mode = GL_LINEAR_MIPMAP_NEAREST; //_LINEAR;
            tx->gl_texturenum = GL_LoadTexture(
                mt->name,
                tx->width, tx->height,
                (uint8_p)(tx + 1),
                true, false
            );
            texture_mode = GL_LINEAR;
        }
#endif
    }

    //
    // sequence the animations
    //
    for (int i = 0; i < m->nummiptex; i++) {
        Texture_p tx = _loadModel->textures[i];
        if (!tx ||
            (tx->name[0] != '+') ||
            (tx->anim_next)
            )
            continue; // allready sequenced

        // find the number of frames in the animation
        Texture_p anims[10];    memset(anims, 0, sizeof(anims));
        Texture_p altanims[10]; memset(altanims, 0, sizeof(altanims));

        int max = tx->name[1];
        int altmax = 0;
        if ((max >= 'a') && (max <= 'z'))   max -= 'a' - 'A';

        if ((max >= '0') && (max <= '9')) {
            max -= '0';
            altmax = 0;
            anims[max] = tx;
            max++;
        }
        else
            if ((max >= 'A') && (max <= 'J')) {
                altmax = max - 'A';
                max = 0;
                altanims[altmax] = tx;
                altmax++;
            }
            else
                Host_SysError("Bad animating texture %s", tx->name);

        for (int j = (i + 1); j < m->nummiptex; j++) {
            Texture_p tx2 = _loadModel->textures[j];
            if (!tx2 ||
                (tx2->name[0] != '+') ||
                strcmp(tx2->name + 2, tx->name + 2)
                )
                continue;

            int num = tx2->name[1];
            if ((num >= 'a') && (num <= 'z'))   num -= ('a' - 'A');
            if ((num >= '0') && (num <= '9')) {
                num -= '0';
                anims[num] = tx2;
                if ((num + 1) > max)        max = num + 1;
            }
            else
                if ((num >= 'A') && (num <= 'J')) {
                    num = num - 'A';
                    altanims[num] = tx2;
                    if (num + 1 > altmax)       altmax = num + 1;
                }
                else    Host_SysError("Bad animating texture %s", tx->name);
        }

#define ANIM_CYCLE 2
        // link them all together
        for (int j = 0; j < max; j++) {
            Texture_p tx2 = anims[j];
            if (!tx2)       Host_SysError("Missing frame %i of %s", j, tx->name);

            tx2->anim_total = max * ANIM_CYCLE;
            tx2->anim_min = j * ANIM_CYCLE;
            tx2->anim_max = (j + 1) * ANIM_CYCLE;
            tx2->anim_next = anims[(j + 1) % max];
            if (altmax)
                tx2->alternate_anims = altanims[0];
        }
        for (int j = 0; j < altmax; j++) {
            Texture_p tx2 = altanims[j];
            if (!tx2)       Host_SysError("Missing frame %i of %s", j, tx->name);

            tx2->anim_total = altmax * ANIM_CYCLE;
            tx2->anim_min = j * ANIM_CYCLE;
            tx2->anim_max = (j + 1) * ANIM_CYCLE;
            tx2->anim_next = altanims[(j + 1) % altmax];
            if (max)
                tx2->alternate_anims = anims[0];
        }
    }
}

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting(Lump_p l) {
    if (!l->fileLen) { _loadModel->lightdata = NULL; return; }

    _loadModel->lightdata = Hunk_AllocName(l->fileLen, Mod_loadName);
    memcpy(_loadModel->lightdata, mod_base + l->fileOfs, l->fileLen);
}

/*
    =================
    Mod_LoadTexinfo
    =================
*/
void Mod_LoadTexinfo(Lump_p l) {
    TexInfo_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    mTexInfo_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->texinfo = out;
    _loadModel->numtexinfo = count;

    for (int i = 0; i < count; i++, in++, out++) {
        for (int j = 0; j < 8; j++)
            out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
        float len1 = Length(out->vecs[0]);
        float len2 = Length(out->vecs[1]);
        len1 = (len1 + len2) / 2;
        if (len1 < 0.32)        out->mipadjust = 4;
        else if (len1 < 0.49)   out->mipadjust = 3;
        else if (len1 < 0.99)   out->mipadjust = 2;
        else                    out->mipadjust = 1;
#if 0
        if (len1 + len2 < 0.001)    out->mipadjust = 1;  // don't crash
        else                        out->mipadjust = 1 / floor((len1 + len2) / 2 + 0.1);
#endif

        int miptex = LittleLong(in->miptex);
        out->flags = LittleLong(in->flags);

        if (!_loadModel->textures) {
            out->texture = r_notexture_mip; // checkerboard texture
            out->flags = 0;
        }
        else {
            if (miptex >= _loadModel->numtextures)       Host_SysError("miptex >= _loadModel->numtextures");

            out->texture = _loadModel->textures[miptex];
            if (!out->texture) {
                out->texture = r_notexture_mip; // texture not found
                out->flags = 0;
            }
        }
    }
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces(Lump_p l) {
    int16_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))           Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int32_t count = l->fileLen / sizeof(*in);
    mSurface_p* out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->marksurfaces = out;
    _loadModel->nummarksurfaces = count;

    for (int i = 0; i < count; i++) {
        int j = LittleShort(in[i]);
        if (j >= _loadModel->numsurfaces)    Host_SysError("Mod_ParseMarksurfaces: bad surface number");

        out[i] = _loadModel->surfaces + j;
    }
} 

/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility(Lump_p l) {
    if (!l->fileLen) { _loadModel->visdata = NULL; return; }

    _loadModel->visdata = Hunk_AllocName(l->fileLen, Mod_loadName);
    memcpy(_loadModel->visdata, mod_base + l->fileOfs, l->fileLen);
}


/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs(Lump_p l) {
    dLeaf_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    mLeaf_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->leafs = out;
    _loadModel->numleafs = count;

    for (int i = 0; i < count; i++, in++, out++) {
        for (int j = 0; j < 3; j++) {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        out->contents = LittleLong(in->contents);

        out->firstmarksurface = _loadModel->marksurfaces + LittleShort(in->firstmarksurface);
        out->nummarksurfaces = LittleShort(in->nummarksurfaces);

        int p = LittleLong(in->visofs);
        if (p == -1)    out->compressed_vis = NULL;
        else            out->compressed_vis = _loadModel->visdata + p;
        out->efrags = NULL;

        for (int j = 0; j < 4; j++)
            out->ambient_sound_level[j] = in->ambient_level[j];

#ifdef GLQAUKE
        // gl underwater warp
        if (out->contents != CONTENTS_EMPTY) {
            for (int j = 0; j < out->nummarksurfaces; j++)
                out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
        }
#endif
    }
}


/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents(mSurface_p s) {
    float mins[2], maxs[2];
    int  bmins[2], bmaxs[2];

    mins[0] = mins[1] = 999999;
    maxs[0] = maxs[1] = -99999;

    mTexInfo_p tex = s->texinfo;

    for (int i = 0; i < s->numedges; i++) {
        int e = _loadModel->surfedges[s->firstedge + i];
        mVertex_p v =
            &_loadModel->vertexes[
                (e >= 0) ?
                    _loadModel->edges[e].v[0] :
                    _loadModel->edges[-e].v[1]
            ];

        for (int j = 0; j < 2; j++) {
            float val =
                v->position[0] * tex->vecs[j][0] +
                v->position[1] * tex->vecs[j][1] +
                v->position[2] * tex->vecs[j][2] +
                /*            */ tex->vecs[j][3];
            if (val < mins[j]) mins[j] = val;
            if (val > maxs[j]) maxs[j] = val;
        }
    }

    for (int i = 0; i < 2; i++) {
        bmins[i] = floor(mins[i] / 16);
        bmaxs[i] = ceil(maxs[i] / 16);

        s->texturemins[i] = bmins[i] * 16;
        s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
        if ((!(tex->flags & TEX_SPECIAL)) &&
#ifdef GLQUAKE
            // (s->extents[i] > 512) /* 256 */
#else
#endif
        (s->extents[i] > 256)
        ) {
            Host_SysError("Bad surface extents");
        }
    }
}

/*
    =================
    Mod_LoadFaces
    =================
*/
void Mod_LoadFaces(Lump_p l) {
    dFace_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    mSurface_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->surfaces = out;
    _loadModel->numsurfaces = count;

    for (int surfnum = 0; surfnum < count; surfnum++, in++, out++) {
        out->firstedge = LittleLong(in->firstedge);
        out->numedges = LittleShort(in->numedges);
        out->flags = 0;

        int planenum = LittleShort(in->planenum);
        int side = LittleShort(in->side);
        if (side)
            out->flags |= SURF_PLANEBACK;

        out->plane = _loadModel->planes + planenum;

        out->texinfo = _loadModel->texinfo + LittleShort(in->texinfo);

        CalcSurfaceExtents(out);

        // lighting info

        for (int i = 0; i < MAXLIGHTMAPS; i++) {
            out->styles[i] = in->styles[i];
        }

        int32_t i = LittleLong(in->lightofs);
        if (i == -1)    out->samples = NULL;
        else            out->samples = _loadModel->lightdata + i;

        // set the drawing flags flag

        if (!Q_strncmp(out->texinfo->texture->name, "sky", 3)) { // sky
            out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
#ifdef GLQUAKE
#ifndef QUAKE2
            GL_SubdivideSurface(out); // cut up polygon for warps
#endif        
#endif
            continue;
        }

        if (!Q_strncmp(out->texinfo->texture->name, "*", 1)) { // turbulent
            out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
            for (int i = 0; i < 2; i++) {
                out->extents[i] = 16384;
                out->texturemins[i] = -8192;
            }
#ifdef GLQUAKE
            GL_SubdivideSurface(out); // cut up polygon for warps
#endif
            continue;
        }
    }
}


/*
    =================
    Mod_SetParent
    =================
*/
void Mod_SetParent(mNode_p node, mNode_p parent) {
    node->parent = parent;
    if (node->contents < 0) return;

    Mod_SetParent(node->children[0], node);
    Mod_SetParent(node->children[1], node);
}


/*
    =================
    Mod_LoadNodes
    =================
*/
void Mod_LoadNodes(Lump_p l) {
    dNode_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    mNode_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->nodes = out;
    _loadModel->numnodes = count;

    for (int i = 0; i < count; i++, in++, out++) {
        for (int j = 0; j < 3; j++) {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        out->plane = _loadModel->planes + LittleLong(in->planenum);

        out->firstsurface = LittleShort(in->firstface);
        out->numsurfaces = LittleShort(in->numfaces);

        for (int j = 0; j < 2; j++) {
            int p = LittleShort(in->children[j]);
            if (p >= 0) out->children[j] = _loadModel->nodes + p;
            else        out->children[j] = (mNode_p)(_loadModel->leafs + (-1 - p));
        }
    }

    Mod_SetParent(_loadModel->nodes, NULL); // sets nodes and leafs
}


/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes(Lump_p l) {
    dClipNode_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    dClipNode_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->clipnodes = out;
    _loadModel->numclipnodes = count;

    Hull_p hull;
    hull = &_loadModel->hulls[1];
    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = _loadModel->planes;
    hull->clip_mins[0] = -16;
    hull->clip_mins[1] = -16;
    hull->clip_mins[2] = -24;
    hull->clip_maxs[0] = 16;
    hull->clip_maxs[1] = 16;
    hull->clip_maxs[2] = 32;

    hull = &_loadModel->hulls[2];
    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = _loadModel->planes;
    hull->clip_mins[0] = -32;
    hull->clip_mins[1] = -32;
    hull->clip_mins[2] = -24;
    hull->clip_maxs[0] = 32;
    hull->clip_maxs[1] = 32;
    hull->clip_maxs[2] = 64;

    for (int i = 0; i < count; i++, out++, in++) {
        out->planenum = LittleLong(in->planenum);
        out->children[0] = LittleShort(in->children[0]);
        out->children[1] = LittleShort(in->children[1]);
    }
}

/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities(Lump_p l) {
    if (!l->fileLen) { _loadModel->entities = NULL; return; }

    _loadModel->entities = Hunk_AllocName(l->fileLen, Mod_loadName);
    memcpy(_loadModel->entities, mod_base + l->fileOfs, l->fileLen);
}


/*
    =================
    Mod_LoadSubmodels
    =================
*/
void Mod_LoadSubmodels(Lump_p l) {
    dModel_p in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = l->fileLen / sizeof(*in);
    dModel_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->SubModels = out;
    _loadModel->numSubModels = count;

    for (int i = 0; i < count; i++, in++, out++) {
        for (int j = 0; j < 3; j++) {    // spread the mins / maxs by a pixel
            out->mins[j] = LittleFloat(in->mins[j]) - 1;
            out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
            out->origin[j] = LittleFloat(in->origin[j]);
        }
        for (int j = 0; j < MAX_MAP_HULLS; j++)
            out->headnode[j] = LittleLong(in->headnode[j]);

        out->visleafs = LittleLong(in->visleafs);
        out->firstface = LittleLong(in->firstface);
        out->numfaces = LittleLong(in->numfaces);
    }
}


/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0() {
    Hull_p hull = &_loadModel->hulls[0];

    mNode_p in = _loadModel->nodes;
    int count = _loadModel->numnodes;
    dClipNode_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = _loadModel->planes;

    for (int i = 0; i < count; i++, out++, in++) {
        out->planenum = in->plane - _loadModel->planes;
        for (int j = 0; j < 2; j++) {
            mNode_p child = in->children[j];
            if (child->contents < 0)    out->children[j] = child->contents;
            else                        out->children[j] = child - _loadModel->nodes;
        }
    }
}


/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs) {
    vec3_t corner;
    for (int i = 0; i < VECT_DIM; i++) {
        corner[i] =
            (fabs(mins[i]) > fabs(maxs[i])) ?
            fabs(mins[i]) : fabs(maxs[i]);
    }

    return Length(corner);
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel(Model_p mod, TypeLess_ptr buffer) {
    _loadModel->type = mod_brush;
    dHeader_p header = (dHeader_p)buffer;

    int ver = LittleLong(header->version);
    if (ver != BSPVERSION)
        Host_SysError(
            "Mod_LoadBrushModel: %s has wrong version number "
            "(%i should be %i)",
            mod->name, ver, BSPVERSION
        );


    // swap all the lumps
    mod_base = (uint8_p)header;

    for (int i = 0; i < (sizeof(dHeader_t) / 4); i++) {
        ((int*)header)[i] = LittleLong(((int*)header)[i]);
    }

    // load into heap

    Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
    Mod_LoadEdges(&header->lumps[LUMP_EDGES]);
    Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
    Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
    Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
    Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
    Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
    Mod_LoadFaces(&header->lumps[LUMP_FACES]);
    Mod_LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES]);
    Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
    Mod_LoadLeafs(&header->lumps[LUMP_LEAFS]);
    Mod_LoadNodes(&header->lumps[LUMP_NODES]);
    Mod_LoadClipnodes(&header->lumps[LUMP_CLIPNODES]);
    Mod_LoadEntities(&header->lumps[LUMP_ENTITIES]);
    Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);

    Mod_MakeHull0();

    mod->numframes = 2;  // regular and alternate animation
    mod->flags = 0;

    //
    // set up the SubModels (FIXME: this is confusing)
    //
    for (int i = 0; i < mod->numSubModels; i++) {
        dModel_p bm = &mod->SubModels[i];

        mod->hulls[0].firstclipnode = bm->headnode[0];
        for (int j = 1; j < MAX_MAP_HULLS; j++) {
            mod->hulls[j].firstclipnode = bm->headnode[j];
            mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
        }

        mod->firstModelSurface = bm->firstface;
        mod->numModelSurfaces = bm->numfaces;

        VectorCopy(bm->maxs, mod->maxs);
        VectorCopy(bm->mins, mod->mins);
        mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

        mod->numleafs = bm->visleafs;

        if (i < (mod->numSubModels - 1)) { // duplicate the basic information
            char name[10];

            snprintf(name, sizeof(name), "*%i", i + 1);
            _loadModel = Mod_FindName(name);
            *_loadModel = *mod;
            strcpy(_loadModel->name, name);
            mod = _loadModel;
        }
    }
}
