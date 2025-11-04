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
#include "vid.h"
#include "platformdefs.h"
#include "zone.h"
#include <string.h>
#include "sys.h"
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
    uint8_p pTexBytes;
    int     rowBytes;
} RectDesc_t;
static RectDesc_t   _rRectDesc;
static uint8_p      _draw_chars;    // 8*8 graphic characters
static qPic_p       _draw_backtile;

//=============================================================================
/* Support Routines */

typedef struct CachePic_s {
    char  name[MAX_QPATH];
    CacheUser_t cache;
} CachePic_t;
typedef CachePic_t* CachePic_p;

#define MAX_CACHED_PICS  128
static CachePic_t _menu_cachepics[MAX_CACHED_PICS];
static int _menu_numcachepics;


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
            Sys_Error("_menu_numcachepics == MAX_CACHED_PICS");
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
    if (!dat) {
        Sys_Error("Draw_CachePic: failed to load %s", path);
    }

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
    num &= 255;

    if (y <= -8)
        return;   // totally off screen

#ifdef PARANOID
    if ((y > vid.height - 80) ||
        (x < 0) ||
        (x > vid.width - 8))
        Sys_Error("Con_DrawCharacter: (%i, %i)", x, y);
    if ((num < 0) ||
        (num > 255))
        Sys_Error("Con_DrawCharacter: char %i", num);
#endif

    int row = num >> 4;
    int col = num & 15;
    uint8_p source = _draw_chars + (row << 10) + (col << 3);

    int drawline;
    if (y < 0) { // clipped
        drawline = 8 + y;
        source -= 128 * y;
        y = 0;
    }
    else
        drawline = 8;


    if (r_pixbytes == 1) {
        uint8_p dest = vid.conbuffer + y * vid.conrowbytes + x;

        while (drawline--) {
#if 0
            for (int i = 0; i < 8; i++)
                if (source[i])
                    dest[i] = source[i];
#else
            if (source[0])
                dest[0] = source[0];
            if (source[1])
                dest[1] = source[1];
            if (source[2])
                dest[2] = source[2];
            if (source[3])
                dest[3] = source[3];
            if (source[4])
                dest[4] = source[4];
            if (source[5])
                dest[5] = source[5];
            if (source[6])
                dest[6] = source[6];
            if (source[7])
                dest[7] = source[7];
#endif
            source += 128;
            dest += vid.conrowbytes;
        }
    }
    else {
        // FIXME: pre-expand to native format?
        uint16_p pusdest = (uint16_p)
            ((uint8_p)vid.conbuffer + y * vid.conrowbytes + (x << 1));

        while (drawline--) {
#if 0
            for (int i = 0; i < 8; i++)
                if (source[i])
                    pusdest[i] = d_8to16table[source[i]];
#else
            if (source[0])
                pusdest[0] = d_8to16table[source[0]];
            if (source[1])
                pusdest[1] = d_8to16table[source[1]];
            if (source[2])
                pusdest[2] = d_8to16table[source[2]];
            if (source[3])
                pusdest[3] = d_8to16table[source[3]];
            if (source[4])
                pusdest[4] = d_8to16table[source[4]];
            if (source[5])
                pusdest[5] = d_8to16table[source[5]];
            if (source[6])
                pusdest[6] = d_8to16table[source[6]];
            if (source[7])
                pusdest[7] = d_8to16table[source[7]];
#endif
            source += 128;
            pusdest += (vid.conrowbytes >> 1);
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
    uint8_p source = _draw_chars + (row << 10) + (col << 3);

    uint8_p dest = vid.direct + 312;

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
    if ((x < 0) ||
        (y < 0) ||
        ((x + pic->width) > vid.width) ||
        ((y + pic->height) > vid.height)
        ) {
        Sys_Error("Draw_Pic: bad coordinates %i/%i-%i/%i", x, y, pic->width, pic->height);
    }

    uint8_p source = pic->data;

    if (r_pixbytes == 1) {
        uint8_p dest = vid.buffer + y * vid.rowbytes + x;

        for (int v = 0; v < pic->height; v++) {
            Q_memcpy(dest, source, pic->width);
            dest += vid.rowbytes;
            source += pic->width;
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)vid.buffer + y * (vid.rowbytes >> 1) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++) {
                pusdest[u] = d_8to16table[source[u]];
            }

            pusdest += vid.rowbytes >> 1;
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
    if (!pic)   return;

    if ((x < 0) ||
        (y < 0) ||
        ((uint32_t)(x + pic->width) > vid.width) ||
        ((uint32_t)(y + pic->height) > vid.height)
        ) {
        Sys_Error("Draw_TransPic: bad coordinates");
    }
    uint8_p source = pic->data;

    if (r_pixbytes == 1) {
        uint8_p dest = vid.buffer + y * vid.rowbytes + x;
        uint8_t tbyte;
        if (pic->width & 7) { // general
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u++)
                    if ((tbyte = source[u]) != TRANSPARENT_COLOR)
                        dest[u] = tbyte;

                dest += vid.rowbytes;
                source += pic->width;
            }
        }
        else { // unwound
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u += 8) {
                    for (int i = 0; i < 8; i++)
                        if ((tbyte = source[u + i]) != TRANSPARENT_COLOR)
                            dest[u + i] = tbyte;

                }
                dest += vid.rowbytes;
                source += pic->width;
            }
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)vid.buffer + y * (vid.rowbytes >> 1) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++) {
                uint8_t tbyte = source[u];
                if (tbyte != TRANSPARENT_COLOR) {
                    pusdest[u] = d_8to16table[tbyte];
                }
            }

            pusdest += vid.rowbytes >> 1;
            source += pic->width;
        }
    }
}


