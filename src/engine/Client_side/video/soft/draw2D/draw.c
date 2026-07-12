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

// draw.c -- this is the only file outside the refresh that touches the vid buffer

#include "draw.h"
#include "screen.h"
#include <string.h>
#include <stdio.h>   // snprintf
#include "host.h"
#include "q_tools.h"


typedef struct {
    vRect_t rect;
    int w; // width;
    int h; //height;
    qColor8_p pData;
    int rowBytes;
} BackTile_t;
static BackTile_t _btR;

/*
===============
Draw_Init
===============
*/
#include "qSymbolChar.h"

void Draw_Init() {
    pDrawChars = (qColor8_p)GetPicFromWad("conchars");
    hid.disc = GetPicFromWad("disc");

    qPic_p BackTile = GetPicFromWad("backtile"); // get from WAD textures
    _btR = (BackTile_t){
        .w = BackTile->width,
        .h = BackTile->height,
        .pData = BackTile->data,
        .rowBytes = BackTile->width,
    };
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void R_DrawRect8(vRect_p prect, int rowbytes, qColor8_p psrc, bool transparent);
void R_DrawRect16(vRect_p prect, int rowbytes, qColor8_p psrc, bool transparent);
void Draw_TileClear(int x, int y, int w, int h) {
    _btR.rect = (vRect_t){
        .x = x,     .y = y,
        .width = w, .height = h
    };

    vRect_t vr = {
        .y = _btR.rect.y
    };

    int tileOffsY = vr.y % _btR.h;
    {
        int height = _btR.rect.height;
        while (height > 0) {
            vr.x = _btR.rect.x;
            vr.height = _btR.h - tileOffsY;
            ClampMoreThen(&vr.height, height);
            int tileOffsX = vr.x % _btR.w;
            {
                int width = _btR.rect.width;
                while (width > 0) {
                    vr.width = _btR.w - tileOffsX;
                    ClampMoreThen(&vr.width, width);
                    qColor8_p pSrc = _btR.pData + (tileOffsY * _btR.rowBytes) + tileOffsX;

                    if (r_pixbytes == 1)     R_DrawRect8(&vr, _btR.rowBytes, pSrc, false);
                    else                    R_DrawRect16(&vr, _btR.rowBytes, pSrc, false);

                    vr.x += vr.width;
                    width -= vr.width;
                    tileOffsX = 0; // only the left tile can be left-clipped
                }
            }
            vr.y += vr.height;
            height -= vr.height;
            tileOffsY = 0;  // only the top tile can be top-clipped
        }
    }
}


//=============================================================================
/* Support Routines */

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character(int x, int y, ConsoleSymbols_t symb) {
    if (y <= -8)    return; // totally off screen

#ifdef PARANOID
    if ((y > Scr.canvas.height - 80) ||
        (x < 0) ||
        (x > Scr.canvas.width - 8)
        )   Host_SysError("Con_DrawCharacter: (%i, %i)", x, y);
    if ((num < 0) ||
        (num > 255)
        )   Host_SysError("Con_DrawCharacter: char %i", num);
#endif

    qColor8_p source = CharGlyphSource(symb);
    int drawline;
    if (y < 0) { // clipped
        drawline = 8 + y;
        source -= 128 * y;
        y = 0;
    }
    else
        drawline = 8;


    if (r_pixbytes == 1) {
        qColor8_p dest = Scr.con.pClr + (y * Scr.SR_rowBytes) + x;

        while (drawline--) {
            for (int i = 0; i < 8; i++)
                if (source[i].i != InkConTransp)
                    dest[i] = source[i];

            source += 128;
            dest += Scr.SR_rowBytes;
        }
    }
    else {
        // FIXME: pre-expand to native format?
        ptrdiff_t row16Byte = DIV2(Scr.SR_rowBytes);
        Rgb16_p pusdest = (Rgb16_p)Scr.con.pClr + (y * row16Byte) + x;

        while (drawline--) {
            for (int i = 0; i < 8; i++)
                if (source[i].i != InkConTransp)
                    pusdest[i] = d_8to16table[source[i].i];

            source += 128;
            pusdest += row16Byte;
        }
    }
}


/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/

void Draw_DebugChar(ConsoleSymbols_t symb) {
    if (!Scr.direct)
        return;  // don't have direct FB access, so no debugchars...

    qColor8_p source = CharGlyphSource(symb);
    qColor8_p dest = Scr.direct + 312;

    int drawline = 8;
    while (drawline--) {
        for (int i = 0; i < 8; i++)
            dest[i] = source[i];

        source += 128;
        dest += 320;
    }
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, qPic_p pic) {
    if (!Scr.canvas.pClr)   return;

    if ((x < 0) ||
        (y < 0) ||
        ((x + pic->width) > Scr.canvas.width) ||
        ((y + pic->height) > Scr.canvas.height)
        )   Host_SysError(
            "Draw_Pic: bad coordinates %i/%i-%i/%i",
            x, y, pic->width, pic->height
        );


    qColor8_p source = pic->data;

    if (r_pixbytes == 1) {
        qColor8_p dest = Scr.canvas.pClr + y * Scr.SR_rowBytes + x;

        for (int v = 0; v < pic->height; v++) {
            Q_memcpy(dest, source, pic->width);
            dest += Scr.SR_rowBytes;
            source += pic->width;
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)Scr.canvas.pClr + y * HALF(Scr.SR_rowBytes) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++)
                pusdest[u] = d_8to16table[source[u].i];

            pusdest += HALF(Scr.SR_rowBytes);
            source += pic->width;
        }
    }
}





