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
#include "q_tools.h"
#include "mathlib.h"
#include "Surface.h"
#include "Sprite.h"
#include "console.h"
#include "z_hunk.h"
#include "AliasModel.h"
#include "BrushModel.h"
#include "SpriteModel.h"

Model_p _loadModel;
char Mod_loadName[32]; // for hunk tags

Model_p Mod_LoadModel(Model_p mod, bool crash);

uint8_t _modNoVis[MAX_MAP_LEAFS / 8];

#define MAX_MOD_KNOWN 512
static uint16_t  _mod_NumKnown = 0;
static Model_t _mod_Known[MAX_MOD_KNOWN];

cvar_t gl_subdivide_size = { "gl_subdivide_size", "128", true };    // GL diff

/*
===============
Mod_Init
===============
*/
void Mod_Init() {
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



/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll() {
    Model_p mod = _mod_Known;
    for (int i = 0; i < _mod_NumKnown; i++, mod++)
        if (mod->type != mod_alias)
            mod->needload = true; // TODO: this is not bool! check it!!!
}

/*
==================
Mod_FindName

==================
*/
Model_p Mod_FindName(cString name) {
    if (!name[0])   Host_SysError("Mod_ForName: NULL name");

    //
    // search the currently loaded models
    //
    Model_p mod = _mod_Known;
    int  i = 0;
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
    Model_p mod = Mod_FindName(name);

    if ((!mod->needload) &&
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
    if (!mod->needload) {
        if ((mod->type == mod_alias) &&
            (Cache_Check(&mod->cache))
            ) {
            return mod;
        }
        else
            return mod;  // not cached at all
    }

    //
    // because the world is so huge, load it one piece at a time
    //
    if (!crash) {}

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


stVert_t stverts[MAXALIASVERTS];
mTriangle_t triangles[MAXALIASTRIS];

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
TriVertx_p poseverts[MAXALIASFRAMES];

uint8_p* player_8bit_texels_tbl;
uint8_p player_8bit_texels;




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
        Con_Printf("%8p : %s\n", mod->cache.data, mod->name);
    }
}