/*
=============
Draw_TransPicTranslate
=============
*/
void Draw_TransPicTranslate(int x, int y, qPic_p pic, uint8_p translation) {
    uint8_t tbyte;
    if (!pic)
        return;

    if ((x < 0) ||
        (y < 0) ||
        ((uint32_t)(x + pic->width) > vid.width) ||
        (uint32_t)(y + pic->height) > vid.height
        ) {
        Sys_Error("Draw_TransPic: bad coordinates");
    }

    uint8_p source = pic->data;
    if (r_pixbytes == 1) {
        uint8_p dest = vid.buffer + y * vid.rowbytes + x;

        if (pic->width & 7) { // general
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u++)
                    if ((tbyte = source[u]) != TRANSPARENT_COLOR)       dest[u] = translation[tbyte];

                dest += vid.rowbytes;
                source += pic->width;
            }
        }
        else { // unwound
            for (int v = 0; v < pic->height; v++) {
                for (int u = 0; u < pic->width; u += 8)
                    for (int i = 0; i < 8; i++)
                        if ((tbyte = source[u + i]) != TRANSPARENT_COLOR)
                            dest[u + i] = translation[tbyte];

                dest += vid.rowbytes;
                source += pic->width;
            }
        }
    }
    else {
        // FIXME: pretranslate at load time?
        uint16_p pusdest = (uint16_p)vid.buffer + y * (vid.rowbytes >> 1) + x;

        for (int v = 0; v < pic->height; v++) {
            for (int u = 0; u < pic->width; u++) {
                tbyte = source[u];

                if (tbyte != TRANSPARENT_COLOR) {
                    pusdest[u] = d_8to16table[tbyte];
                }
            }

            pusdest += vid.rowbytes >> 1;
            source += pic->width;
        }
    }
}