/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic(int x, int y, qPic_p pic) {
    if (!Scr.canvas.pClr)   return;

    if (!pic)   return;

    if ((x < 0) ||
        (y < 0) ||
        ((uint32_t)(x + pic->width) > Scr.canvas.width) ||
        ((uint32_t)(y + pic->height) > Scr.canvas.height)
        ) {
        Host_SysError("Draw_TransPic: bad coordinates");
    }
    qColor8_p source = pic->data;

    if (r_pixbytes == 1) {
        qColor8_p dest = Scr.canvas.pClr + y * Scr.SR_rowBytes + x;
        qColor8_t tbyte;
        if (pic->width & 7) { // general
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u++)
                    if ((tbyte = source[u]).i != InkTransp)
                        dest[u] = tbyte;

                dest += Scr.SR_rowBytes;
                source += pic->width;
            }
        }
        else { // unwound
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u += 8)
                    for (int i = 0; i < 8; i++)
                        if ((tbyte = source[u + i]).i != InkTransp)
                            dest[u + i] = tbyte;

                dest += Scr.SR_rowBytes;
                source += pic->width;
            }
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)Scr.canvas.pClr + y * HALF(Scr.SR_rowBytes) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++) {
                uint8_t tbyte = source[u].i;
                if (tbyte != InkTransp)
                    pusdest[u] = d_8to16table[tbyte];
            }

            pusdest += HALF(Scr.SR_rowBytes);
            source += pic->width;
        }
    }
}


