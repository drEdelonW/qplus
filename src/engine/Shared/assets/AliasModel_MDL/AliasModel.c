#include "Alias.h"
#include "AliasModel.h"

#ifdef GLQUAKE
#   include "qOpenGL.h"
#else
#   include "r_local.h"
#   include "d_iface.h"
#endif
#include "endian_tools.h"
#include "host.h"
#include "z_hunk.h"
#include "q_tools.h"
#include <string.h>

#ifdef GLQUAKE

int   posenum;
AliasHdr_p pheader;

#endif

AliasHdr_p  paliashdr;

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
#ifdef GLQUAKE
dAliasFrameType_p Mod_LoadAliasFrame(TypeLess_ptr pin, mAliasFrameDesc_p frame) {
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

    return (dAliasFrameType_p)pinframe;
}

#else
dAliasFrameType_p Mod_LoadAliasFrame(
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
    TriVertx_p pframe = Hunk_AllocName(
        sizeof(*pframe) * numv,
        Mod_loadName
    );

    *pframeindex = (uint8_p)pframe - (uint8_p)pheader;

    for (int j = 0; j < numv; j++) {
        // these are all uint8_t values, so no need to deal with endianness
        pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

        for (int k = 0; k < 3; k++) {
            pframe[j].v[k] = pinframe[j].v[k];
        }
    }

    pinframe += numv;

    return (dAliasFrameType_p)pinframe;
}
#endif


