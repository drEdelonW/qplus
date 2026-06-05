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

#include "model.h"
#include <string.h>
#include "host.h"
#include "common.h"
#include "q_tools.h"
#include "console.h"
#include "d_iface.h"
#include "r_local.h"
#include "endian_tools.h"
#include "z_hunk.h"

Model_p _loadModel;
char Mod_loadName[32]; // for hunk tags

void Mod_LoadBrushModel(Model_p mod, TypeLess_ptr buffer);  // .bsp file
void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer);  // .mdl file
Model_p Mod_LoadModel(Model_p mod, bool crash);

static uint8_t _modNoVis[MAX_MAP_LEAFS / 8];

#define MAX_MOD_KNOWN 256
static uint16_t _mod_NumKnown = 0;
static Model_t _mod_Known[MAX_MOD_KNOWN];

// values for Model_t's needload

/*
===============
Mod_Init
===============
*/
void Mod_Init() {
    memset(_modNoVis, 0xFF, sizeof(_modNoVis));
}

/*
===============
Mod_Extradata

Caches the data if needed
===============
*/
TypeLess_ptr Mod_Extradata(Model_p mod) {
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
mLeaf_p Mod_PointInLeaf(vec3_t p, Model_p model) {
    if ((!model) || (!model->nodes))    Host_SysError("Mod_PointInLeaf: bad model");

    mNode_p node = model->nodes;
    while (1) {
        if (node->contents < 0)     return (mLeaf_p)node;

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

    return _decompressed;
}

uint8_p Mod_LeafPVS(mLeaf_p leaf, Model_p model) {
    if (leaf == model->leafs)       return _modNoVis;
    return Mod_DecompressVis(leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll() {
    Model_p mod = _mod_Known;
    for (int i = 0; i < _mod_NumKnown; i++, mod++) {
        mod->needload = NL_UNREFERENCED;
        //FIX FOR CACHE_ALLOC ERRORS:
        if (mod->type == mod_sprite)
            mod->cache.data = NULL;
    }
}

/*
==================
Mod_FindName

==================
*/
Model_p Mod_FindName(cString name) {
    Model_p avail = NULL;

    if (!name[0])   Host_SysError("Mod_ForName: NULL name");

    //
    // search the currently loaded models
    //
    Model_p mod = _mod_Known;
    int i = 0;
    for (; i < _mod_NumKnown; i++, mod++) {
        if (!strcmp(mod->name, name))   break;

        if ((mod->needload == NL_UNREFERENCED) &&
            (!avail || (mod->type != mod_alias))
            )
            avail = mod;
    }

    if (i == _mod_NumKnown) {
        if (_mod_NumKnown == MAX_MOD_KNOWN) {
            if (avail) {
                mod = avail;
                if ((mod->type == mod_alias) &&
                    (Cache_Check(&mod->cache)))
                    Cache_Free(&mod->cache);
            }
            else    Host_SysError("_mod_NumKnown == MAX_MOD_KNOWN");
        }
        else    _mod_NumKnown++;
        strcpy(mod->name, name);
        mod->needload = NL_NEEDS_LOADED;
    }

    return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel(cString name) {
    Model_p mod = Mod_FindName(name);

    if ((mod->needload == NL_PRESENT) &&
        (mod->type == mod_alias)
        )
        Cache_Check(&mod->cache);
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
Model_p Mod_LoadModel(Model_p mod, bool crash) {
    if (
        (mod->type == mod_alias) &&
        (Cache_Check(&mod->cache))
        ) {
        mod->needload = NL_PRESENT;
        return mod;
    }
    else {
        if (mod->needload == NL_PRESENT)        return mod;
    }

    //
    // because the world is so huge, load it one piece at a time
    //

    //
    // load the file
    //
    uint8_t stackbuf[1024];  // avoid dirtying the cache heap
    uint32_p buf = (uint32_p)COM_LoadStackFile(mod->name, stackbuf, sizeof(stackbuf));
    if (!buf) {
        if (crash)      Host_SysError("Mod_NumForName: %s not found", mod->name);

        return NULL;
    }

    //
    // allocate a new model
    //
    COM_FileBase(mod->name, Mod_loadName);

    _loadModel = mod;

    //
    // fill it in
    //

    // call the apropriate loader
    mod->needload = NL_PRESENT;

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
    Model_p mod = Mod_FindName(name);
    return Mod_LoadModel(mod, crash);
}


/*
===============================================================================

                    BRUSHMODEL LOADING

===============================================================================
*/

uint8_p mod_base;


/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/*
=================
Mod_LoadAliasFrame
=================
*/
TypeLess_ptr Mod_LoadAliasFrame(
    TypeLess_ptr  pin,
    int32_p     pframeindex,
    int32_t     numv,
    TriVertx_p  pbboxmin,
    TriVertx_p  pbboxmax,
    AliasHdr_p  pheader,
    cString     name
) {
    dAliasFrame_p pdaliasframe = (dAliasFrame_p)pin;

    strcpy(name, pdaliasframe->name);

    for (int i = 0; i < VECT_DIM; i++) {
        // these are uint8_t values, so we don't have to worry about
        // endianness
        pbboxmin->v[i] = pdaliasframe->bboxmin.v[i];
        pbboxmax->v[i] = pdaliasframe->bboxmax.v[i];
    }

    TriVertx_p pinframe = (TriVertx_p)(pdaliasframe + 1);
    TriVertx_p pframe = Hunk_AllocName(numv * sizeof(*pframe), Mod_loadName);

    *pframeindex = (uint8_p)pframe - (uint8_p)pheader;

    for (int j = 0; j < numv; j++) {
        // these are all uint8_t values, so no need to deal with endianness
        pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

        for (int k = 0; k < 3; k++) {
            pframe[j].v[k] = pinframe[j].v[k];
        }
    }

    pinframe += numv;

    return (TypeLess_ptr)pinframe;
}


/*
=================
Mod_LoadAliasGroup
=================
*/
TypeLess_ptr  Mod_LoadAliasGroup(
    TypeLess_ptr  pin,
    int32_p pframeindex,
    int numv,
    TriVertx_p pbboxmin,
    TriVertx_p pbboxmax,
    AliasHdr_p pheader,
    cString name
) {

    dAliasGroup_p pingroup = (dAliasGroup_p)pin;
    int32_t numframes = LittleLong(pingroup->numframes);
    mAliasGroup_p paliasgroup = Hunk_AllocName(
        sizeof(mAliasGroup_t) + (numframes - 1) * sizeof(paliasgroup->frames[0]), Mod_loadName);
    paliasgroup->numframes = numframes;

    for (int i = 0; i < VECT_DIM; i++) {
        // these are uint8_t values, so we don't have to worry about endianness
        pbboxmin->v[i] = pingroup->bboxmin.v[i];
        pbboxmax->v[i] = pingroup->bboxmax.v[i];
    }

    *pframeindex = (uint8_p)paliasgroup - (uint8_p)pheader;
    dAliasInterval_p pin_intervals = (dAliasInterval_p)(pingroup + 1);
    float_p poutintervals = Hunk_AllocName(numframes * sizeof(float), Mod_loadName);
    paliasgroup->intervals = (uint8_p)poutintervals - (uint8_p)pheader;

    for (int i = 0; i < numframes; i++) {
        *poutintervals = LittleFloat(pin_intervals->interval);
        if (*poutintervals <= 0.0)      Host_SysError("Mod_LoadAliasGroup: interval<=0");

        poutintervals++;
        pin_intervals++;
    }

    TypeLess_ptr ptemp = (TypeLess_ptr)pin_intervals;

    for (int i = 0; i < numframes; i++) {
        ptemp = Mod_LoadAliasFrame(
            ptemp,
            &paliasgroup->frames[i].frame,
            numv,
            &paliasgroup->frames[i].bboxmin,
            &paliasgroup->frames[i].bboxmax,
            pheader, name
        );
    }

    return ptemp;
}


/*
=================
Mod_LoadAliasSkin
=================
*/
TypeLess_ptr Mod_LoadAliasSkin(TypeLess_ptr pin, int32_p pskinindex, int skinsize, AliasHdr_p pheader) {
    uint8_p pskin = Hunk_AllocName(skinsize * r_pixbytes, Mod_loadName);
    uint8_p pinskin = (uint8_p)pin;
    *pskinindex = (uint8_p)pskin - (uint8_p)pheader;

    if (r_pixbytes == 1)    Q_memcpy(pskin, pinskin, skinsize);
    else if (r_pixbytes == 2) {
        uint16_p pusskin = (uint16_p)pskin;
        for (int i = 0; i < skinsize; i++)
            pusskin[i] = d_8to16table[pinskin[i]];
    }
    else                    Host_SysError("Mod_LoadAliasSkin: driver set invalid r_pixbytes: %d\n", r_pixbytes);

    pinskin += skinsize;
    return ((TypeLess_ptr)pinskin);
}


/*
=================
Mod_LoadAliasSkinGroup
=================
*/
TypeLess_ptr Mod_LoadAliasSkinGroup(TypeLess_ptr pin, int32_p pskinindex, int32_t skinsize, AliasHdr_p pheader) {
    dAliasSkinGroup_p pinskingroup = (dAliasSkinGroup_p)pin;
    int32_t numskins = LittleLong(pinskingroup->numskins);
    mAliasSkinGroup_p paliasskingroup = Hunk_AllocName(
        sizeof(mAliasSkinGroup_t) + (numskins - 1) * sizeof(paliasskingroup->skindescs[0]),
        Mod_loadName
    );

    paliasskingroup->numskins = numskins;
    *pskinindex = (uint8_p)paliasskingroup - (uint8_p)pheader;
    dAliasSkinInterval_p pinskinintervals = (dAliasSkinInterval_p)(pinskingroup + 1);
    float_p poutskinintervals = Hunk_AllocName(numskins * sizeof(float), Mod_loadName);
    paliasskingroup->intervals = (uint8_p)poutskinintervals - (uint8_p)pheader;

    for (int32_t i = 0; i < numskins; i++) {
        *poutskinintervals = LittleFloat(pinskinintervals->interval);
        if (*poutskinintervals <= 0)        Host_SysError("Mod_LoadAliasSkinGroup: interval<=0");

        poutskinintervals++;
        pinskinintervals++;
    }

    TypeLess_ptr ptemp = (TypeLess_ptr)pinskinintervals;
    for (int32_t i = 0; i < numskins; i++) {
        ptemp = Mod_LoadAliasSkin(
            ptemp, &paliasskingroup->skindescs[i].skin, skinsize, pheader
        );
    }

    return ptemp;
}


/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer) {
    size_t start = Hunk_LowMark();

    Mdl_p pinmodel = (Mdl_p)buffer;

    int version = LittleLong(pinmodel->version);
    if (version != ALIAS_VERSION)       Host_SysError("%s has wrong version number (%i should be %i)", mod->name, version, ALIAS_VERSION);

    //
    // allocate space for a working header, plus all the data except the frames,
    // skin and group info
    //

    AliasHdr_p pheader = Hunk_AllocName(
        sizeof(AliasHdr_t) +
        (LittleLong(pinmodel->numframes) - 1) * sizeof(pheader->frames[0]) +
        sizeof(Mdl_t) +
        LittleLong(pinmodel->numverts) * sizeof(stvert_t) +
        LittleLong(pinmodel->numtris) * sizeof(mTriangle_t),
        Mod_loadName
    );
    Mdl_p pmodel = (Mdl_p)((uint8_p)&pheader[1] +
        (LittleLong(pinmodel->numframes) - 1) *
        sizeof(pheader->frames[0]));

    // mod->cache.data = pheader;
    mod->flags = LittleLong(pinmodel->flags);

    //
    // endian-adjust and copy the data, starting with the alias model header
    //
    pmodel->boundingradius = LittleFloat(pinmodel->boundingradius);
    pmodel->numskins = LittleLong(pinmodel->numskins);
    pmodel->skinwidth = LittleLong(pinmodel->skinwidth);

    pmodel->skinheight = LittleLong(pinmodel->skinheight);
    if (pmodel->skinheight > MAX_LBM_HEIGHT)    Host_SysError("model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT);

    pmodel->numverts = LittleLong(pinmodel->numverts);
    if (pmodel->numverts <= 0)                  Host_SysError("model %s has no vertices", mod->name);
    if (pmodel->numverts > MAXALIASVERTS)       Host_SysError("model %s has too many vertices", mod->name);

    pmodel->numtris = LittleLong(pinmodel->numtris);
    if (pmodel->numtris <= 0)                   Host_SysError("model %s has no triangles", mod->name);

    pmodel->numframes = LittleLong(pinmodel->numframes);
    pmodel->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
    mod->synctype = LittleLong(pinmodel->synctype);
    mod->numframes = pmodel->numframes;

    for (int i = 0; i < VECT_DIM; i++) {
        pmodel->scale[i] = LittleFloat(pinmodel->scale[i]);
        pmodel->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
        pmodel->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
    }

    int numskins = pmodel->numskins;
    int numframes = pmodel->numframes;

    if (pmodel->skinwidth & 0x03)               Host_SysError("Mod_LoadAliasModel: skinwidth not multiple of 4");

    pheader->model = (uint8_p)pmodel - (uint8_p)pheader;

    //
    // load the skins
    //
    int skinsize = pmodel->skinheight * pmodel->skinwidth;

    if (numskins < 1)                           Host_SysError("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

    dAliasSkinType_p pskintype = (dAliasSkinType_p)&pinmodel[1];
    mAliasSkinDesc_p pskindesc = Hunk_AllocName(numskins * sizeof(mAliasSkinDesc_t), Mod_loadName);

    pheader->skindesc = (uint8_p)pskindesc - (uint8_p)pheader;

    for (int i = 0; i < numskins; i++) {
        AliasSkinType_t skintype;

        skintype = LittleLong(pskintype->type);
        pskindesc[i].type = skintype;

        if (skintype == ALIAS_SKIN_SINGLE)  pskintype = (dAliasSkinType_p)Mod_LoadAliasSkin(pskintype + 1, &pskindesc[i].skin, skinsize, pheader);
        else                                pskintype = (dAliasSkinType_p)Mod_LoadAliasSkinGroup(pskintype + 1, &pskindesc[i].skin, skinsize, pheader);

    }

    //
    // set base s and t vertices
    //
    stvert_p pstverts = (stvert_p)&pmodel[1];
    stvert_p pinstverts = (stvert_p)pskintype;

    pheader->stverts = (uint8_p)pstverts - (uint8_p)pheader;

    for (int32_t i = 0; i < pmodel->numverts; i++) {
        pstverts[i].onseam = LittleLong(pinstverts[i].onseam);
        // put s and t in 16.16 format
        pstverts[i].s = LittleLong(pinstverts[i].s) << 16;
        pstverts[i].t = LittleLong(pinstverts[i].t) << 16;
    }

    //
    // set up the triangles
    //
    mTriangle_p ptri = (mTriangle_p)&pstverts[pmodel->numverts];
    dTriangle_p pintriangles = (dTriangle_p)&pinstverts[pmodel->numverts];

    pheader->triangles = (uint8_p)ptri - (uint8_p)pheader;

    for (int32_t i = 0; i < pmodel->numtris; i++) {
        ptri[i].facesfront = LittleLong(pintriangles[i].facesfront);

        for (int j = 0; j < 3; j++) {
            ptri[i].vertindex[j] = LittleLong(pintriangles[i].vertindex[j]);
        }
    }

    //
    // load the frames
    //
    if (numframes < 1)          Host_SysError("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

    dAliasFrameType_p pframetype = (dAliasFrameType_p)&pintriangles[pmodel->numtris];

    for (int i = 0; i < numframes; i++) {
        AliasFrameType_t frametype = LittleLong(pframetype->type);
        pheader->frames[i].type = frametype;

        if (frametype == ALIAS_SINGLE) {
            pframetype = (dAliasFrameType_p)
                Mod_LoadAliasFrame(
                    pframetype + 1,
                    &pheader->frames[i].frame,
                    pmodel->numverts,
                    &pheader->frames[i].bboxmin,
                    &pheader->frames[i].bboxmax,
                    pheader, pheader->frames[i].name
                );
        }
        else {
            pframetype = (dAliasFrameType_p)
                Mod_LoadAliasGroup(
                    pframetype + 1,
                    &pheader->frames[i].frame,
                    pmodel->numverts,
                    &pheader->frames[i].bboxmin,
                    &pheader->frames[i].bboxmax,
                    pheader, pheader->frames[i].name
                );
        }
    }

    mod->type = mod_alias;

    // FIXME: do this right
    mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
    mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;

    //
    // move the complete, relocatable alias model to the cache
    //
    size_t total = Hunk_LowMark() - start;

    Cache_Alloc(&mod->cache, total, Mod_loadName);
    if (!mod->cache.data)       return;

    memcpy(mod->cache.data, pheader, total);

    Hunk_FreeToLowMark(start);
}

//=============================================================================



//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print() {
    Con_Printf("Cached models:\n");
    Model_p mod = _mod_Known;
    for (int i = 0; i < _mod_NumKnown; i++, mod++) {
        Con_Printf("[%d] %8p : %s", i, mod->cache.data, mod->name);
        if (mod->needload & NL_UNREFERENCED)    Con_Printf(" (!R)");
        if (mod->needload & NL_NEEDS_LOADED)    Con_Printf(" (!P)");
        Con_Printf("\n");
    }
}