void Draw_CharToConback(int num, uint8_p dest) {
    int row = num >> 4;
    int col = num & 15;
    uint8_p source = _draw_chars + (row << 10) + (col << 3);

    int drawline = 8;
    while (drawline--) {
        for (int x = 0; x < 8; x++)
            if (source[x])
                dest[x] = 0x60 + source[x];
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
#ifdef _WIN32
    sprintf(ver, "(WinQuake) %4.2f", (float)VERSION);
    uint8_p dest = conback->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
#elif defined(X11)
    sprintf(ver, "(X11 Quake %2.2f) %4.2f", (float)X11_VERSION, (float)VERSION);
    uint8_p dest = conback->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
#elif defined(__linux__)
    sprintf(ver, "(Linux Quake %2.2f) %4.2f", (float)LINUX_VERSION, (float)VERSION);
    uint8_p dest = conback->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
#else
    uint8_p dest = conback->data + 320 - 43 + 320 * 186;
    sprintf(ver, "%4.2f", VERSION);
#endif

    for (int x = 0; x < strlen(ver); x++)
        Draw_CharToConback(ver[x], dest + (x << 3));

    // draw the pic
    if (r_pixbytes == 1) {
        dest = vid.conbuffer;

        for (int y = 0; y < lines; y++, dest += vid.conrowbytes) {
            int v = (vid.conheight - lines + y) * 200 / vid.conheight;
            uint8_p src = conback->data + v * 320;
            if (vid.conwidth == 320)
                memcpy(dest, src, vid.conwidth);
            else {
                int f = 0;
                int fstep = 320 * 0x10000 / vid.conwidth;
                for (int x = 0; x < vid.conwidth; x += 4) {
                    dest[x] = src[f >> 16];
                    f += fstep;
                    dest[x + 1] = src[f >> 16];
                    f += fstep;
                    dest[x + 2] = src[f >> 16];
                    f += fstep;
                    dest[x + 3] = src[f >> 16];
                    f += fstep;
                }
            }
        }
    }
    else {
        uint16_p pusdest = (uint16_p)vid.conbuffer;

        for (int y = 0; y < lines; y++, pusdest += (vid.conrowbytes >> 1)) {
            // FIXME: pre-expand to native format?
            // FIXME: does the endian switching go away in production?
            int v = (vid.conheight - lines + y) * 200 / vid.conheight;
            uint8_p src = conback->data + v * 320;
            int f = 0;
            int fstep = 320 * 0x10000 / vid.conwidth;
            for (int x = 0; x < vid.conwidth; x += 4) {
                pusdest[x] = d_8to16table[src[f >> 16]];
                f += fstep;
                pusdest[x + 1] = d_8to16table[src[f >> 16]];
                f += fstep;
                pusdest[x + 2] = d_8to16table[src[f >> 16]];
                f += fstep;
                pusdest[x + 3] = d_8to16table[src[f >> 16]];
                f += fstep;
            }
        }
    }
}


/*
==============
R_DrawRect8
==============
*/
void R_DrawRect8(vRect_p prect, int rowbytes, uint8_p psrc, int transparent) {
    uint8_p pdest = vid.buffer + (prect->y * vid.rowbytes) + prect->x;

    int srcdelta = rowbytes - prect->width;
    int destdelta = vid.rowbytes - prect->width;

    if (transparent) {
        for (int i = 0; i < prect->height; i++) {
            for (int j = 0; j < prect->width; j++) {
                uint8_t t = *psrc;
                if (t != TRANSPARENT_COLOR)
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
void R_DrawRect16(vRect_p prect, int rowbytes, uint8_p psrc, int transparent) {
    // FIXME: would it be better to pre-expand native-format versions?

    uint16_p pdest = (uint16_p)vid.buffer +
        (prect->y * (vid.rowbytes >> 1)) + prect->x;

    int srcdelta = rowbytes - prect->width;
    int destdelta = (vid.rowbytes >> 1) - prect->width;

    uint8_t t;
    if (transparent) {
        for (int i = 0; i < prect->height; i++) {
            for (int j = 0; j < prect->width; j++) {
                t = *psrc;
                if (t != TRANSPARENT_COLOR)
                    *pdest = d_8to16table[t];


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
                *pdest = d_8to16table[*psrc];
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

    _rRectDesc.rect.x = x;
    _rRectDesc.rect.y = y;
    _rRectDesc.rect.width = w;
    _rRectDesc.rect.height = h;

    vRect_t vr = { .y = _rRectDesc.rect.y };
    int height = _rRectDesc.rect.height;

    int tileoffsety = vr.y % _rRectDesc.height;

    while (height > 0) {
        vr.x = _rRectDesc.rect.x;
        int width = _rRectDesc.rect.width;

        if (tileoffsety != 0)
            vr.height = _rRectDesc.height - tileoffsety;
        else
            vr.height = _rRectDesc.height;

        if (vr.height > height)
            vr.height = height;

        int tileoffsetx = vr.x % _rRectDesc.width;

        while (width > 0) {
            if (tileoffsetx != 0)
                vr.width = _rRectDesc.width - tileoffsetx;
            else
                vr.width = _rRectDesc.width;

            if (vr.width > width)
                vr.width = width;

            uint8_p psrc = _rRectDesc.pTexBytes +
                (tileoffsety * _rRectDesc.rowBytes) + tileoffsetx;

            if (r_pixbytes == 1) {
                R_DrawRect8(&vr, _rRectDesc.rowBytes, psrc, 0);
            }
            else {
                R_DrawRect16(&vr, _rRectDesc.rowBytes, psrc, 0);
            }

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
    if (r_pixbytes == 1) {
        uint8_p dest = vid.buffer + y * vid.rowbytes + x;
        for (int v = 0; v < h; v++, dest += vid.rowbytes)
            for (int u = 0; u < w; u++)
                dest[u] = c;
    }
    else {
        uint32_t uc = d_8to16table[c];
        uint16_p pusdest = (uint16_p)vid.buffer + y * (vid.rowbytes >> 1) + x;
        for (int v = 0; v < h; v++, pusdest += (vid.rowbytes >> 1))
            for (int u = 0; u < w; u++)
                pusdest[u] = uc;
    }
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen() {
    VID_UnlockBuffer();
    S_ExtraUpdate();
    VID_LockBuffer();

    for (int y = 0; y < vid.height; y++) {
        uint8_p pbuf = (uint8_p)(vid.buffer + vid.rowbytes * y);
        int t = (y & 1) << 1;

        for (int x = 0; x < vid.width; x++) {
            if ((x & 3) != t)
                pbuf[x] = 0;
        }
    }

    VID_UnlockBuffer();
    S_ExtraUpdate();
    VID_LockBuffer();
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
    D_BeginDirectRect(vid.width - 24, 0, draw_disc->data, 24, 24);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc() {
    D_EndDirectRect(vid.width - 24, 0, 24, 24);
}