/*
=================
Mod_LoadAliasGroup
=================
*/
#ifdef GLQUAKE
dAliasFrameType_p Mod_LoadAliasGroup(
    TypeLess_ptr pin,
    mAliasFrameDesc_p frame
) {
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
#else
dAliasFrameType_p  Mod_LoadAliasGroup(
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
        sizeof(mAliasGroup_t) +
        sizeof(paliasgroup->frames[0]) * (numframes - 1),
        Mod_loadName
    );
    paliasgroup->numframes = numframes;

    for (int i = 0; i < VECT_DIM; i++) {
        // these are uint8_t values, so we don't have to worry about endianness
        pbboxmin->v[i] = pingroup->bboxmin.v[i];
        pbboxmax->v[i] = pingroup->bboxmax.v[i];
    }

    *pframeindex = (uint8_p)paliasgroup - (uint8_p)pheader;
    dAliasInterval_p pin_intervals = (dAliasInterval_p)(pingroup + 1);
    float_p poutintervals = Hunk_AllocName(
        sizeof(float) * numframes,
        Mod_loadName
    );
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
#endif

#ifndef GLQUAKE
/*
=================
Mod_LoadAliasSkin
=================
*/
dAliasSkinType_p Mod_LoadAliasSkin(
    TypeLess_ptr pin,
    int32_p pskinindex,
    int skinsize,
    AliasHdr_p pheader
) {
    uint8_p pskin = Hunk_AllocName(skinsize * r_pixbytes, Mod_loadName);
    uint8_p pinskin = (uint8_p)pin;
    *pskinindex = (uint8_p)pskin - (uint8_p)pheader;

#if 0
    if (r_pixbytes == 1)    Q_memcpy(pskin, pinskin, skinsize);
    else if (r_pixbytes == 2) {
        uint16_p pusskin = (uint16_p)pskin;
        for (int i = 0; i < skinsize; i++)
            pusskin[i] = d_8to16table[pinskin[i]];
    }
    else                    Host_SysError("Mod_LoadAliasSkin: driver set invalid r_pixbytes: %d\n", r_pixbytes);
#else
    switch (r_pixbytes) {
    case 1: { Q_memcpy(pskin, pinskin, skinsize); } break;
    case 2: {
        uint16_p pusskin = (uint16_p)pskin;
        for (int i = 0; i < skinsize; i++)
            pusskin[i] = d_8to16table[pinskin[i]];
    } break;
    default: { Host_SysError("Mod_LoadAliasSkin: driver set invalid r_pixbytes: %d\n", r_pixbytes); } break;
    }
#endif

    pinskin += skinsize;
    return ((dAliasSkinType_p)pinskin);
}
#endif

#ifdef GLQUAKE

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
        if (pskintype->type == ALIAS_SKIN_SINGLE) { // TODO make throu swith case 
            Mod_FloodFillSkin(skin, pheader->skinwidth, pheader->skinheight);

            // save 8 bit texels for the player model to remap
    //  if (!strcmp(_loadModel->name,"progs/player.mdl")) {
            uint8_p texels = Hunk_AllocName(s, Mod_loadName);
            pheader->texels[i] = texels - (uint8_p)pheader;
            memcpy(texels, (uint8_p)(pskintype + 1), s);
            //  }
            char name[32];  snprintf(name, sizeof(name), "%s_%i", _loadModel->name, i);
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
                char name[32];  snprintf(name, sizeof(name), "%s_%i_%i", _loadModel->name, i, j);
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


#else
/*
=================
Mod_LoadAliasSkinGroup
=================
*/
dAliasSkinType_p Mod_LoadAliasSkinGroup(
    TypeLess_ptr pin,
    int32_p pskinindex,
    int32_t skinsize,
    AliasHdr_p pheader
) {
    dAliasSkinGroup_p pinskingroup = (dAliasSkinGroup_p)pin;
    int32_t numskins = LittleLong(pinskingroup->numskins);
    mAliasSkinGroup_p paliasskingroup = Hunk_AllocName(
        sizeof(mAliasSkinGroup_t) +
        sizeof(paliasskingroup->skindescs[0]) * (numskins - 1),
        Mod_loadName
    );

    paliasskingroup->numskins = numskins;
    *pskinindex = (uint8_p)paliasskingroup - (uint8_p)pheader;
    dAliasSkinInterval_p pinskinintervals = (dAliasSkinInterval_p)(pinskingroup + 1);
    float_p poutskinintervals = Hunk_AllocName(
        sizeof(float) * numskins,
        Mod_loadName
    );
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

    return (dAliasSkinType_p)ptemp;
}
#endif

/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer) {
    size_t start = Hunk_LowMark();
    Mdl_p pinmodel = (Mdl_p)buffer;

    int32_t version = LittleLong(pinmodel->version);
#ifdef GLQUAKE
    if (version != ALIAS_VERSION)        Host_SysError("%s has wrong version number (%i should be %i)", mod->name, version, ALIAS_VERSION);

    //
    // allocate space for a working header, plus all the data except the frames,
    // skin and group info
    //
    pheader = Hunk_AllocName(
        sizeof(AliasHdr_t) +
        sizeof(pheader->frames[0]) * (LittleLong(pinmodel->numframes) - 1),
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

#   if 0
        if (frametype == ALIAS_SINGLE)  pframetype = Mod_LoadAliasFrame(pframetype + 1, &pheader->frames[i]);
        else                            pframetype = Mod_LoadAliasGroup(pframetype + 1, &pheader->frames[i]);
#   else
        switch (frametype) {
        case ALIAS_SINGLE: { pframetype = Mod_LoadAliasFrame(pframetype + 1, &pheader->frames[i]); } break;
        case ALIAS_GROUP: { pframetype = Mod_LoadAliasGroup(pframetype + 1, &pheader->frames[i]); } break;
        default: { Host_Error("frametype [0x%X] UNKNOWN!\n", frametype); } break;
        }
#   endif

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

#else
// void Mod_LoadAliasModel(Model_p mod, TypeLess_ptr buffer) {
//     size_t start = Hunk_LowMark();
//     Mdl_p pinmodel = (Mdl_p)buffer;

//     int32_t version = LittleLong(pinmodel->version);
    if (version != ALIAS_VERSION)       Host_SysError("%s has wrong version number (%i should be %i)", mod->name, version, ALIAS_VERSION);

    //
    // allocate space for a working header, plus all the data except the frames,
    // skin and group info
    //

    AliasHdr_p pheader = Hunk_AllocName(
        sizeof(AliasHdr_t) +
        sizeof(pheader->frames[0]) * (LittleLong(pinmodel->numframes) - 1) +
        sizeof(Mdl_t) +
        sizeof(stvert_t) * LittleLong(pinmodel->numverts) +
        sizeof(mTriangle_t) * LittleLong(pinmodel->numtris),
        Mod_loadName
    );
    Mdl_p pmodel = (Mdl_p)(
        (uint8_p)&pheader[1] +
        sizeof(pheader->frames[0]) * (LittleLong(pinmodel->numframes) - 1)
        );

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
    mAliasSkinDesc_p pskindesc = Hunk_AllocName(
        sizeof(mAliasSkinDesc_t) * numskins,
        Mod_loadName
    );

    pheader->skindesc = (uint8_p)pskindesc - (uint8_p)pheader;

    for (int i = 0; i < numskins; i++) {
        AliasSkinType_t skintype = LittleLong(pskintype->type);
        pskindesc[i].type = skintype;

#   if 0
        if (skintype == ALIAS_SKIN_SINGLE)  pskintype = Mod_LoadAliasSkin(pskintype + 1, &pskindesc[i].skin, skinsize, pheader);
        else                                pskintype = Mod_LoadAliasSkinGroup(pskintype + 1, &pskindesc[i].skin, skinsize, pheader);
#   else
        switch (skintype) {
        case ALIAS_SKIN_SINGLE: { pskintype = Mod_LoadAliasSkin(pskintype + 1, &pskindesc[i].skin, skinsize, pheader); } break;
        case ALIAS_SKIN_GROUP: { pskintype = Mod_LoadAliasSkinGroup(pskintype + 1, &pskindesc[i].skin, skinsize, pheader); } break;
        default: { Host_Error("skintype [0x%X] UNKNOWN!\n", skintype); } break;
        }
#   endif
    }

    //
    // set base s and t vertices
    //
    stvert_p pstverts = (stvert_p)&pmodel[1];
    stvert_p pinstverts = (stvert_p)pskintype;

    pheader->stverts = (uint8_p)pstverts - (uint8_p)pheader;

    for (int32_t i = 0; i < pmodel->numverts; i++) {
        pstverts[i].onseam = LittleLong(pinstverts[i].onseam);
        pstverts[i].s = LittleLong(pinstverts[i].s) << 16; // put s and t in 16.16 format
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
    if (numframes < 1)      Host_SysError("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

    dAliasFrameType_p pframetype = (dAliasFrameType_p)&pintriangles[pmodel->numtris];

    for (int i = 0; i < numframes; i++) {
        AliasFrameType_t frametype = LittleLong(pframetype->type);
        pheader->frames[i].type = frametype;

#   if 1
        switch (frametype) {
        case ALIAS_SINGLE: {
            pframetype = Mod_LoadAliasFrame(
                (dAliasFrameType_p)(pframetype + 1),
                &pheader->frames[i].frame,
                pmodel->numverts,
                &pheader->frames[i].bboxmin,
                &pheader->frames[i].bboxmax,
                pheader, pheader->frames[i].name
            );
        } break;
        case ALIAS_GROUP: {
            pframetype = Mod_LoadAliasGroup(
                (dAliasFrameType_p)(pframetype + 1),
                &pheader->frames[i].frame,
                pmodel->numverts,
                &pheader->frames[i].bboxmin,
                &pheader->frames[i].bboxmax,
                pheader, pheader->frames[i].name
            );
        } break;
        default: { Host_Error("frametype [0x%X] UNKNOWN!\n", frametype); } break;
        }
#   else
        if (frametype == ALIAS_SINGLE) {
            pframetype = Mod_LoadAliasFrame(
                pframetype + 1,
                &pheader->frames[i].frame,
                pmodel->numverts,
                &pheader->frames[i].bboxmin,
                &pheader->frames[i].bboxmax,
                pheader, pheader->frames[i].name
            );
        }
        else {
            pframetype = Mod_LoadAliasGroup(
                pframetype + 1,
                &pheader->frames[i].frame,
                pmodel->numverts,
                &pheader->frames[i].bboxmin,
                &pheader->frames[i].bboxmax,
                pheader, pheader->frames[i].name
            );
        }
#   endif
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
    #endif
}