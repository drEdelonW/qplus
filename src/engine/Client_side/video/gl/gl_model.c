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
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.
// #include "gl_model.h"
#include "model.h"
#include "qOpenGL.h"
#include "types.h"
#include "bspfile.h"
#include "cvar.h"
#include <string.h>
#include "host.h"
#include "common.h"
#include "endian_tools.h"
#include "spritegn.h"
#include "q_tools.h"
#include "mathlib.h"
#include "Surface.h"
#include "Sprite.h"
#include "console.h"


Model_p loadmodel;
static char _loadName[32]; // for hunk tags

void Mod_LoadSpriteModel(Model_p mod, TypeLess_ptr buffer);
void Mod_LoadBrushModel(Model_p mod, TypeLess_ptr buffer);
void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer);
Model_p Mod_LoadModel(Model_p mod, bool crash);

static uint8_t _modNoVis[MAX_MAP_LEAFS / 8];

#define MAX_MOD_KNOWN 512
static uint16_t  _mod_NumKnown = 0;
static Model_t _mod_Known[MAX_MOD_KNOWN];

cvar_t gl_subdivide_size = { "gl_subdivide_size", "128", true };    // GL diff

/*
===============
Mod_Init
===============
*/
void Mod_Init(void) {
    Cvar_RegisterVariable(&gl_subdivide_size);      // GL diff
    memset(_modNoVis, 0xff, sizeof(_modNoVis));
}

/*
===============
Mod_Init

Caches the data if needed
===============
*/
TypeLess_ptr Mod_Extradata(Model_p mod) { // same
    TypeLess_ptr r = Cache_Check(&mod->cache);
    if (r)  return r;

    Mod_LoadModel(mod, true);

    if (!mod->cache.data)   Host_SysError("Mod_Extradata: caching failed");

    return mod->cache.data;
}

/*
===============
Mod_PointInLeaf
===============
*/
mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model) { // same
    if (!model || !model->nodes)    Host_SysError("Mod_PointInLeaf: bad model");

    mNode_p node = model->nodes;
    while (1) {
        if (node->contents < 0) return (mLeaf_p)node;

        mPlane_p plane = node->plane;
        float d = DotProduct(p, plane->normal) - plane->dist;
        if (d > 0)  node = node->children[0];
        else        node = node->children[1];
    }

    return NULL; // never reached
}


/*
===================
Mod_DecompressVis
===================
*/
uint8_p Mod_DecompressVis(uint8_p in, Model_p model) {
    static uint8_t _decompressed[MAX_MAP_LEAFS / 8];

    int row = (model->numleafs + 7) >> 3;
    uint8_p out = _decompressed;

#if 0
    memcpy(out, in, row);
#else
    if (!in) { // no vis info, so make all visible
        while (row) {
            *out++ = 0xff;
            row--;
        }
        return _decompressed;
    }

    do {
        if (*in) {
            *out++ = *in++;
            continue;
        }

        int c = in[1];
        in += 2;
        while (c) {
            *out++ = 0;
            c--;
        }
    } while (out - _decompressed < row);
#endif

    return _decompressed;
}

uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model) {
    if (leaf == model->leafs)
        return _modNoVis;
    return Mod_DecompressVis(leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll(void) {
    int  i;
    Model_p mod;

    for (i = 0, mod = _mod_Known; i < _mod_NumKnown; i++, mod++)
        if (mod->type != mod_alias)
            mod->needload = true;
}

/*
==================
Mod_FindName

==================
*/
Model_p Mod_FindName(cString name) {


    if (!name[0])
        Host_SysError("Mod_ForName: NULL name");

    //
    // search the currently loaded models
    //
    int  i = 0;
    Model_p mod = _mod_Known;
    for (; i < _mod_NumKnown; i++, mod++)
        if (!strcmp(mod->name, name))
            break;

    if (i == _mod_NumKnown) {
        if (_mod_NumKnown == MAX_MOD_KNOWN)
            Host_SysError("_mod_NumKnown == MAX_MOD_KNOWN");
        strcpy(mod->name, name);
        mod->needload = true;
        _mod_NumKnown++;
    }

    return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel(cString name) {
    Model_p mod;

    mod = Mod_FindName(name);

    if (!mod->needload) {
        if (mod->type == mod_alias)
            Cache_Check(&mod->cache);
    }
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
Model_p Mod_LoadModel(Model_p mod, bool crash) {
    TypeLess_ptr d;
    uint32_p buf;
    uint8_t stackbuf[1024];  // avoid dirtying the cache heap

    if (!mod->needload) {
        if (mod->type == mod_alias) {
            d = Cache_Check(&mod->cache);
            if (d)
                return mod;
        }
        else
            return mod;  // not cached at all
    }

    //
    // because the world is so huge, load it one piece at a time
    //
    if (!crash) {

    }

    //
    // load the file
    //
    buf = (uint32_p)COM_LoadStackFile(mod->name, stackbuf, sizeof(stackbuf));
    if (!buf) {
        if (crash)
            Host_SysError("Mod_NumForName: %s not found", mod->name);
        return NULL;
    }

    //
    // allocate a new model
    //
    COM_FileBase(mod->name, _loadName);

    loadmodel = mod;

    //
    // fill it in
    //

    // call the apropriate loader
    mod->needload = false;

    switch (LittleLong(*(uint32_p)buf)) {
    case IDPOLYHEADER:      Mod_LoadAliasModel(mod, buf);   break;
    case IDSPRITEHEADER:    Mod_LoadSpriteModel(mod, buf);  break;
    default:                Mod_LoadBrushModel(mod, buf);   break;
    }

    return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
Model_p Mod_ForName(cString name, bool crash) {
    Model_p mod;

    mod = Mod_FindName(name);

    return Mod_LoadModel(mod, crash);
}


/*
===============================================================================

                    BRUSHMODEL LOADING

===============================================================================
*/

uint8_p mod_base;


/*
=================
Mod_LoadTextures
=================
*/
void Mod_LoadTextures(Lump_p l) {
    int  i, j, pixels, num, max, altmax;
    MipTex_p mt;
    Texture_p tx, tx2;
    Texture_p anims[10];
    Texture_p altanims[10];
    dMipTexLump_p m;

    if (!l->fileLen) {
        loadmodel->textures = NULL;
        return;
    }
    m = (dMipTexLump_p)(mod_base + l->fileOfs);

    m->nummiptex = LittleLong(m->nummiptex);

    loadmodel->numtextures = m->nummiptex;
    loadmodel->textures = Hunk_AllocName(m->nummiptex * sizeof(*loadmodel->textures), _loadName);

    for (i = 0; i < m->nummiptex; i++) {
        m->dataofs[i] = LittleLong(m->dataofs[i]);
        if (m->dataofs[i] == -1)
            continue;
        mt = (MipTex_p)((uint8_p)m + m->dataofs[i]);
        mt->width = LittleLong(mt->width);
        mt->height = LittleLong(mt->height);
        for (j = 0; j < MIPLEVELS; j++)
            mt->offsets[j] = LittleLong(mt->offsets[j]);

        if ((mt->width & 15) || (mt->height & 15))
            Host_SysError("Texture %s is not 16 aligned", mt->name);
        pixels = mt->width * mt->height / 64 * 85;
        tx = Hunk_AllocName(sizeof(Texture_t) + pixels, _loadName);
        loadmodel->textures[i] = tx;

        memcpy(tx->name, mt->name, sizeof(tx->name));
        tx->width = mt->width;
        tx->height = mt->height;
        for (j = 0; j < MIPLEVELS; j++)
            tx->offsets[j] = mt->offsets[j] + sizeof(Texture_t) - sizeof(MipTex_t);
        // the pixels immediately follow the structures
        memcpy(tx + 1, mt + 1, pixels);


        if (!Q_strncmp(mt->name, "sky", 3))
            R_InitSky(tx);
        else {
            texture_mode = GL_LINEAR_MIPMAP_NEAREST; //_LINEAR;
            tx->gl_texturenum = GL_LoadTexture(mt->name, tx->width, tx->height, (uint8_p)(tx + 1), true, false);
            texture_mode = GL_LINEAR;
        }
    }

    //
    // sequence the animations
    //
    for (i = 0; i < m->nummiptex; i++) {
        tx = loadmodel->textures[i];
        if (!tx || tx->name[0] != '+')
            continue;
        if (tx->anim_next)
            continue; // allready sequenced

        // find the number of frames in the animation
        memset(anims, 0, sizeof(anims));
        memset(altanims, 0, sizeof(altanims));

        max = tx->name[1];
        altmax = 0;
        if (max >= 'a' && max <= 'z')
            max -= 'a' - 'A';
        if (max >= '0' && max <= '9') {
            max -= '0';
            altmax = 0;
            anims[max] = tx;
            max++;
        }
        else if (max >= 'A' && max <= 'J') {
            altmax = max - 'A';
            max = 0;
            altanims[altmax] = tx;
            altmax++;
        }
        else
            Host_SysError("Bad animating texture %s", tx->name);

        for (j = i + 1; j < m->nummiptex; j++) {
            tx2 = loadmodel->textures[j];
            if (!tx2 || tx2->name[0] != '+')
                continue;
            if (strcmp(tx2->name + 2, tx->name + 2))
                continue;

            num = tx2->name[1];
            if (num >= 'a' && num <= 'z')
                num -= 'a' - 'A';
            if (num >= '0' && num <= '9') {
                num -= '0';
                anims[num] = tx2;
                if (num + 1 > max)
                    max = num + 1;
            }
            else if (num >= 'A' && num <= 'J') {
                num = num - 'A';
                altanims[num] = tx2;
                if (num + 1 > altmax)
                    altmax = num + 1;
            }
            else
                Host_SysError("Bad animating texture %s", tx->name);
        }

#define ANIM_CYCLE 2
        // link them all together
        for (j = 0; j < max; j++) {
            tx2 = anims[j];
            if (!tx2)
                Host_SysError("Missing frame %i of %s", j, tx->name);
            tx2->anim_total = max * ANIM_CYCLE;
            tx2->anim_min = j * ANIM_CYCLE;
            tx2->anim_max = (j + 1) * ANIM_CYCLE;
            tx2->anim_next = anims[(j + 1) % max];
            if (altmax)
                tx2->alternate_anims = altanims[0];
        }
        for (j = 0; j < altmax; j++) {
            tx2 = altanims[j];
            if (!tx2)
                Host_SysError("Missing frame %i of %s", j, tx->name);
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
    if (!l->fileLen) {
        loadmodel->lightdata = NULL;
        return;
    }
    loadmodel->lightdata = Hunk_AllocName(l->fileLen, _loadName);
    memcpy(loadmodel->lightdata, mod_base + l->fileOfs, l->fileLen);
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility(Lump_p l) {
    if (!l->fileLen) {
        loadmodel->visdata = NULL;
        return;
    }
    loadmodel->visdata = Hunk_AllocName(l->fileLen, _loadName);
    memcpy(loadmodel->visdata, mod_base + l->fileOfs, l->fileLen);
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities(Lump_p l) {
    if (!l->fileLen) {
        loadmodel->entities = NULL;
        return;
    }
    loadmodel->entities = Hunk_AllocName(l->fileLen, _loadName);
    memcpy(loadmodel->entities, mod_base + l->fileOfs, l->fileLen);
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes(Lump_p l) {
    dVertex_p in;
    mVertex_p out;
    int   i, count;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->vertexes = out;
    loadmodel->numvertexes = count;

    for (i = 0; i < count; i++, in++, out++) {
        out->position[0] = LittleFloat(in->point[0]);
        out->position[1] = LittleFloat(in->point[1]);
        out->position[2] = LittleFloat(in->point[2]);
    }
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels(Lump_p l) {
    dModel_p in;
    dModel_p out;
    int   i, j, count;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->SubModels = out;
    loadmodel->numSubModels = count;

    for (i = 0; i < count; i++, in++, out++) {
        for (j = 0; j < 3; j++) { // spread the mins / maxs by a pixel
            out->mins[j] = LittleFloat(in->mins[j]) - 1;
            out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
            out->origin[j] = LittleFloat(in->origin[j]);
        }
        for (j = 0; j < MAX_MAP_HULLS; j++)
            out->headnode[j] = LittleLong(in->headnode[j]);
        out->visleafs = LittleLong(in->visleafs);
        out->firstface = LittleLong(in->firstface);
        out->numfaces = LittleLong(in->numfaces);
    }
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges(Lump_p l) {
    dEdge_p in;
    mEdge_p out;
    int  i, count;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName((count + 1) * sizeof(*out), _loadName);

    loadmodel->edges = out;
    loadmodel->numedges = count;

    for (i = 0; i < count; i++, in++, out++) {
        out->v[0] = (uint16_t)LittleShort(in->v[0]);
        out->v[1] = (uint16_t)LittleShort(in->v[1]);
    }
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo(Lump_p l) {
    TexInfo_p in;
    mTexInfo_p out;
    int  i, j, count;
    int  miptex;
    float len1, len2;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->texinfo = out;
    loadmodel->numtexinfo = count;

    for (i = 0; i < count; i++, in++, out++) {
        for (j = 0; j < 8; j++)
            out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
        len1 = Length(out->vecs[0]);
        len2 = Length(out->vecs[1]);
        len1 = (len1 + len2) / 2;
        if (len1 < 0.32)        out->mipadjust = 4;
        else if (len1 < 0.49)   out->mipadjust = 3;
        else if (len1 < 0.99)   out->mipadjust = 2;
        else                    out->mipadjust = 1;
#if 0
        if (len1 + len2 < 0.001)
            out->mipadjust = 1;  // don't crash
        else
            out->mipadjust = 1 / floor((len1 + len2) / 2 + 0.1);
#endif

        miptex = LittleLong(in->miptex);
        out->flags = LittleLong(in->flags);

        if (!loadmodel->textures) {
            out->texture = r_notexture_mip; // checkerboard texture
            out->flags = 0;
        }
        else {
            if (miptex >= loadmodel->numtextures)
                Host_SysError("miptex >= loadmodel->numtextures");
            out->texture = loadmodel->textures[miptex];
            if (!out->texture) {
                out->texture = r_notexture_mip; // texture not found
                out->flags = 0;
            }
        }
    }
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents(mSurface_p s) {
    float mins[2], maxs[2], val;
    int  i, j, e;
    mVertex_p v;
    mTexInfo_p tex;
    int  bmins[2], bmaxs[2];

    mins[0] = mins[1] = 999999;
    maxs[0] = maxs[1] = -99999;

    tex = s->texinfo;

    for (i = 0; i < s->numedges; i++) {
        e = loadmodel->surfedges[s->firstedge + i];
        if (e >= 0)
            v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
        else
            v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

        for (j = 0; j < 2; j++) {
            val = v->position[0] * tex->vecs[j][0] +
                v->position[1] * tex->vecs[j][1] +
                v->position[2] * tex->vecs[j][2] +
                tex->vecs[j][3];
            if (val < mins[j])
                mins[j] = val;
            if (val > maxs[j])
                maxs[j] = val;
        }
    }

    for (i = 0; i < 2; i++) {
        bmins[i] = floor(mins[i] / 16);
        bmaxs[i] = ceil(maxs[i] / 16);

        s->texturemins[i] = bmins[i] * 16;
        s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
        if (!(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256 */)
            Host_SysError("Bad surface extents");
    }
}


/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces(Lump_p l) {
    dFace_p in;
    mSurface_p out;
    int   i, count, surfnum;
    int   planenum, side;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->surfaces = out;
    loadmodel->numsurfaces = count;

    for (surfnum = 0; surfnum < count; surfnum++, in++, out++) {
        out->firstedge = LittleLong(in->firstedge);
        out->numedges = LittleShort(in->numedges);
        out->flags = 0;

        planenum = LittleShort(in->planenum);
        side = LittleShort(in->side);
        if (side)
            out->flags |= SURF_PLANEBACK;

        out->plane = loadmodel->planes + planenum;

        out->texinfo = loadmodel->texinfo + LittleShort(in->texinfo);

        CalcSurfaceExtents(out);

        // lighting info

        for (i = 0; i < MAXLIGHTMAPS; i++)
            out->styles[i] = in->styles[i];
        i = LittleLong(in->lightofs);
        if (i == -1)
            out->samples = NULL;
        else
            out->samples = loadmodel->lightdata + i;

        // set the drawing flags flag

        if (!Q_strncmp(out->texinfo->texture->name, "sky", 3)) // sky
        {
            out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
#ifndef QUAKE2
            GL_SubdivideSurface(out); // cut up polygon for warps
#endif
            continue;
        }

        if (!Q_strncmp(out->texinfo->texture->name, "*", 1))  // turbulent
        {
            out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
            for (i = 0; i < 2; i++) {
                out->extents[i] = 16384;
                out->texturemins[i] = -8192;
            }
            GL_SubdivideSurface(out); // cut up polygon for warps
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
    if (node->contents < 0)
        return;
    Mod_SetParent(node->children[0], node);
    Mod_SetParent(node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes(Lump_p l) {
    int   i, j, count, p;
    dNode_p in;
    mNode_p out;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->nodes = out;
    loadmodel->numnodes = count;

    for (i = 0; i < count; i++, in++, out++) {
        for (j = 0; j < 3; j++) {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        p = LittleLong(in->planenum);
        out->plane = loadmodel->planes + p;

        out->firstsurface = LittleShort(in->firstface);
        out->numsurfaces = LittleShort(in->numfaces);

        for (j = 0; j < 2; j++) {
            p = LittleShort(in->children[j]);
            if (p >= 0)
                out->children[j] = loadmodel->nodes + p;
            else
                out->children[j] = (mNode_p)(loadmodel->leafs + (-1 - p));
        }
    }

    Mod_SetParent(loadmodel->nodes, NULL); // sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs(Lump_p l) {
    dLeaf_p in;
    mLeaf_p out;
    int   i, j, count, p;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->leafs = out;
    loadmodel->numleafs = count;

    for (i = 0; i < count; i++, in++, out++) {
        for (j = 0; j < 3; j++) {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        p = LittleLong(in->contents);
        out->contents = p;

        out->firstmarksurface = loadmodel->marksurfaces +
            LittleShort(in->firstmarksurface);
        out->nummarksurfaces = LittleShort(in->nummarksurfaces);

        p = LittleLong(in->visofs);
        if (p == -1)
            out->compressed_vis = NULL;
        else
            out->compressed_vis = loadmodel->visdata + p;
        out->efrags = NULL;

        for (j = 0; j < 4; j++)
            out->ambient_sound_level[j] = in->ambient_level[j];

        // gl underwater warp
        if (out->contents != CONTENTS_EMPTY) {
            for (j = 0; j < out->nummarksurfaces; j++)
                out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
        }
    }
}

/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes(Lump_p l) {
    dClipNode_p in, out;
    int   i, count;
    Hull_p hull;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->clipnodes = out;
    loadmodel->numclipnodes = count;

    hull = &loadmodel->hulls[1];
    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = loadmodel->planes;
    hull->clip_mins[0] = -16;
    hull->clip_mins[1] = -16;
    hull->clip_mins[2] = -24;
    hull->clip_maxs[0] = 16;
    hull->clip_maxs[1] = 16;
    hull->clip_maxs[2] = 32;

    hull = &loadmodel->hulls[2];
    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = loadmodel->planes;
    hull->clip_mins[0] = -32;
    hull->clip_mins[1] = -32;
    hull->clip_mins[2] = -24;
    hull->clip_maxs[0] = 32;
    hull->clip_maxs[1] = 32;
    hull->clip_maxs[2] = 64;

    for (i = 0; i < count; i++, out++, in++) {
        out->planenum = LittleLong(in->planenum);
        out->children[0] = LittleShort(in->children[0]);
        out->children[1] = LittleShort(in->children[1]);
    }
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0(void) {
    mNode_p in, child;
    dClipNode_p out;
    int   i, j, count;
    Hull_p hull;

    hull = &loadmodel->hulls[0];

    in = loadmodel->nodes;
    count = loadmodel->numnodes;
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = loadmodel->planes;

    for (i = 0; i < count; i++, out++, in++) {
        out->planenum = in->plane - loadmodel->planes;
        for (j = 0; j < 2; j++) {
            child = in->children[j];
            if (child->contents < 0)
                out->children[j] = child->contents;
            else
                out->children[j] = child - loadmodel->nodes;
        }
    }
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces(Lump_p l) {
    int  i, j, count;
    int16_p in;
    mSurface_p* out;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->marksurfaces = out;
    loadmodel->nummarksurfaces = count;

    for (i = 0; i < count; i++) {
        j = LittleShort(in[i]);
        if (j >= loadmodel->numsurfaces)
            Host_SysError("Mod_ParseMarksurfaces: bad surface number");
        out[i] = loadmodel->surfaces + j;
    }
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges(Lump_p l) {
    int  i, count;
    int* in, * out;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), _loadName);

    loadmodel->surfedges = out;
    loadmodel->numsurfedges = count;

    for (i = 0; i < count; i++)
        out[i] = LittleLong(in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes(Lump_p l) {
    int   i, j;
    mPlane_p out;
    dPlane_p in;
    int   count;
    int   bits;

    in = (TypeLess_ptr)(mod_base + l->fileOfs);
    if (l->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->fileLen / sizeof(*in);
    out = Hunk_AllocName(count * 2 * sizeof(*out), _loadName);

    loadmodel->planes = out;
    loadmodel->numplanes = count;

    for (i = 0; i < count; i++, in++, out++) {
        bits = 0;
        for (j = 0; j < 3; j++) {
            out->normal[j] = LittleFloat(in->normal[j]);
            if (out->normal[j] < 0)
                bits |= 1 << j;
        }

        out->dist = LittleFloat(in->dist);
        out->type = LittleLong(in->type);
        out->signbits = bits;
    }
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs) {
    vec3_t corner;
    for (int i = 0; i < 3; i++) {
        corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
    }

    return Length(corner);
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel(Model_p mod, TypeLess_ptr buffer) {
    int   i, j;
    dHeader_p header;
    dModel_p bm;

    loadmodel->type = mod_brush;

    header = (dHeader_p)buffer;

    i = LittleLong(header->version);
    if (i != BSPVERSION)
        Host_SysError("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);

    // swap all the lumps
    mod_base = (uint8_p)header;

    for (i = 0; i < sizeof(dHeader_t) / 4; i++)
        ((int*)header)[i] = LittleLong(((int*)header)[i]);

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

    //
    // set up the SubModels (FIXME: this is confusing)
    //
    for (i = 0; i < mod->numSubModels; i++) {
        bm = &mod->SubModels[i];

        mod->hulls[0].firstclipnode = bm->headnode[0];
        for (j = 1; j < MAX_MAP_HULLS; j++) {
            mod->hulls[j].firstclipnode = bm->headnode[j];
            mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
        }

        mod->firstModelSurface = bm->firstface;
        mod->numModelSurfaces = bm->numfaces;

        VectorCopy(bm->maxs, mod->maxs);
        VectorCopy(bm->mins, mod->mins);

        mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

        mod->numleafs = bm->visleafs;

        if (i < mod->numSubModels - 1) { // duplicate the basic information
            char name[10];

            snprintf(name, sizeof(name), "*%i", i + 1);
            loadmodel = Mod_FindName(name);
            *loadmodel = *mod;
            strcpy(loadmodel->name, name);
            mod = loadmodel;
        }
    }
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

AliasHdr_p pheader;

stvert_t stverts[MAXALIASVERTS];
mTriangle_t triangles[MAXALIASTRIS];

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
TriVertx_p poseverts[MAXALIASFRAMES];
int   posenum;

uint8_p* player_8bit_texels_tbl;
uint8_p player_8bit_texels;

/*
=================
Mod_LoadAliasFrame
=================
*/
TypeLess_ptr Mod_LoadAliasFrame(TypeLess_ptr pin, mAliasFrameDesc_p frame) {
    TriVertx_p pinframe;
    // int    i, j;
    dAliasFrame_p pdaliasframe;

    pdaliasframe = (dAliasFrame_p)pin;

    strcpy(frame->name, pdaliasframe->name);
    frame->firstpose = posenum;
    frame->numposes = 1;

    for (int i = 0; i < 3; i++) {
        // these are byte values, so we don't have to worry about
        // endianness
        frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
        frame->bboxmin.v[i] = pdaliasframe->bboxmax.v[i];
    }

    pinframe = (TriVertx_p)(pdaliasframe + 1);

    poseverts[posenum] = pinframe;
    posenum++;

    pinframe += pheader->numverts;

    return (TypeLess_ptr)pinframe;
}


/*
=================
Mod_LoadAliasGroup
=================
*/
TypeLess_ptr Mod_LoadAliasGroup(TypeLess_ptr pin, mAliasFrameDesc_p frame) {
    dAliasGroup_p pingroup;
    int     i, numframes;
    dAliasInterval_p pin_intervals;
    TypeLess_ptr ptemp;

    pingroup = (dAliasGroup_p)pin;

    numframes = LittleLong(pingroup->numframes);

    frame->firstpose = posenum;
    frame->numposes = numframes;

    for (i = 0; i < 3; i++) {
        // these are byte values, so we don't have to worry about endianness
        frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
        frame->bboxmin.v[i] = pingroup->bboxmax.v[i];
    }

    pin_intervals = (dAliasInterval_p)(pingroup + 1);

    frame->interval = LittleFloat(pin_intervals->interval);

    pin_intervals += numframes;

    ptemp = (TypeLess_ptr)pin_intervals;

    for (i = 0; i < numframes; i++) {
        poseverts[posenum] = (TriVertx_p)((dAliasFrame_p)ptemp + 1);
        posenum++;

        ptemp = (TriVertx_p)((dAliasFrame_p)ptemp + 1) + pheader->numverts;
    }

    return ptemp;
}

//=========================================================

/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes - Ed
=================
*/

typedef struct {
    int16_t  x, y;
} floodfill_t;


// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
    if (pos[off] == fillcolor) \
    { \
        pos[off] = 255; \
        fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
        inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
    } \
    else if (pos[off] != 255) fdc = pos[off]; \
}

void Mod_FloodFillSkin(uint8_p skin, int skinwidth, int skinheight) {
    byte    fillcolor = *skin; // assume this is the pixel to fill
    floodfill_t   fifo[FLOODFILL_FIFO_SIZE];
    int     inpt = 0, outpt = 0;
    int     filledcolor = -1;
    int     i;

    if (filledcolor == -1) {
        filledcolor = 0;
        // attempt to find opaque black
        for (i = 0; i < 256; ++i)
            if (d_8to24table[i] == (255 << 0)) // alpha 1.0
            {
                filledcolor = i;
                break;
            }
    }

    // can't fill to filled color or to transparent color (used as visited marker)
    if ((fillcolor == filledcolor) || (fillcolor == 255)) {
        //printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
        return;
    }

    fifo[inpt].x = 0, fifo[inpt].y = 0;
    inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

    while (outpt != inpt) {
        int   x = fifo[outpt].x, y = fifo[outpt].y;
        int   fdc = filledcolor;
        uint8_p pos = &skin[x + skinwidth * y];

        outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

        if (x > 0)    FLOODFILL_STEP(-1, -1, 0);
        if (x < skinwidth - 1) FLOODFILL_STEP(1, 1, 0);
        if (y > 0)    FLOODFILL_STEP(-skinwidth, 0, -1);
        if (y < skinheight - 1) FLOODFILL_STEP(skinwidth, 0, 1);
        skin[x + skinwidth * y] = fdc;
    }
}

/*
===============
Mod_LoadAllSkins
===============
*/
TypeLess_ptr Mod_LoadAllSkins(int numskins, dAliasSkinType_p pskintype) {
    int  i, j, k;
    char name[32];
    int  s;
    // uint8_p copy;
    uint8_p skin;
    uint8_p texels;
    dAliasSkinGroup_p pinskingroup;
    int  groupskins;
    dAliasSkinInterval_p pinskinintervals;

    skin = (uint8_p)(pskintype + 1);

    if (numskins < 1 || numskins > MAX_SKINS)
        Host_SysError("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

    s = pheader->skinwidth * pheader->skinheight;

    for (i = 0; i < numskins; i++) {
        if (pskintype->type == ALIAS_SKIN_SINGLE) {
            Mod_FloodFillSkin(skin, pheader->skinwidth, pheader->skinheight);

            // save 8 bit texels for the player model to remap
    //  if (!strcmp(loadmodel->name,"progs/player.mdl")) {
            texels = Hunk_AllocName(s, _loadName);
            pheader->texels[i] = texels - (uint8_p)pheader;
            memcpy(texels, (uint8_p)(pskintype + 1), s);
            //  }
            snprintf(name, sizeof(name), "%s_%i", loadmodel->name, i);
            pheader->gl_texturenum[i][0] =
                pheader->gl_texturenum[i][1] =
                pheader->gl_texturenum[i][2] =
                pheader->gl_texturenum[i][3] =
                GL_LoadTexture(name, pheader->skinwidth,
                    pheader->skinheight, (uint8_p)(pskintype + 1), true, false);
            pskintype = (dAliasSkinType_p)((uint8_p)(pskintype + 1) + s);
        }
        else {
            // animating skin group.  yuck.
            pskintype++;
            pinskingroup = (dAliasSkinGroup_p)pskintype;
            groupskins = LittleLong(pinskingroup->numskins);
            pinskinintervals = (dAliasSkinInterval_p)(pinskingroup + 1);

            pskintype = (TypeLess_ptr)(pinskinintervals + groupskins);

            for (j = 0; j < groupskins; j++) {
                Mod_FloodFillSkin(skin, pheader->skinwidth, pheader->skinheight);
                if (j == 0) {
                    texels = Hunk_AllocName(s, _loadName);
                    pheader->texels[i] = texels - (uint8_p)pheader;
                    memcpy(texels, (uint8_p)(pskintype), s);
                }
                snprintf(name, sizeof(name), "%s_%i_%i", loadmodel->name, i, j);
                pheader->gl_texturenum[i][j & 3] =
                    GL_LoadTexture(name, pheader->skinwidth,
                        pheader->skinheight, (uint8_p)(pskintype), true, false);
                pskintype = (dAliasSkinType_p)((uint8_p)(pskintype)+s);
            }
            k = j;
            for (/* */; j < 4; j++)
                pheader->gl_texturenum[i][j & 3] =
                pheader->gl_texturenum[i][j - k];
        }
    }

    return (TypeLess_ptr)pskintype;
}

//=========================================================================

/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer) {
    int     i, j;
    Mdl_p   pinmodel;
    stvert_p pinstverts;
    dTriangle_p pintriangles;
    int     version, numframes; //, numskins;
    int     size;
    dAliasFrameType_p pframetype;
    dAliasSkinType_p pskintype;
    int     start, end, total;

    start = Hunk_LowMark();

    pinmodel = (Mdl_p)buffer;

    version = LittleLong(pinmodel->version);
    if (version != ALIAS_VERSION)
        Host_SysError("%s has wrong version number (%i should be %i)",
            mod->name, version, ALIAS_VERSION);

    //
    // allocate space for a working header, plus all the data except the frames,
    // skin and group info
    //
    size = sizeof(AliasHdr_t)
        + (LittleLong(pinmodel->numframes) - 1) *
        sizeof(pheader->frames[0]);
    pheader = Hunk_AllocName(size, _loadName);

    mod->flags = LittleLong(pinmodel->flags);

    //
    // endian-adjust and copy the data, starting with the alias model header
    //
    pheader->boundingradius = LittleFloat(pinmodel->boundingradius);
    pheader->numskins = LittleLong(pinmodel->numskins);
    pheader->skinwidth = LittleLong(pinmodel->skinwidth);
    pheader->skinheight = LittleLong(pinmodel->skinheight);

    if (pheader->skinheight > MAX_LBM_HEIGHT)
        Host_SysError("model %s has a skin taller than %d", mod->name,
            MAX_LBM_HEIGHT);

    pheader->numverts = LittleLong(pinmodel->numverts);

    if (pheader->numverts <= 0)
        Host_SysError("model %s has no vertices", mod->name);

    if (pheader->numverts > MAXALIASVERTS)
        Host_SysError("model %s has too many vertices", mod->name);

    pheader->numtris = LittleLong(pinmodel->numtris);

    if (pheader->numtris <= 0)
        Host_SysError("model %s has no triangles", mod->name);

    pheader->numframes = LittleLong(pinmodel->numframes);
    numframes = pheader->numframes;
    if (numframes < 1)
        Host_SysError("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

    pheader->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
    mod->synctype = LittleLong(pinmodel->synctype);
    mod->numframes = pheader->numframes;

    for (i = 0; i < 3; i++) {
        pheader->scale[i] = LittleFloat(pinmodel->scale[i]);
        pheader->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
        pheader->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
    }


    //
    // load the skins
    //
    pskintype = (dAliasSkinType_p)&pinmodel[1];
    pskintype = Mod_LoadAllSkins(pheader->numskins, pskintype);

    //
    // load base s and t vertices
    //
    pinstverts = (stvert_p)pskintype;

    for (i = 0; i < pheader->numverts; i++) {
        stverts[i].onseam = LittleLong(pinstverts[i].onseam);
        stverts[i].s = LittleLong(pinstverts[i].s);
        stverts[i].t = LittleLong(pinstverts[i].t);
    }

    //
    // load triangle lists
    //
    pintriangles = (dTriangle_p)&pinstverts[pheader->numverts];

    for (i = 0; i < pheader->numtris; i++) {
        triangles[i].facesfront = LittleLong(pintriangles[i].facesfront);

        for (j = 0; j < 3; j++) {
            triangles[i].vertindex[j] =
                LittleLong(pintriangles[i].vertindex[j]);
        }
    }

    //
    // load the frames
    //
    posenum = 0;
    pframetype = (dAliasFrameType_p)&pintriangles[pheader->numtris];

    for (i = 0; i < numframes; i++) {
        AliasFrameType_t frametype;

        frametype = LittleLong(pframetype->type);

        if (frametype == ALIAS_SINGLE) {
            pframetype = (dAliasFrameType_p)
                Mod_LoadAliasFrame(pframetype + 1, &pheader->frames[i]);
        }
        else {
            pframetype = (dAliasFrameType_p)
                Mod_LoadAliasGroup(pframetype + 1, &pheader->frames[i]);
        }
    }

    pheader->numposes = posenum;

    mod->type = mod_alias;

    // FIXME: do this right
    mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
    mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;

    //
    // build the draw lists
    //
    GL_MakeAliasModelDisplayLists(mod, pheader);

    //
    // move the complete, relocatable alias model to the cache
    // 
    end = Hunk_LowMark();
    total = end - start;

    Cache_Alloc(&mod->cache, total, _loadName);
    if (!mod->cache.data)
        return;
    memcpy(mod->cache.data, pheader, total);

    Hunk_FreeToLowMark(start);
}

//=============================================================================

/*
=================
Mod_LoadSpriteFrame
=================
*/
TypeLess_ptr Mod_LoadSpriteFrame(TypeLess_ptr pin, mSpriteFrame_p* ppframe, int framenum) {
    dSpriteFrame_p pinframe;
    mSpriteFrame_p pspriteframe;
    int     width, height, size, origin[2];
    // uint16_p ppixout;
    // uint8_p ppixin;
    char    name[NAME_LENGTH];

    pinframe = (dSpriteFrame_p)pin;

    width = LittleLong(pinframe->width);
    height = LittleLong(pinframe->height);
    size = width * height;

    pspriteframe = Hunk_AllocName(sizeof(mSpriteFrame_t), _loadName);

    Q_memset(pspriteframe, 0, sizeof(mSpriteFrame_t));

    *ppframe = pspriteframe;

    pspriteframe->width = width;
    pspriteframe->height = height;
    origin[0] = LittleLong(pinframe->origin[0]);
    origin[1] = LittleLong(pinframe->origin[1]);

    pspriteframe->up = origin[1];
    pspriteframe->down = origin[1] - height;
    pspriteframe->left = origin[0];
    pspriteframe->right = width + origin[0];

    snprintf(name, sizeof(name), "%s_%i", loadmodel->name, framenum);
    pspriteframe->gl_texturenum = GL_LoadTexture(name, width, height, (uint8_p)(pinframe + 1), true, true);

    return (TypeLess_ptr)((uint8_p)pinframe + sizeof(dSpriteFrame_t) + size);
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
TypeLess_ptr Mod_LoadSpriteGroup(TypeLess_ptr pin, mSpriteFrame_p* ppframe, int framenum) {
    dSpriteGroup_p pingroup;
    mSpriteGroup_p pspritegroup;
    int     i, numframes;
    dSpriteInterval_p pin_intervals;
    float_p poutintervals;
    TypeLess_ptr ptemp;

    pingroup = (dSpriteGroup_p)pin;

    numframes = LittleLong(pingroup->numframes);

    pspritegroup = Hunk_AllocName(sizeof(mSpriteGroup_t) +
        (numframes - 1) * sizeof(pspritegroup->frames[0]), _loadName);

    pspritegroup->numframes = numframes;

    *ppframe = (mSpriteFrame_p)pspritegroup;

    pin_intervals = (dSpriteInterval_p)(pingroup + 1);

    poutintervals = Hunk_AllocName(numframes * sizeof(float), _loadName);

    pspritegroup->intervals = poutintervals;

    for (i = 0; i < numframes; i++) {
        *poutintervals = LittleFloat(pin_intervals->interval);
        if (*poutintervals <= 0.0)
            Host_SysError("Mod_LoadSpriteGroup: interval<=0");

        poutintervals++;
        pin_intervals++;
    }

    ptemp = (TypeLess_ptr)pin_intervals;

    for (i = 0; i < numframes; i++) {
        ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i], framenum * 100 + i);
    }

    return ptemp;
}


/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel(Model_p mod, TypeLess_ptr buffer) {
    int     i;
    int     version;
    dSprite_p pin;
    mSprite_p psprite;
    int     numframes;
    int     size;
    dSpriteFrameType_p pframetype;

    pin = (dSprite_p)buffer;

    version = LittleLong(pin->version);
    if (version != SPRITE_VERSION)
        Host_SysError("%s has wrong version number "
            "(%i should be %i)", mod->name, version, SPRITE_VERSION);

    numframes = LittleLong(pin->numframes);

    size = sizeof(mSprite_t) + (numframes - 1) * sizeof(psprite->frames);

    psprite = Hunk_AllocName(size, _loadName);

    mod->cache.data = psprite;

    psprite->type = LittleLong(pin->type);
    psprite->maxwidth = LittleLong(pin->width);
    psprite->maxheight = LittleLong(pin->height);
    psprite->beamlength = LittleFloat(pin->beamlength);
    mod->synctype = LittleLong(pin->synctype);
    psprite->numframes = numframes;

    mod->mins[0] = mod->mins[1] = -psprite->maxwidth / 2;
    mod->maxs[0] = mod->maxs[1] = psprite->maxwidth / 2;
    mod->mins[2] = -psprite->maxheight / 2;
    mod->maxs[2] = psprite->maxheight / 2;

    //
    // load the frames
    //
    if (numframes < 1)
        Host_SysError("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);

    mod->numframes = numframes;

    pframetype = (dSpriteFrameType_p)(pin + 1);

    for (i = 0; i < numframes; i++) {
        SpriteFrameType_t frametype;

        frametype = LittleLong(pframetype->type);
        psprite->frames[i].type = frametype;

        if (frametype == SPR_SINGLE) {
            pframetype = (dSpriteFrameType_p)
                Mod_LoadSpriteFrame(pframetype + 1,
                    &psprite->frames[i].frameptr, i);
        }
        else {
            pframetype = (dSpriteFrameType_p)
                Mod_LoadSpriteGroup(pframetype + 1,
                    &psprite->frames[i].frameptr, i);
        }
    }

    mod->type = mod_sprite;
}

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print(void) {
    int  i;
    Model_p mod;

    Con_Printf("Cached models:\n");
    for (i = 0, mod = _mod_Known; i < _mod_NumKnown; i++, mod++) {
        Con_Printf("%8p : %s\n", mod->cache.data, mod->name);
    }
}


