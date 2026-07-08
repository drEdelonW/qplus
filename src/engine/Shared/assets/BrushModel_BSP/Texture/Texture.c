#include "Texture.h"
#include "z_hunk.h"
#ifdef GLQUAKE
# include "qOpenGL.h"
#else
# include "r_shared.h"
#endif
#include "client.h"
#include "host.h"

Texture_p r_notexture_mip;

typedef enum {
    MipOffset0 = sizeof(Texture_t),
    MipOffset1 = MipOffset0 + (16 * 16),
    MipOffset2 = MipOffset1 + (8 * 8),
    MipOffset3 = MipOffset2 + (4 * 4),
    NoTextureSize = MipOffset3 + (2 * 2),
} MipOffset_t;
/*
==================
R_InitTextures
==================
*/
void R_InitTextures() {
    // create a simple checkerboard texture for the default
    *(r_notexture_mip = Hunk_AllocName(
        NoTextureSize, "notexture"
    )) = (Texture_t){
        .width = 16,
        .height = 16,
        .offsets[Mip0] = MipOffset0,
        .offsets[Mip1] = MipOffset1,
        .offsets[Mip2] = MipOffset2,
        .offsets[Mip3] = MipOffset3,
    };

    for (int m = 0; m < MIPLEVELS; m++) {
        qColor8_p dest = GetMipPtr(r_notexture_mip, m);
        for (int y = 0; y < (16 >> m); y++)
            for (int x = 0; x < (16 >> m); x++) {
                (dest++)->i =
                    ((y < (8 >> m)) ^ (x < (8 >> m))) ?
                    0x00 : 0xFF;
            }
    }
}


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
Texture_p R_TextureAnimation(Texture_p base) {
    if (
        (currententity->frame) &&
        (base->alternate_anims)
        )
        base = base->alternate_anims;

    if (!base->anim_total)      return base;

    int reletive = (int)(GetClSimTime() * 10) % base->anim_total;

    int count = 0;
    while (
        (base->anim_min > reletive) ||
        (base->anim_max <= reletive)
        ) {
        base = base->anim_next;
        if (!base)          Host_SysError("R_TextureAnimation: broken cycle");
        if (++count > 100)  Host_SysError("R_TextureAnimation: infinite cycle");
    }

    return base;
}


#include "endian_tools.h"
#include "q_tools.h"
#include <string.h>
#ifdef GLQUAKE
#   include "qOpenGL.h"
#   include "model.h"
#else
#   include "render.h"
#endif

/*
=================
Mod_LoadTextures
=================
*/
void Mod_LoadTextures(Lump_p Lump_in) {
    if (!Lump_in->fileLen) { _loadModel->textures = NULL; return; }

    dMipTexLump_p m = getMapLumpPtr(mod_base, Lump_in);

    m->nummiptex = LittleLong(m->nummiptex);

    _loadModel->numtextures = m->nummiptex;
    _loadModel->textures = Hunk_AllocName(
        m->nummiptex * sizeof(*_loadModel->textures), Mod_loadName
    );

    for (int i = 0; i < m->nummiptex; i++) {
        m->dataOfs[i] = LittleLong(m->dataOfs[i]);
        if (m->dataOfs[i] == -1)        continue;

        MipTex_p mt = (MipTex_p)((uint8_p)m + m->dataOfs[i]);
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
        Texture_p anims[10] = { 0 };
        Texture_p altanims[10] = { 0 };

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
