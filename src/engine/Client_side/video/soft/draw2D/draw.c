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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "draw.h"
#include "screen.h"
#include "vid.h"    // vid.con.pBuff VID_UnlockBuffer()
#include "platformdefs.h"
#include "zone.h"
#include <string.h>
#include "host.h"
#include "common.h"
#include "d_iface.h"
#include "versions.h"
#include "sound.h"
#include "q_tools.h"
#include "wad.h"

qPic_p draw_disc;

typedef struct {
    vRect_t rect;
    int     width;
    int     height;
    qColor8_p pTexBytes;
    int     rowBytes;
} RectDesc_t;
static RectDesc_t   _rRectDesc;
static qColor8_p    _draw_chars;    // 8*8 graphic characters
static qPic_p       _draw_backtile;

//=============================================================================
/* Support Routines */

typedef struct CachePic_s {
    char        name[MAX_QPATH];
    CacheUser_t cache;
} CachePic_t;
typedef CachePic_t* CachePic_p;

#define MAX_CACHED_PICS  128
static CachePic_t   _menu_cachepics[MAX_CACHED_PICS];
static int          _menu_numcachepics = 0;


qPic_p Draw_PicFromWad(cStringRO name) {
    return W_GetLumpName(name);
}

/*
================
Draw_CachePic
================
*/
qPic_p Draw_CachePic(cStringRO path) {
    CachePic_p pic = _menu_cachepics;
    int i = 0;
    for (; i < _menu_numcachepics; pic++, i++)
        if (!strcmp(path, pic->name))
            break;

    if (i == _menu_numcachepics) {
        if (_menu_numcachepics == MAX_CACHED_PICS)
            Host_SysError("_menu_numcachepics == MAX_CACHED_PICS");

        _menu_numcachepics++;
        strcpy(pic->name, path);
    }

    qPic_p dat = Cache_Check(&pic->cache);
    if (dat)
        return dat;

    //
    // load the pic from disk
    //
    COM_LoadCacheFile(path, &pic->cache);

    dat = (qPic_p)pic->cache.data;
    if (!dat)
        Host_SysError("Draw_CachePic: failed to load %s", path);

    SwapPic(dat);

    return dat;
}



/*
===============
Draw_Init
===============
*/
void Draw_Init() {
    _draw_chars = W_GetLumpName("conchars");
    draw_disc = W_GetLumpName("disc");
    _draw_backtile = W_GetLumpName("backtile");

    _rRectDesc.width = _draw_backtile->width;
    _rRectDesc.height = _draw_backtile->height;
    _rRectDesc.pTexBytes = _draw_backtile->data;
    _rRectDesc.rowBytes = _draw_backtile->width;
}



/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character(int x, int y, int num) {
    num &= 0xFF;

    if (y <= -8)    return; // totally off screen

#ifdef PARANOID
    if ((y > Scr.vrect.height - 80) ||
        (x < 0) ||
        (x > Scr.vrect.width - 8))
        Host_SysError("Con_DrawCharacter: (%i, %i)", x, y);
    if ((num < 0) ||
        (num > 255))
        Host_SysError("Con_DrawCharacter: char %i", num);
#endif

    int row = num >> 4;
    int col = num & 15;
    qColor8_p source = _draw_chars + (row << 10) + (col << 3);

    int drawline;
    if (y < 0) { // clipped
        drawline = 8 + y;
        source -= 128 * y;
        y = 0;
    }
    else
        drawline = 8;


    if (r_pixbytes == 1) {
        qColor8_p dest = (qColor8_p)vid.con.pBuff + y * vid.conrowbytes + x;

        while (drawline--) {
            for (int i = 0; i < 8; i++)
                if (source[i].i != InkConTransp)
                    dest[i] = source[i];

            source += 128;
            dest += vid.conrowbytes;
        }
    }
    else {
        // FIXME: pre-expand to native format?
        uint16_p pusdest = (uint16_p) // TODO: rework to Rgb16_p
            ((uint8_p)vid.con.pBuff + y * vid.conrowbytes + (x << 1));

        while (drawline--) {
            for (int i = 0; i < 8; i++)
                if (source[i].i != InkConTransp)
                    pusdest[i] = d_8to16table[source[i].i];

            source += 128;
            pusdest += HALF(vid.conrowbytes);
        }
    }
}

