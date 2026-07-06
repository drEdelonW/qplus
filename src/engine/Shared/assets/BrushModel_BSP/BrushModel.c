#include "assert.h"
#include "Lump.h"
#include "Texture.h"
#include "Surface.h"

typedef enum {  // BSP Lumps
    LUMP_ENTITIES = 0u, // Mod_LoadEntities
    LUMP_PLANES,        // Mod_LoadPlanes
    LUMP_TEXTURES,      // Mod_LoadTextures
    LUMP_VERTEXES,      // Mod_LoadVertexes
    LUMP_VISIBILITY,    // Mod_LoadVisibility
    LUMP_NODES,         // Mod_LoadNodes
    LUMP_TEXINFO,       // Mod_LoadTexinfo
    LUMP_FACES,         // Mod_LoadFaces
    LUMP_LIGHTING,      // Mod_LoadLighting
    LUMP_CLIPNODES,     // Mod_LoadClipnodes
    LUMP_LEAFS,         // Mod_LoadLeafs
    LUMP_MARKSURFACES,  // Mod_LoadMarksurfaces
    LUMP_EDGES,         // Mod_LoadEdges
    LUMP_SURFEDGES,     // Mod_LoadSurfedges
    LUMP_MODELS,        // Mod_LoadSubmodels

    HEADER_LUMPS        // total count of lumps in BSP header
} LumpType;

typedef struct {
    int32_t version;
    Lump_t  lumps[HEADER_LUMPS];    // LumpType
} dHeader_t;        STATIC_ASSERT_SIZE(dHeader_t, 4 + 15 * 8); // 124
typedef dHeader_t* dHeader_p;


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
#include "vector_tools.h"
#ifdef GLQUAKE
#   include "qOpenGL.h"
#else
#   include "render.h"
#endif
#include <string.h>  // strcpy, memcpy
#include <stdio.h>
#include <math.h>

/*
    =================
    Mod_LoadVertexes
    =================
*/
void Mod_LoadVertexes(Lump_p Lump_in) {
    dVertex_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(dVertex_t))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(dVertex_t);
    mVertex_p out = Hunk_AllocName(sizeof(mVertex_t) * count, Mod_loadName);

    _loadModel->vertexes = out;
    _loadModel->numvertexes = count;

    for (int i = 0; i < count; i++)
        out[i].position = LittleVector(in[i].point);
}


/*
    =================
    Mod_LoadEdges
    =================
*/
void Mod_LoadEdges(Lump_p Lump_in) {
    dEdge_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(dEdge_t))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(dEdge_t);
    mEdge_p out = Hunk_AllocName(sizeof(mEdge_t) * (count + 1), Mod_loadName);

    _loadModel->edges = out;
    _loadModel->numedges = count;

    for (int i = 0; i < count; i++) {
        out[i].v16[0] = (uint16_t)LittleShort(in[i].v16[0]);
        out[i].v16[1] = (uint16_t)LittleShort(in[i].v16[1]);
    }
}


/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges(Lump_p Lump_in) {
    int32_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))
        Host_SysError(
            "MOD_LoadBmodel: funny lump size in %s",
            _loadModel->name
        );

    int count = Lump_in->fileLen / sizeof(*in);
    int32_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->surfedges = out;
    _loadModel->numsurfedges = count;

    for (int i = 0; i < count; i++)
        out[i] = LittleLong(in[i]);
}


/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting(Lump_p Lump_in) {
    if (!Lump_in->fileLen) {
        _loadModel->lightdata = NULL;
        return;
    }

    _loadModel->lightdata = Hunk_AllocName(Lump_in->fileLen, Mod_loadName);
    memcpy(_loadModel->lightdata, getMapLumpPtr(mod_base, Lump_in), Lump_in->fileLen);
}

