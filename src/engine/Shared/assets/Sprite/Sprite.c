#include "Sprite.h"
#include "endian_tools.h"
#include "host.h"
#include "q_tools.h"
#ifdef GLQUAKE
#   include "model.h"
#   include <stdio.h>
#   include "qOpenGL.h"
#else
#   include "d_iface.h"
#endif

/*
=================
Mod_LoadSpriteFrame
=================
*/

// TODO: extract similar logic outside #ifdef
#ifdef GLQUAKE

TypeLess_ptr Mod_LoadSpriteFrame(TypeLess_ptr pin, mSpriteFrame_p* ppframe, int framenum) { //
    dSpriteFrame_p pinframe = (dSpriteFrame_p)pin;

    int width = LittleLong(pinframe->width);
    int height = LittleLong(pinframe->height);
    int size = width * height;

    mSpriteFrame_p pspriteframe = Hunk_AllocName(
        sizeof(mSpriteFrame_t),     //
        Mod_loadName
    );

    Q_memset(pspriteframe, 0, sizeof(mSpriteFrame_t));  //
    *ppframe = pspriteframe;

    pspriteframe->width = width;
    pspriteframe->height = height;
    int origin[2] = {
        LittleLong(pinframe->origin[0]),
        LittleLong(pinframe->origin[1])
    };

    pspriteframe->up = origin[1];
    pspriteframe->down = origin[1] - height;
    pspriteframe->left = origin[0];
    pspriteframe->right = width + origin[0];

    char name[NAME_LENGTH]; snprintf(name, sizeof(name), "%s_%i", loadmodel->name, framenum);               //
    pspriteframe->gl_texturenum = GL_LoadTexture(
        name,
        width, height,
        (uint8_p)(pinframe + 1),
        true, true
    ); //

    return (TypeLess_ptr)((uint8_p)pinframe + sizeof(dSpriteFrame_t) + size);
}
#else

TypeLess_ptr  Mod_LoadSpriteFrame(TypeLess_ptr  pin, mSpriteFrame_p* ppframe) {
    dSpriteFrame_p pinframe = (dSpriteFrame_p)pin;

    int width = LittleLong(pinframe->width);
    int height = LittleLong(pinframe->height);
    int size = width * height;

    mSpriteFrame_p pspriteframe = Hunk_AllocName(
        sizeof(mSpriteFrame_t) + size * r_pixbytes,
        Mod_loadName
    );

    Q_memset(pspriteframe, 0, sizeof(mSpriteFrame_t) + size);
    *ppframe = pspriteframe;

    pspriteframe->width = width;
    pspriteframe->height = height;
    int origin[2] = {
        LittleLong(pinframe->origin[0]),
        LittleLong(pinframe->origin[1])
    };

    pspriteframe->up = origin[1];
    pspriteframe->down = origin[1] - height;
    pspriteframe->left = origin[0];
    pspriteframe->right = width + origin[0];

#if 0
    if (r_pixbytes == 1)    Q_memcpy(&pspriteframe->pixels[0], (uint8_p)(pinframe + 1), size);
    else if (r_pixbytes == 2) {
        uint8_p ppixin = (uint8_p)(pinframe + 1);
        uint16_p ppixout = (uint16_p)&pspriteframe->pixels[0];

        for (int i = 0; i < size; i++)
            ppixout[i] = d_8to16table[ppixin[i]];
    }
    else     Host_SysError("Mod_LoadSpriteFrame: driver set invalid r_pixbytes: %d\n", r_pixbytes);
#else
    switch (r_pixbytes) {
    case 1: { Q_memcpy(&pspriteframe->pixels[0], (uint8_p)(pinframe + 1), size); } break;
    case 2: {
        uint8_p ppixin = (uint8_p)(pinframe + 1);
        uint16_p ppixout = (uint16_p)&pspriteframe->pixels[0];

        for (int i = 0; i < size; i++)
            ppixout[i] = d_8to16table[ppixin[i]];
    } break;
    default:    Host_SysError("Mod_LoadSpriteFrame: driver set invalid r_pixbytes: %d\n", r_pixbytes);
    }
#endif
    return (TypeLess_ptr)((uint8_p)pinframe + sizeof(dSpriteFrame_t) + size);
}
#endif



