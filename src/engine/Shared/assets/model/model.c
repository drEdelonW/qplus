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
#include "AliasModel.h"
#include "BrushModel.h"
#include "SpriteModel.h"

Model_p _loadModel;
char Mod_loadName[32]; // for hunk tags

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


uint8_p mod_base;


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