/*
================
Draw_String
================
*/
void Draw_String(int x, int y, cStringRO str) {
    while (*str) {
        Draw_Character(x, y, *str);
        str++;
        x += 8;
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
void Draw_DebugChar(char num) {
    if (!vid.direct)
        return;  // don't have direct FB access, so no debugchars...

    int drawline = 8;
    int row = num >> 4;
    int col = num & 15;
    qColor8_p source = _draw_chars + (row << 10) + (col << 3);

    qColor8_p dest = vid.direct + 312;

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
    if (!Scr.vrect.pBuff)   return;

    if ((x < 0) ||
        (y < 0) ||
        ((x + pic->width) > Scr.vrect.width) ||
        ((y + pic->height) > Scr.vrect.height)
        ) {
        Host_SysError("Draw_Pic: bad coordinates %i/%i-%i/%i", x, y, pic->width, pic->height);
    }

    qColor8_p source = pic->data;

    if (r_pixbytes == 1) {
        qColor8_p dest = Scr.vrect.pClr + y * vid.rowbytes + x;

        for (int v = 0; v < pic->height; v++) {
            Q_memcpy(dest, source, pic->width);
            dest += vid.rowbytes;
            source += pic->width;
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)Scr.vrect.pBuff + y * HALF(vid.rowbytes) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++)
                pusdest[u] = d_8to16table[source[u].i];

            pusdest += HALF(vid.rowbytes);
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
    if (!Scr.vrect.pBuff)   return;

    if (!pic)   return;

    if ((x < 0) ||
        (y < 0) ||
        ((uint32_t)(x + pic->width) > Scr.vrect.width) ||
        ((uint32_t)(y + pic->height) > Scr.vrect.height)
        ) {
        Host_SysError("Draw_TransPic: bad coordinates");
    }
    qColor8_p source = pic->data;

    if (r_pixbytes == 1) {
        qColor8_p dest = Scr.vrect.pClr + y * vid.rowbytes + x;
        qColor8_t tbyte;
        if (pic->width & 7) { // general
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u++)
                    if ((tbyte = source[u]).i != InkTransp)
                        dest[u] = tbyte;

                dest += vid.rowbytes;
                source += pic->width;
            }
        }
        else { // unwound
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u += 8)
                    for (int i = 0; i < 8; i++)
                        if ((tbyte = source[u + i]).i != InkTransp)
                            dest[u + i] = tbyte;

                dest += vid.rowbytes;
                source += pic->width;
            }
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)Scr.vrect.pBuff + y * HALF(vid.rowbytes) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++) {
                uint8_t tbyte = source[u].i;
                if (tbyte != InkTransp)
                    pusdest[u] = d_8to16table[tbyte];
            }

            pusdest += HALF(vid.rowbytes);
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
    if (!Scr.vrect.pBuff)   return;

    if (!pic)
        return;

    if ((x < 0) ||
        (y < 0) ||
        ((uint32_t)(x + pic->width) > Scr.vrect.width) ||
        (uint32_t)(y + pic->height) > Scr.vrect.height
        ) {
        Host_SysError("Draw_TransPic: bad coordinates");
    }

    qColor8_t tbyte;
    qColor8_p source = pic->data;
    if (r_pixbytes == 1) {
        qColor8_p dest = Scr.vrect.pClr + y * vid.rowbytes + x;

        if (pic->width & 7) { // general
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u++)
                    if ((tbyte = source[u]).i != InkTransp)
                        dest[u] = translation->pal[tbyte.i];

                dest += vid.rowbytes;
                source += pic->width;
            }
        }
        else { // unwound
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u += 8)
                    for (int i = 0; i < 8; i++)
                        if ((tbyte = source[u + i]).i != InkTransp)
                            dest[u + i] = translation->pal[tbyte.i];

                dest += vid.rowbytes;
                source += pic->width;
            }
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)Scr.vrect.pBuff + y * HALF(vid.rowbytes) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++) {
                tbyte = source[u];

                if (tbyte.i != InkTransp)
                    pusdest[u] = d_8to16table[tbyte.i];
            }

            pusdest += HALF(vid.rowbytes);
            source += pic->width;
        }
    }
}