/*
=================
Mod_LoadSpriteGroup
=================
*/

// TODO: extract similar logic outside #ifdef

#ifdef GLQUAKE
TypeLess_ptr Mod_LoadSpriteGroup(TypeLess_ptr pin, mSpriteFrame_p* ppframe, int framenum) {
#else
TypeLess_ptr Mod_LoadSpriteGroup(TypeLess_ptr pin, mSpriteFrame_p * ppframe) {
#endif
    dSpriteGroup_p pingroup = (dSpriteGroup_p)pin;
    int numframes = LittleLong(pingroup->numframes);

    mSpriteGroup_p pspritegroup = Hunk_AllocName(
        sizeof(mSpriteGroup_t) + (numframes - 1) * sizeof(pspritegroup->frames[0]),
        Mod_loadName
    );

    pspritegroup->numframes = numframes;
    *ppframe = (mSpriteFrame_p)pspritegroup;

    dSpriteInterval_p pin_intervals = (dSpriteInterval_p)(pingroup + 1);

    float_p poutintervals = Hunk_AllocName(numframes * sizeof(float), Mod_loadName);
    pspritegroup->intervals = poutintervals;

    for (int i = 0; i < numframes; i++) {
        *poutintervals = LittleFloat(pin_intervals->interval);
        if (*poutintervals <= 0.0)          Host_SysError("Mod_LoadSpriteGroup: interval<=0");

        poutintervals++;
        pin_intervals++;
    }

    TypeLess_ptr ptemp = (TypeLess_ptr)pin_intervals;

    for (int i = 0; i < numframes; i++) {
#ifdef GLQUAKE
        ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i], framenum * 100 + i);
#else
        ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i]);
#endif
    }

    return ptemp;
}


/*
=================
Mod_LoadSpriteModel
=================
*/

void Mod_LoadSpriteModel(Model_p mod, TypeLess_ptr buffer) {
    dSprite_p pin = (dSprite_p)buffer;

    int version = LittleLong(pin->version);
    if (version != SPRITE_VERSION)
        Host_SysError(
            "%s has wrong version number "
            "(%i should be %i)",
            mod->name, version, SPRITE_VERSION
        );

    int numframes = LittleLong(pin->numframes);

    mSprite_p psprite = Hunk_AllocName(
        sizeof(mSprite_t) + (numframes - 1) * sizeof(psprite->frames),
        Mod_loadName
    );

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
    if (numframes < 1)        Host_SysError("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);

    mod->numframes = numframes;

#ifndef GLQUAKE
    mod->flags = 0;
#endif

    dSpriteFrameType_p pframetype = (dSpriteFrameType_p)(pin + 1);

    for (int i = 0; i < numframes; i++) {
        SpriteFrameType_t frametype;

        frametype = LittleLong(pframetype->type);
        psprite->frames[i].type = frametype;

#ifdef GLQUAKE
        if (frametype == SPR_SINGLE)    pframetype = (dSpriteFrameType_p)Mod_LoadSpriteFrame(pframetype + 1, &psprite->frames[i].frameptr, i);
        else                            pframetype = (dSpriteFrameType_p)Mod_LoadSpriteGroup(pframetype + 1, &psprite->frames[i].frameptr, i);
#else
        if (frametype == SPR_SINGLE)    pframetype = (dSpriteFrameType_p)Mod_LoadSpriteFrame(pframetype + 1, &psprite->frames[i].frameptr);
        else                            pframetype = (dSpriteFrameType_p)Mod_LoadSpriteGroup(pframetype + 1, &psprite->frames[i].frameptr);
#endif
    }
    mod->type = mod_sprite;
}