/*
    =================
    Mod_LoadTexinfo
    =================
*/
void Mod_LoadTexinfo(Lump_p Lump_in) {
    TexInfo_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(*in);
    mTexInfo_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->texinfo = out;
    _loadModel->numtexinfo = count;

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < 8; j++)
            out[i].vecs[S_AX].V[j] = LittleFloat(in[i].vecs[S_AX].V[j]);

        float len1 = Length((out[i].vecs[S_AX].vx));
        float len2 = Length((out[i].vecs[T_AX].vx));
        len1 = (len1 + len2) / 2;

        /**/ if (len1 < 0.32f)  out[i].mipadjust = 4.0f;
        else if (len1 < 0.49f)  out[i].mipadjust = 3.0f;
        else if (len1 < 0.99f)  out[i].mipadjust = 2.0f;
        else /*              */ out[i].mipadjust = 1.0f;
#if 0
        if ((len1 + len2) < 0.001)      out[i].mipadjust = 1.0f;  // don't crash
        else                            out[i].mipadjust = 1 / floor((len1 + len2) / 2 + 0.1f);
#endif

        int miptex = LittleLong(in[i].miptex);
        out[i].flags = LittleLong(in[i].flags);

        if (!_loadModel->textures) {
            out[i].texture = r_notexture_mip; // checkerboard texture
            out[i].flags = 0;
        }
        else {
            if (miptex >= _loadModel->numtextures)       Host_SysError("miptex >= _loadModel->numtextures");

            out[i].texture = _loadModel->textures[miptex];
            if (!(out[i].texture)) {
                out[i].texture = r_notexture_mip; // texture not found
                out[i].flags = 0;
            }
        }
    }
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces(Lump_p Lump_in) {
    int16_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))           Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int32_t count = Lump_in->fileLen / sizeof(*in);
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
void Mod_LoadVisibility(Lump_p Lump_in) {
    if (!Lump_in->fileLen) { _loadModel->visdata = NULL; return; }

    _loadModel->visdata = Hunk_AllocName(Lump_in->fileLen, Mod_loadName);
    memcpy(_loadModel->visdata, getMapLumpPtr(mod_base, Lump_in), Lump_in->fileLen);
}