void Draw_CharToConback(int num, qColor8_p dest) {
    int row = num >> 4;
    int col = num & 15;
    qColor8_p source = _draw_chars + (row << 10) + (col << 3);

    int drawline = 8;
    while (drawline--) {
        for (int x = 0; x < 8; x++)
            if (source[x].i)
                dest[x].i = 0x60 + source[x].i;
        source += 128;
        dest += 320;
    }

}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines) {
    qPic_p conback = Draw_CachePic("gfx/conback.lmp");

    // hack the version number directly into the pic
    char ver[100];
    qColor8_p dest;
#ifdef _WIN32
    snprintf(ver, sizeof(ver), "(WinQuake) %4.2f", (float)VERSION);
    dest = conback->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
#elif defined(X11)
    snprintf(ver, sizeof(ver), "(X11 Quake %2.2f) %4.2f", (float)X11_VERSION, (float)VERSION);
    dest = conback->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
#elif defined(__linux__)
    snprintf(ver, sizeof(ver), "(Linux Quake %2.2f) %4.2f", (float)LINUX_VERSION, (float)VERSION);
    dest = conback->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
#else
    dest = conback->data + 320 - 43 + 320 * 186;
    snprintf(ver, sizeof(ver), "%4.2f", VERSION);
#endif

    for (int x = 0; x < strlen(ver); x++)
        Draw_CharToConback(ver[x], dest + (x << 3));

    // draw the pic
    if (r_pixbytes == 1) {
        dest = vid.con.pClr;

        for (int y = 0; y < lines; y++, dest += vid.conrowbytes) {
            int v = (vid.con.height - lines + y) * 200 / vid.con.height;
            qColor8_p src = conback->data + v * 320;
            if (vid.con.width == 320)
                memcpy(dest, src, vid.con.width);
            else {
                fixed16_t f = 0;
                fixed16_t fstep = 320 * FIXED16_ONE / vid.con.width;
                for (int x = 0; x < vid.con.width; x += 4) {
                    dest[x + 0] = src[FIXED16_TO_INT(f)];     f += fstep;
                    dest[x + 1] = src[FIXED16_TO_INT(f)];     f += fstep;
                    dest[x + 2] = src[FIXED16_TO_INT(f)];     f += fstep;
                    dest[x + 3] = src[FIXED16_TO_INT(f)];     f += fstep;
                }
            }
        }
    }
    else {
        uint16_p pusdest = (uint16_p)vid.con.pBuff;

        for (int y = 0; y < lines; y++, pusdest += HALF(vid.conrowbytes)) {
            // FIXME: pre-expand to native format?
            // FIXME: does the endian switching go away in production?
            int v = (vid.con.height - lines + y) * 200 / vid.con.height;
            qColor8_p src = conback->data + v * 320;
            fixed16_t f = 0;
            fixed16_t fstep = 320 * FIXED16_ONE / vid.con.width;
            for (int x = 0; x < vid.con.width; x += 4) {
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
    if (!Scr.vrect.pBuff)   return;
    qColor8_p pdest = Scr.vrect.pClr + (prect->y * vid.rowbytes) + prect->x;

    int srcdelta = rowbytes - prect->width;
    int destdelta = vid.rowbytes - prect->width;

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
            pdest += vid.rowbytes;
        }
    }
}


/*
==============
R_DrawRect16
==============
*/
void R_DrawRect16(vRect_p prect, int rowbytes, qColor8_p psrc, bool transparent) {
    if (!Scr.vrect.pBuff)   return;
    // FIXME: would it be better to pre-expand native-format versions?

    uint16_p pdest = (uint16_p)Scr.vrect.pBuff +
        (prect->y * HALF(vid.rowbytes)) + prect->x;

    int srcdelta = rowbytes - prect->width;
    int destdelta = HALF(vid.rowbytes) - prect->width;

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
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h) {
    _rRectDesc.rect = (vRect_t){
        .x = x,
        .y = y,
        .width = w,
        .height = h
    };

    vRect_t vr = { .y = _rRectDesc.rect.y };
    int height = _rRectDesc.rect.height;

    int tileoffsety = vr.y % _rRectDesc.height;

    while (height > 0) {
        vr.x = _rRectDesc.rect.x;
        int width = _rRectDesc.rect.width;

        if (tileoffsety != 0)   vr.height = _rRectDesc.height - tileoffsety;
        else                    vr.height = _rRectDesc.height;

        if (vr.height > height)
            vr.height = height;

        int tileoffsetx = vr.x % _rRectDesc.width;

        while (width > 0) {
            if (tileoffsetx != 0)   vr.width = _rRectDesc.width - tileoffsetx;
            else                    vr.width = _rRectDesc.width;

            if (vr.width > width)
                vr.width = width;

            qColor8_p psrc = _rRectDesc.pTexBytes + (ptrdiff_t)(
                (tileoffsety * _rRectDesc.rowBytes) + tileoffsetx);

            if (r_pixbytes == 1)     R_DrawRect8(&vr, _rRectDesc.rowBytes, psrc, false);
            else                    R_DrawRect16(&vr, _rRectDesc.rowBytes, psrc, false);

            vr.x += vr.width;
            width -= vr.width;
            tileoffsetx = 0; // only the left tile can be left-clipped
        }

        vr.y += vr.height;
        height -= vr.height;
        tileoffsety = 0;  // only the top tile can be top-clipped
    }
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c) {
    if (!Scr.vrect.pBuff)   return;
    int stride = vid.rowbytes;
    if (r_pixbytes == 1) {
        uint8_p dest = Scr.vrect.pBuff + (ptrdiff_t)(
            (y * stride) + x
            );
        for (int v = 0; v < h; v++, dest += stride)
            for (int u = 0; u < w; u++)
                dest[u] = c;
    }
    else {
        stride = HALF(stride);
        uint16_p pusdest = (uint16_p)Scr.vrect.pBuff + (ptrdiff_t)(
            (y * stride) + x
            );
        for (int v = 0; v < h; v++, pusdest += stride)
            for (int u = 0; u < w; u++)
                pusdest[u] = d_8to16table[c];
    }
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen() {
    if (!Scr.vrect.pBuff)   return;
    VID_UnlockBuffer(); S_ExtraUpdate(); VID_LockBuffer();

    for (int y = 0; y < Scr.vrect.height; y++) {
        uint8_p pbuf = (uint8_p)(Scr.vrect.pBuff + vid.rowbytes * y);
        int t = TWICE(y & 1);

        for (int x = 0; x < Scr.vrect.width; x++)
            if ((x & 3) != t)
                pbuf[x] = 0;
    }

    VID_UnlockBuffer(); S_ExtraUpdate(); VID_LockBuffer();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc() {
    D_BeginDirectRect(Scr.vrect.width - 24, 0, draw_disc->data, 24, 24);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc() {
    D_EndDirectRect(Scr.vrect.width - 24, 0, 24, 24);
}

