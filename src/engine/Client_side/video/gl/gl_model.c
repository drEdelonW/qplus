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


Model_p loadmodel;
char Mod_loadName[32]; // for hunk tags

void Mod_LoadBrushModel(Model_p mod, TypeLess_ptr buffer);  // .bsp file
void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer);  // .mdl file
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
    if (leaf == model->leafs)       return _modNoVis;
    return Mod_DecompressVis(leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll(void) {
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
    dAliasFrame_p pdaliasframe = (dAliasFrame_p)pin;

    strcpy(frame->name, pdaliasframe->name);
    frame->firstpose = posenum;
    frame->numposes = 1;

    for (int i = 0; i < VECT_DIM; i++) {
        // these are byte values, so we don't have to worry about
        // endianness
        frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
        frame->bboxmin.v[i] = pdaliasframe->bboxmax.v[i];
    }

    TriVertx_p pinframe = (TriVertx_p)(pdaliasframe + 1);

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
    dAliasGroup_p pingroup = (dAliasGroup_p)pin;

    int numframes = LittleLong(pingroup->numframes);

    frame->firstpose = posenum;
    frame->numposes = numframes;

    for (int i = 0; i < VECT_DIM; i++) {
        // these are byte values, so we don't have to worry about endianness
        frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
        frame->bboxmin.v[i] = pingroup->bboxmax.v[i];
    }

    dAliasInterval_p pin_intervals = (dAliasInterval_p)(pingroup + 1);

    frame->interval = LittleFloat(pin_intervals->interval);

    pin_intervals += numframes;

    TypeLess_ptr ptemp = (TypeLess_ptr)pin_intervals;

    for (int i = 0; i < numframes; i++) {
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
    int16_t x;
    int16_t y;
} floodfill_t;


// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
    if (pos[off] == fillcolor) {\
        pos[off] = 255; \
        fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
        inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
    } \
    else if (pos[off] != 255) fdc = pos[off]; \
}

void Mod_FloodFillSkin(uint8_p skin, int skinwidth, int skinheight) {
    int filledcolor = -1;
    if (filledcolor == -1) {
        filledcolor = 0;
        // attempt to find opaque black
        for (int i = 0; i < 256; ++i)
            if (d_8to24table[i] == (255 << 0)) {// alpha 1.0
                filledcolor = i;
                break;
            }
    }

    byte    fillcolor = *skin; // assume this is the pixel to fill
    // can't fill to filled color or to transparent color (used as visited marker)
    if ((fillcolor == filledcolor) || (fillcolor == 255)) {
        //printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
        return;
    }

    int inpt = 0;
    floodfill_t fifo[FLOODFILL_FIFO_SIZE];
    fifo[inpt].x = 0;
    fifo[inpt].y = 0;
    inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

    int outpt = 0;
    while (outpt != inpt) {
        int x = fifo[outpt].x;
        int y = fifo[outpt].y;
        int fdc = filledcolor;
        uint8_p pos = &skin[x + skinwidth * y];

        outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

        if (x > 0)              FLOODFILL_STEP(-1, -1, 0);
        if (x < skinwidth - 1)  FLOODFILL_STEP(1, 1, 0);
        if (y > 0)              FLOODFILL_STEP(-skinwidth, 0, -1);
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
    uint8_p skin = (uint8_p)(pskintype + 1);

    if ((numskins < 1) || (numskins > MAX_SKINS))   Host_SysError("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

    int s = pheader->skinwidth * pheader->skinheight;

    for (int i = 0; i < numskins; i++) {
        if (pskintype->type == ALIAS_SKIN_SINGLE) {
            Mod_FloodFillSkin(skin, pheader->skinwidth, pheader->skinheight);

            // save 8 bit texels for the player model to remap
    //  if (!strcmp(loadmodel->name,"progs/player.mdl")) {
            uint8_p texels = Hunk_AllocName(s, Mod_loadName);
            pheader->texels[i] = texels - (uint8_p)pheader;
            memcpy(texels, (uint8_p)(pskintype + 1), s);
            //  }
            char name[32];  snprintf(name, sizeof(name), "%s_%i", loadmodel->name, i);
            pheader->gl_texturenum[i][0] =
                pheader->gl_texturenum[i][1] =
                pheader->gl_texturenum[i][2] =
                pheader->gl_texturenum[i][3] =
                GL_LoadTexture(
                    name, pheader->skinwidth,
                    pheader->skinheight, (uint8_p)(pskintype + 1), true, false
                );
            pskintype = (dAliasSkinType_p)((uint8_p)(pskintype + 1) + s);
        }
        else {
            // animating skin group.  yuck.
            pskintype++;
            dAliasSkinGroup_p pinskingroup = (dAliasSkinGroup_p)pskintype;
            int groupskins = LittleLong(pinskingroup->numskins);
            dAliasSkinInterval_p pinskinintervals = (dAliasSkinInterval_p)(pinskingroup + 1);

            pskintype = (TypeLess_ptr)(pinskinintervals + groupskins);

            int j = 0;
            for (/* */; j < groupskins; j++) {
                Mod_FloodFillSkin(skin, pheader->skinwidth, pheader->skinheight);
                if (j == 0) {
                    uint8_p texels = Hunk_AllocName(s, Mod_loadName);
                    pheader->texels[i] = texels - (uint8_p)pheader;
                    memcpy(texels, (uint8_p)(pskintype), s);
                }
                char name[32];  snprintf(name, sizeof(name), "%s_%i_%i", loadmodel->name, i, j);
                pheader->gl_texturenum[i][j & 3] =
                    GL_LoadTexture(
                        name, pheader->skinwidth,
                        pheader->skinheight, (uint8_p)(pskintype), true, false
                    );
                pskintype = (dAliasSkinType_p)((uint8_p)(pskintype)+s);
            }
            int k = j;
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
    int start = Hunk_LowMark();

    Mdl_p pinmodel = (Mdl_p)buffer;

    int version = LittleLong(pinmodel->version);
    if (version != ALIAS_VERSION)        Host_SysError("%s has wrong version number (%i should be %i)", mod->name, version, ALIAS_VERSION);

    //
    // allocate space for a working header, plus all the data except the frames,
    // skin and group info
    //
    pheader = Hunk_AllocName(
        sizeof(AliasHdr_t) +
        (LittleLong(pinmodel->numframes) - 1) * sizeof(pheader->frames[0]),
        Mod_loadName
    );

    mod->flags = LittleLong(pinmodel->flags);

    //
    // endian-adjust and copy the data, starting with the alias model header
    //
    pheader->boundingradius = LittleFloat(pinmodel->boundingradius);
    pheader->numskins = LittleLong(pinmodel->numskins);
    pheader->skinwidth = LittleLong(pinmodel->skinwidth);
    pheader->skinheight = LittleLong(pinmodel->skinheight);
    if (pheader->skinheight > MAX_LBM_HEIGHT)       Host_SysError("model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT);

    pheader->numverts = LittleLong(pinmodel->numverts);
    if (pheader->numverts <= 0)                     Host_SysError("model %s has no vertices", mod->name);
    if (pheader->numverts > MAXALIASVERTS)          Host_SysError("model %s has too many vertices", mod->name);

    pheader->numtris = LittleLong(pinmodel->numtris);
    if (pheader->numtris <= 0)                      Host_SysError("model %s has no triangles", mod->name);

    pheader->numframes = LittleLong(pinmodel->numframes);
    int numframes = pheader->numframes;
    if (numframes < 1)                              Host_SysError("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

    pheader->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
    mod->synctype = LittleLong(pinmodel->synctype);
    mod->numframes = pheader->numframes;

    for (int i = 0; i < VECT_DIM; i++) {
        pheader->scale[i] = LittleFloat(pinmodel->scale[i]);
        pheader->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
        pheader->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
    }


    //
    // load the skins
    //
    dAliasSkinType_p pskintype = (dAliasSkinType_p)&pinmodel[1];
    pskintype = Mod_LoadAllSkins(pheader->numskins, pskintype);

    //
    // load base s and t vertices
    //
    stvert_p pinstverts = (stvert_p)pskintype;

    for (int i = 0; i < pheader->numverts; i++) {
        stverts[i].onseam = LittleLong(pinstverts[i].onseam);
        stverts[i].s = LittleLong(pinstverts[i].s);
        stverts[i].t = LittleLong(pinstverts[i].t);
    }

    //
    // load triangle lists
    //
    dTriangle_p pintriangles = (dTriangle_p)&pinstverts[pheader->numverts];

    for (int i = 0; i < pheader->numtris; i++) {
        triangles[i].facesfront = LittleLong(pintriangles[i].facesfront);

        for (int j = 0; j < 3; j++) {
            triangles[i].vertindex[j] = LittleLong(pintriangles[i].vertindex[j]);
        }
    }

    //
    // load the frames
    //
    posenum = 0;
    dAliasFrameType_p pframetype = (dAliasFrameType_p)&pintriangles[pheader->numtris];

    for (int i = 0; i < numframes; i++) {
        AliasFrameType_t frametype;

        frametype = LittleLong(pframetype->type);

        if (frametype == ALIAS_SINGLE)  pframetype = (dAliasFrameType_p)Mod_LoadAliasFrame(pframetype + 1, &pheader->frames[i]);
        else                            pframetype = (dAliasFrameType_p)Mod_LoadAliasGroup(pframetype + 1, &pheader->frames[i]);

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
    int end = Hunk_LowMark();
    int total = end - start;

    Cache_Alloc(&mod->cache, total, Mod_loadName);
    if (!mod->cache.data)   return;

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
void Mod_Print(void) {
    Con_Printf("Cached models:\n");
    Model_p mod = _mod_Known;
    for (int i = 0; i < _mod_NumKnown; i++, mod++) {
        Con_Printf("%8p : %s\n", mod->cache.data, mod->name);
    }
}