/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs(Lump_p Lump_in) {
    dLeaf_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(*in);
    mLeaf_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->leafs = out;
    _loadModel->numleafs = count;

    for (int i = 0; i < count; i++) {
        out[i].bb = (BBox_t){
            .mins = {
                .x = LittleShort(in[i].mins[X_AX]),
                .y = LittleShort(in[i].mins[Y_AX]),
                .z = LittleShort(in[i].mins[Z_AX])
            },
            .maxs = {
                .x = LittleShort(in[i].maxs[X_AX]),
                .y = LittleShort(in[i].maxs[Y_AX]),
                .z = LittleShort(in[i].maxs[Z_AX])
            }
        };

        out[i].contents = LittleLong(in[i].contents);

        out[i].firstmarksurface = _loadModel->marksurfaces + LittleShort(in[i].firstmarksurface);
        out[i].nummarksurfaces = LittleShort(in[i].nummarksurfaces);

        int p = LittleLong(in[i].visofs);
        if (p == -1)    out[i].compressed_vis = NULL;
        else            out[i].compressed_vis = _loadModel->visdata + p;
        out[i].efrags = NULL;

        for (int j = 0; j < 4; j++)
            out[i].ambient_sound_level[j] = in[i].ambient_level[j];

#ifdef GLQAUKE
        // gl underwater warp
        if (out[i].contents != CONTENTS_EMPTY) {
            for (int j = 0; j < out[i].nummarksurfaces; j++)
                out[i].firstmarksurface[j]->flags |= SURF_UNDERWATER;
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
    vec2_t mins = { .s = 999999.0f,  .t = 999999.0f };
    vec2_t maxs = { .s = -999999.0f, .t = -999999.0f };

    mTexInfo_p tex = s->texinfo;

    for (int i = 0; i < s->numedges; i++) {
        int e = _loadModel->surfedges[s->firstedge + i];
        mVertex_p v =
            &_loadModel->vertexes[
                (e >= 0) ?
                    _loadModel->edges[e].v16[0] :
                    _loadModel->edges[-e].v16[1]
            ];

        for (int j = 0; j < VECT_TX_DIM; j++) {
            float val = DotProduct(v->position, tex->vecs[j].vx) + tex->vecs[j].offs;
            if (val < mins.v[j])  mins.v[j] = val;  // not CLAMP but GET MIN
            if (val > maxs.v[j])  maxs.v[j] = val;  // not CLAMP but GET MAX
        }
    }

    int bmins[VECT_TX_DIM];
    int bmaxs[VECT_TX_DIM];
    for (int i = 0; i < VECT_TX_DIM; i++) {
        bmins[i] = floorf(mins.v[i] / 16.0f);
        bmaxs[i] = ceilf(maxs.v[i] / 16.0f);

        s->texturemins[i] = MUL16(bmins[i]);    // TODO: solve what is the mul16 and why?
        s->extents[i] = MUL16(bmaxs[i] - bmins[i]);// TODO: solve what is the mul16 and why?
        if (
            (!(tex->flags & TEX_SPECIAL)) &&
#ifdef GLQUAKE
            (s->extents[i] > 512) /* 256 */
#else
            (s->extents[i] > 256)
#endif
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
void Mod_LoadFaces(Lump_p Lump_in) {
    dFace_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))
        Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(*in);
    mSurface_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->surfaces = out;
    _loadModel->numsurfaces = count;

    for (int surfnum = 0; surfnum < count; surfnum++, out++) {
        out->firstedge = LittleLong(in[surfnum].firstedge);
        out->numedges = LittleShort(in[surfnum].numedges);
        out->flags = 0;

        int planenum = LittleShort(in[surfnum].planenum);
        int side = LittleShort(in[surfnum].side);
        if (side)
            out->flags |= SURF_PLANEBACK;

        out->plane = _loadModel->planes + planenum;
        out->texinfo = _loadModel->texinfo + LittleShort(in[surfnum].texinfo);

        CalcSurfaceExtents(out);

        // lighting info

        for (int lm = 0; lm < MAXLIGHTMAPS; lm++) {
            out->styles[lm] = in[surfnum].styles[lm];
        }

        int32_t li = LittleLong(in[surfnum].lightofs);
        if (li == -1)   out->samples = NULL;
        else            out->samples = _loadModel->lightdata + li;

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
            for (int sd = 0; sd < 2; sd++) {
                out->extents[sd] = 0x4000; // (16384)
                out->texturemins[sd] = 0xE000; // (-8192)
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
    if (node->contents < CONTENTS_NODE) return;

    Mod_SetParent(node->children[0], node);
    Mod_SetParent(node->children[1], node);
}


/*
    =================
    Mod_LoadNodes
    =================
*/
void Mod_LoadNodes(Lump_p Lump_in) {
    dNode_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(*in);
    mNode_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->nodes = out;
    _loadModel->numnodes = count;

    for (int i = 0; i < count; i++) {
        out[i].bb = (BBox_t){
            .mins = {
                .x = LittleShort(in[i].mins[X_AX]),
                .y = LittleShort(in[i].mins[Y_AX]),
                .z = LittleShort(in[i].mins[Z_AX])
            },
            .maxs = {
                .x = LittleShort(in[i].maxs[X_AX]),
                .y = LittleShort(in[i].maxs[Y_AX]),
                .z = LittleShort(in[i].maxs[Z_AX])
            }
        };

        out[i].plane = _loadModel->planes + LittleLong(in[i].planenum);

        out[i].firstsurface = LittleShort(in[i].firstface);
        out[i].numsurfaces = LittleShort(in[i].numfaces);

        for (int j = 0; j < 2; j++) {
            int p = LittleShort(in[i].children[j]);
            if (p >= 0) out[i].children[j] = _loadModel->nodes + p;
            else        out[i].children[j] = (mNode_p)(_loadModel->leafs + (-1 - p));
        }
    }

    Mod_SetParent(_loadModel->nodes, NULL); // sets nodes and leafs
}


/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes(Lump_p Lump_in) {
    dClipNode_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(*in);
    dClipNode_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->clipnodes = out;
    _loadModel->numclipnodes = count;

    _loadModel->hulls[1] = (Hull_t){
        .clipnodes = out,
        .planes = _loadModel->planes,
        .firstclipnode = 0,
        .lastclipnode = count - 1,
        .clip = {
            .mins = {.x = -16.0f, .y = -16.0f, .z = -24.0f },
            .maxs = {.x = 16.0f, .y = 16.0f, .z = 32.0f }
        }
    };
    _loadModel->hulls[2] = (Hull_t){
        .clipnodes = out,
        .planes = _loadModel->planes,
        .firstclipnode = 0,
        .lastclipnode = count - 1,
        .clip = {
            .mins = {.x = -32.0f, .y = -32.0f, .z = -24.0f },
            .maxs = {.x = 32.0f, .y = 32.0f, .z = 64.0f },
        }
    };
    for (int i = 0; i < count; i++) {
        out[i].planenum = LittleLong(in[i].planenum);
        out[i].children[0] = LittleShort(in[i].children[0]);
        out[i].children[1] = LittleShort(in[i].children[1]);
    }
}

/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities(Lump_p Lump_in) {
    if (!Lump_in->fileLen) { _loadModel->entities = NULL; return; }

    _loadModel->entities = Hunk_AllocName(Lump_in->fileLen, Mod_loadName);
    memcpy(_loadModel->entities, getMapLumpPtr(mod_base, Lump_in), Lump_in->fileLen);
}


/*
    =================
    Mod_LoadSubmodels
    =================
*/
void Mod_LoadSubmodels(Lump_p Lump_in) {
    dModel_p in = getMapLumpPtr(mod_base, Lump_in);
    if (Lump_in->fileLen % sizeof(*in))       Host_SysError("MOD_LoadBmodel: funny lump size in %s", _loadModel->name);

    int count = Lump_in->fileLen / sizeof(*in);
    dModel_p out = Hunk_AllocName(count * sizeof(*out), Mod_loadName);

    _loadModel->SubModels = out;
    _loadModel->numSubModels = count;

    for (int i = 0; i < count; i++) {
        out[i].bb.mins = VectorAddScalar(LittleVector(in[i].bb.mins), -1.f);
        out[i].bb.maxs = VectorAddScalar(LittleVector(in[i].bb.maxs), 1.f);
        out[i].origin = LittleVector(in[i].origin);

        for (int j = 0; j < MAX_MAP_HULLS; j++)
            out[i].headnode[j] = LittleLong(in[i].headnode[j]);

        out[i].visleafs = LittleLong(in[i].visleafs);
        out[i].firstface = LittleLong(in[i].firstface);
        out[i].numfaces = LittleLong(in[i].numfaces);
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
    hull->planes = _loadModel->planes;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;

    for (int i = 0; i < count; i++) {
        out[i].planenum = in[i].plane - _loadModel->planes;
        for (int j = 0; j < 2; j++) {
            mNode_p child = in[i].children[j];
            if (child->contents < CONTENTS_NODE)    out[i].children[j] = child->contents;
            else                                    out[i].children[j] = child - _loadModel->nodes;
        }
    }
}


/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs) {  // TODO: remake it with BBoxt tools
    vec3_t corner;
    for (int i = 0; i < VECT_DIM; i++) {
        corner.v[i] =
            (fabsf(mins.v[i]) > fabsf(maxs.v[i])) ?
            fabsf(mins.v[i]) : fabsf(maxs.v[i]);
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

        mod->BB = bm->bb;
        mod->radius = RadiusFromBounds(mod->BB.mins, mod->BB.maxs);

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
