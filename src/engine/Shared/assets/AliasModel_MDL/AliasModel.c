#include "Alias.h"
#include "AliasModel.h"

#ifdef GLQUAKE
#   include "qOpenGL.h"
#else
#   include "d_iface.h"
#   include "r_local.h"
#endif
#include "endian_tools.h"
#include "host.h"
#include "z_hunk.h"
#include "q_tools.h"
#include <string.h>

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