/*
=============
Draw_TransPicTranslate
=============
*/
void Draw_TransPicTranslate(int x, int y, qPic_p pic, palMap_p translation) {
    if (!Scr.canvas.pClr)   return;

    if (!pic)
        return;

    if ((x < 0) ||
        (y < 0) ||
        ((uint32_t)(x + pic->width) > Scr.canvas.width) ||
        (uint32_t)(y + pic->height) > Scr.canvas.height
        ) {
        Host_SysError("Draw_TransPic: bad coordinates");
    }

    qColor8_t tbyte;
    qColor8_p source = pic->data;
    if (r_pixbytes == 1) {
        qColor8_p dest = Scr.canvas.pClr + (y * Scr.SR_rowBytes) + x;

        if (pic->width & 7) { // general
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u++)
                    if ((tbyte = source[u]).i != InkTransp)
                        dest[u] = translation->pal[tbyte.i];

                dest += Scr.SR_rowBytes;
                source += pic->width;
            }
        }
        else { // unwound
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u += 8)
                    for (int i = 0; i < 8; i++)
                        if ((tbyte = source[u + i]).i != InkTransp)
                            dest[u + i] = translation->pal[tbyte.i];

                dest += Scr.SR_rowBytes;
                source += pic->width;
            }
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)Scr.canvas.pClr + y * HALF(Scr.SR_rowBytes) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++) {
                tbyte = source[u];
                if (tbyte.i != InkTransp)
                    pusdest[u] = d_8to16table[tbyte.i];
            }

            pusdest += HALF(Scr.SR_rowBytes);
            source += pic->width;
        }
    }
}




/*
================
Draw_ConsoleBackground

================
*/
#include "versions.h"
void Draw_ConsoleBackground(int lines) {
    qPic_p conback = Draw_CachePic("gfx/conback.lmp");

    // hack the version number directly into the pic
    char ver[100];
    qColor8_p dest = conback->data + ((320 * 186) + 320) - 11;
#ifdef _WIN32
    snprintf(ver, sizeof(ver), "(WinQuake) %4.2f", (float)VERSION);
    dest += -(MUL8(strlen(ver)));
#elif defined(X11)
    snprintf(ver, sizeof(ver), "(X11 Quake %2.2f) %4.2f", (float)X11_VERSION, (float)VERSION);
    dest += -(MUL8(strlen(ver)));
#elif defined(__linux__)
    snprintf(ver, sizeof(ver), "(Linux Quake %2.2f) %4.2f", (float)LINUX_VERSION, (float)VERSION);
    dest += -(MUL8(strlen(ver)));
#else
    dest += -MUL8(4);
    snprintf(ver, sizeof(ver), "%4.2f", VERSION);
#endif

    for (int x = 0; x < strlen(ver); x++)
        Draw_CharToConback(ver[x], dest + (x << 3));

    // draw the pic
    if (r_pixbytes == 1) {
        dest = Scr.con.pClr;

        for (int y = 0; y < lines; y++, dest += Scr.SR_rowBytes) {
            int v = (Scr.con.height - lines + y) * 200 / Scr.con.height;
            qColor8_p src = conback->data + v * 320;
            if (Scr.con.width == 320)
                memcpy(dest, src, Scr.con.width);
            else {
                fixed16_t f = 0;
                fixed16_t fstep = 320 * FIXED16_ONE / Scr.con.width;
                for (int x = 0; x < Scr.con.width; x += 4) {
                    dest[x + 0] = src[FIXED16_TO_INT(f)];     f += fstep;
                    dest[x + 1] = src[FIXED16_TO_INT(f)];     f += fstep;
                    dest[x + 2] = src[FIXED16_TO_INT(f)];     f += fstep;
                    dest[x + 3] = src[FIXED16_TO_INT(f)];     f += fstep;
                }
            }
        }
    }
    else {
        uint16_p pusdest = (uint16_p)Scr.con.pClr;

        for (int y = 0; y < lines; y++, pusdest += HALF(Scr.SR_rowBytes)) {
            // FIXME: pre-expand to native format?
            // FIXME: does the endian switching go away in production?
            int v = (Scr.con.height - lines + y) * 200 / Scr.con.height;
            qColor8_p src = conback->data + v * 320;
            fixed16_t f = 0;
            fixed16_t fstep = 320 * FIXED16_ONE / Scr.con.width;
            for (int x = 0; x < Scr.con.width; x += 4) {
                pusdest[x + 0] = d_8to16table[src[FIXED16_TO_INT(f)].i];  f += fstep;
                pusdest[x + 1] = d_8to16table[src[FIXED16_TO_INT(f)].i];  f += fstep;
                pusdest[x + 2] = d_8to16table[src[FIXED16_TO_INT(f)].i];  f += fstep;
                pusdest[x + 3] = d_8to16table[src[FIXED16_TO_INT(f)].i];  f += fstep;
            }
        }
    }
}


