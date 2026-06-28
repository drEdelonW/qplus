#include "Texture.h"
#include "z_hunk.h"
#ifdef GLQUAKE
// # include "glquake.h"
# include "qOpenGL.h"
#else
# include "r_shared.h"
#endif
#include "client.h"
#include "host.h"

Texture_p r_notexture_mip;

/*
==================
R_InitTextures
==================
*/
void R_InitTextures() {
    // create a simple checkerboard texture for the default
    *(r_notexture_mip = Hunk_AllocName(
        sizeof(Texture_t) + (16 * 16) + (8 * 8) + (4 * 4) + (2 * 2),
        "notexture")
        ) = (Texture_t){
        .width = r_notexture_mip->height = 16,
        .offsets[0] = sizeof(Texture_t),
        .offsets[1] = r_notexture_mip->offsets[0] + (16 * 16),
        .offsets[2] = r_notexture_mip->offsets[1] + (8 * 8),
        .offsets[3] = r_notexture_mip->offsets[2] + (4 * 4),
    };

    for (int m = 0; m < 4; m++) {
        uint8_p dest = (uint8_p)r_notexture_mip + r_notexture_mip->offsets[m];
        for (int y = 0; y < (16 >> m); y++)
            for (int x = 0; x < (16 >> m); x++) {
                *dest++ = ((y < (8 >> m)) ^ (x < (8 >> m))) ?
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

    int reletive = (int)(cl.time * 10) % base->anim_total;

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