/*
==============
R_DrawRect8
==============
*/
void R_DrawRect8(vRect_p prect, int rowbytes, qColor8_p psrc, bool transparent) {
    if (!Scr.canvas.pClr)   return;
    qColor8_p pdest = Scr.canvas.pClr + (prect->y * Scr.SR_rowBytes) + prect->x;

    int srcdelta = rowbytes - prect->width;
    int destdelta = Scr.SR_rowBytes - prect->width;

    if (transparent) {
        for (int i = 0; i < prect->height; i++) {
            for (int j = 0; j < prect->width; j++) {
                qColor8_t t = *psrc;
                if (t.i != InkTransp)
                    *pdest = t;

                psrc++;
                pdest++;
            }

            psrc += srcdelta;
            pdest += destdelta;
        }
    }
    else {
        for (int i = 0; i < prect->height; i++) {
            memcpy(pdest, psrc, prect->width);
            psrc += rowbytes;
            pdest += Scr.SR_rowBytes;
        }
    }
}


/*
==============
R_DrawRect16
==============
*/
void R_DrawRect16(vRect_p prect, int rowbytes, qColor8_p psrc, bool transparent) {
    if (!Scr.canvas.pClr)   return;
    // FIXME: would it be better to pre-expand native-format versions?

    uint16_p pdest = (uint16_p)Scr.canvas.pClr +
        (prect->y * HALF(Scr.SR_rowBytes)) + prect->x;

    int srcdelta = rowbytes - prect->width;
    int destdelta = HALF(Scr.SR_rowBytes) - prect->width;

    qColor8_t t;
    if (transparent) {
        for (int i = 0; i < prect->height; i++) {
            for (int j = 0; j < prect->width; j++) {
                t = *psrc;
                if (t.i != InkTransp)
                    *pdest = d_8to16table[t.i];

                psrc++;
                pdest++;
            }

            psrc += srcdelta;
            pdest += destdelta;
        }
    }
    else {
        for (int i = 0; i < prect->height; i++) {
            for (int j = 0; j < prect->width; j++) {
                *pdest = d_8to16table[(*psrc).i];
                psrc++;
                pdest++;
            }

            psrc += srcdelta;
            pdest += destdelta;
        }
    }
}



/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, qColor8_t c) {
    if (!Scr.canvas.pClr)   return;
    int stride = Scr.SR_rowBytes;
    if (r_pixbytes == 1) {
        qColor8_p dest = Scr.canvas.pClr + (ptrdiff_t)(
            (y * stride) + x
            );
        for (int v = 0; v < h; v++, dest += stride)
            for (int u = 0; u < w; u++)
                dest[u] = c;
    }
    else {
        stride = HALF(stride);
        uint16_p pusdest = (uint16_p)Scr.canvas.pClr + (ptrdiff_t)(
            (y * stride) + x
            );
        for (int v = 0; v < h; v++, pusdest += stride)
            for (int u = 0; u < w; u++)
                pusdest[u] = d_8to16table[c.i];
    }
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
#include "sound.h"  // S_ExtraUpdateBUL
void Draw_FadeScreen() {
    if (!Scr.canvas.pClr)   return;
    S_ExtraUpdateBUL();

    for (int y = 0; y < Scr.canvas.height; y++) {
        uint8_p pbuf = (uint8_p)(Scr.canvas.pClr + Scr.SR_rowBytes * y);
        int t = TWICE(y & 1);

        for (int x = 0; x < Scr.canvas.width; x++)
            if ((x & 3) != t)
                pbuf[x] = 0;
    }

    S_ExtraUpdateBUL();
}

//=============================================================================


