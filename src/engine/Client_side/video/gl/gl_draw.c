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
#include "draw.h"
#include "qOpenGL.h"
#include "versions.h"
#include "cvar.h"
#include "platformdefs.h"
#include "console.h"
#include "host.h"
#include "wad.h"
#include "common.h"
#include "cmd.h"
#include "q_tools.h"
#include <string.h>
#include "sbar.h"
#include "z_hunk.h"


// draw.c -- this is the only file outside the refresh that touches the
// vid buffer


#define GL_COLOR_INDEX8_EXT     0x80E5


cvar_t  gl_nobind = { "gl_nobind", "0" };
cvar_t  gl_max_size = { "gl_max_size", "1024" };
cvar_t  gl_picmip = { "gl_picmip", "0" };

uint8_p _drawChars;    // 8*8 graphic characters
qPic_p draw_disc;
qPic_p draw_backtile;

int   translate_texture;
int   char_texture;

typedef struct {
    int  texnum;
    float sl, tl, sh, th;
} glpic_t;
typedef glpic_t* glpic_p;

byte  conback_buffer[sizeof(qPic_t) + sizeof(glpic_t)];
qPic_p conback = (qPic_p)&conback_buffer;

int  gl_lightmap_format = 4;
int  gl_solid_format = 3;
int  gl_alpha_format = 4;

int  gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int  gl_filter_max = GL_LINEAR;


int  texels;

typedef struct {
    int  texnum;
    char identifier[NAME_LENGTH];
    int  width, height;
    bool mipmap;
} glTexture_t;
typedef glTexture_t* glTexture_p;

#define MAX_GLTEXTURES 1024
glTexture_t gltextures[MAX_GLTEXTURES];
int   numgltextures;


void GL_Bind(int texnum) {
    if (gl_nobind.value)
        texnum = char_texture;
    if (currenttexture == texnum)
        return;
    currenttexture = texnum;
#ifdef _WIN32
    bindTexFunc(GL_TEXTURE_2D, texnum);
#else
    glBindTexture(GL_TEXTURE_2D, texnum);
#endif
}


/*
=============================================================================

scrap allocation

Allocate all the little status bar obejcts into a single texture
to crutch up stupid hardware / drivers

=============================================================================
*/

#define MAX_SCRAPS  2
#define BLOCK_WIDTH  256
#define BLOCK_HEIGHT 256

int   scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte  scrap_texels[MAX_SCRAPS][BLOCK_WIDTH * BLOCK_HEIGHT * 4];
bool scrap_dirty;
int   scrap_texnum;

// returns a texture number and the position inside it
int Scrap_AllocBlock(int w, int h, int* x, int* y) {
    // int  i, j;
    // int  best, best2;
    // int  bestx;
    // int  texnum;

    for (int texnum = 0; texnum < MAX_SCRAPS; texnum++) {
        int best = BLOCK_HEIGHT;

        for (int i = 0; i < BLOCK_WIDTH - w; i++) {
            int best2 = 0;

            int j;
            for (j = 0; j < w; j++) {
                if (scrap_allocated[texnum][i + j] >= best)
                    break;
                if (scrap_allocated[texnum][i + j] > best2)
                    best2 = scrap_allocated[texnum][i + j];
            }
            if (j == w) { // this is a valid spot
                *x = i;
                *y = best = best2;
            }
        }

        if (best + h > BLOCK_HEIGHT)
            continue;

        for (int i = 0; i < w; i++)
            scrap_allocated[texnum][*x + i] = best + h;

        return texnum;
    }

    Host_SysError("Scrap_AllocBlock: full");
}

int scrap_uploads;
void GL_Upload8(uint8_p data, int width, int height, bool mipmap, bool alpha);
void Scrap_Upload(void) {
    int  texnum;

    scrap_uploads++;

    for (texnum = 0; texnum < MAX_SCRAPS; texnum++) {
        GL_Bind(scrap_texnum + texnum);
        GL_Upload8(scrap_texels[texnum], BLOCK_WIDTH, BLOCK_HEIGHT, false, true);
    }
    scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s {
    char    name[MAX_QPATH];
    qPic_t  pic;
    byte    padding[32]; // for appended glpic
} cachepic_t;
typedef cachepic_t* cachepic_p;

#define MAX_CACHED_PICS  128
cachepic_t menu_cachepics[MAX_CACHED_PICS];
int   menu_numcachepics;

byte  menuplyr_pixels[4096];

int  pic_texels;
int  pic_count;

int GL_LoadPicTexture(qPic_p pic);

qPic_p Draw_PicFromWad(cStringRO name) {
    qPic_p p;
    glpic_p gl;

    p = W_GetLumpName(name);
    gl = (glpic_p)p->data;

    // load little ones into the scrap
    if (p->width < 64 && p->height < 64) {
        int  x, y;
        int  i, j, k;
        int  texnum;

        texnum = Scrap_AllocBlock(p->width, p->height, &x, &y);
        scrap_dirty = true;
        k = 0;
        for (i = 0; i < p->height; i++)
            for (j = 0; j < p->width; j++, k++)
                scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = p->data[k];
        texnum += scrap_texnum;
        gl->texnum = texnum;
        gl->sl = (x + 0.01) / (float)BLOCK_WIDTH;
        gl->sh = (x + p->width - 0.01) / (float)BLOCK_WIDTH;
        gl->tl = (y + 0.01) / (float)BLOCK_WIDTH;
        gl->th = (y + p->height - 0.01) / (float)BLOCK_WIDTH;

        pic_count++;
        pic_texels += p->width * p->height;
    }
    else {
        gl->texnum = GL_LoadPicTexture(p);
        gl->sl = 0;
        gl->sh = 1;
        gl->tl = 0;
        gl->th = 1;
    }
    return p;
}


/*
================
Draw_CachePic
================
*/
qPic_p Draw_CachePic(cStringRO path) {
    cachepic_p pic;
    int   i;
    qPic_p dat;
    glpic_p gl;

    for (pic = menu_cachepics, i = 0; i < menu_numcachepics; pic++, i++)
        if (!strcmp(path, pic->name))
            return &pic->pic;

    if (menu_numcachepics == MAX_CACHED_PICS)
        Host_SysError("menu_numcachepics == MAX_CACHED_PICS");
    menu_numcachepics++;
    strcpy(pic->name, path);

    //
    // load the pic from disk
    //
    dat = (qPic_p)COM_LoadTempFile(path);
    if (!dat)
        Host_SysError("Draw_CachePic: failed to load %s", path);
    SwapPic(dat);

    // HACK HACK HACK --- we need to keep the bytes for
    // the translatable player picture just for the menu
    // configuration dialog
    if (!strcmp(path, "gfx/menuplyr.lmp"))
        memcpy(menuplyr_pixels, dat->data, dat->width * dat->height);

    pic->pic.width = dat->width;
    pic->pic.height = dat->height;

    gl = (glpic_p)pic->pic.data;
    gl->texnum = GL_LoadPicTexture(dat);
    gl->sl = 0;
    gl->sh = 1;
    gl->tl = 0;
    gl->th = 1;

    return &pic->pic;
}


void Draw_CharToConback(int num, uint8_p dest) {
    int  row, col;
    uint8_p source;
    int  drawline;
    int  x;

    row = num >> 4;
    col = num & 15;
    source = _drawChars + (row << 10) + (col << 3);

    drawline = 8;

    while (drawline--) {
        for (x = 0; x < 8; x++)
            if (source[x] != 255)
                dest[x] = 0x60 + source[x];
        source += 128;
        dest += 320;
    }

}

typedef struct
{
    cString name;
    int minimize, maximize;
} glmode_t;

glmode_t modes[] = {
    {"GL_NEAREST", GL_NEAREST, GL_NEAREST},
    {"GL_LINEAR", GL_LINEAR, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
    {"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
    {"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
===============
Draw_TextureMode_f
===============
*/
void Draw_TextureMode_f(void) {
    int  i;
    glTexture_p glt;

    if (Cmd_Argc() == 1) {
        for (i = 0; i < 6; i++)
            if (gl_filter_min == modes[i].minimize) {
                Con_Printf("%s\n", modes[i].name);
                return;
            }
        Con_Printf("current filter is unknown???\n");
        return;
    }

    for (i = 0; i < 6; i++) {
        if (!Q_strcasecmp(modes[i].name, Cmd_Argv(1)))
            break;
    }
    if (i == 6) {
        Con_Printf("bad filter name\n");
        return;
    }

    gl_filter_min = modes[i].minimize;
    gl_filter_max = modes[i].maximize;

    // change all the existing mipmap texture objects
    for (i = 0, glt = gltextures; i < numgltextures; i++, glt++) {
        if (glt->mipmap) {
            GL_Bind(glt->texnum);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
        }
    }
}

/*
===============
Draw_Init
===============
*/
void Draw_Init(void) {
    Cvar_RegisterVariable(&gl_nobind);
    Cvar_RegisterVariable(&gl_max_size);
    Cvar_RegisterVariable(&gl_picmip);

    // 3dfx can only handle 256 wide textures
    if (!Q_strncasecmp((cString)gl_renderer, "3dfx", 4) ||
        strstr((cString)gl_renderer, "Glide"))
        Cvar_Set("gl_max_size", "256");

    Cmd_AddCommand("gl_texturemode", &Draw_TextureMode_f);

    // load the console background and the charset
    // by hand, because we need to write the version
    // string into the background before turning
    // it into a texture
    _drawChars = W_GetLumpName("conchars");
    for (int i = 0; i < 256 * 64; i++)
        if (_drawChars[i] == 0)
            _drawChars[i] = 255; // proper transparent color

    // now turn them into textures
    char_texture = GL_LoadTexture("charset", 128, 128, _drawChars, false, true);

    size_t start = Hunk_LowMark();

    qPic_p cb = (qPic_p)COM_LoadTempFile("gfx/conback.lmp");
    if (!cb)
        Host_SysError("Couldn't load gfx/conback.lmp");
    SwapPic(cb);

    // hack the version number directly into the pic
    char ver[40];
    snprintf(ver, sizeof(ver),
#if defined(__linux__)
        "(Linux %2.2f, gl %4.2f) %4.2f", (float)LINUX_VERSION,
#else
        "(gl %4.2f) %4.2f",
#endif
        (float)GLQUAKE_VERSION, (float)VERSION);
    uint8_p dest = cb->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
    int y = strlen(ver);
    for (int x = 0; x < y; x++)
        Draw_CharToConback(ver[x], dest + (x << 3));

#if 0
    conback->width = vid.conwidth;
    conback->height = vid.conheight;

    // scale console to vid size
    uint8_p ncdata
        uint8_p dest = ncdata = Hunk_AllocName(vid.conwidth * vid.conheight, "conback");

    for (int y = 0; y < vid.conheight; y++, dest += vid.conwidth) {
        src = cb->data + cb->width * (y * cb->height / vid.conheight);
        if (vid.conwidth == cb->width)
            memcpy(dest, src, vid.conwidth);
        else {
            f = 0;
            fstep = cb->width * 0x10000 / vid.conwidth;
            for (int x = 0; x < vid.conwidth; x += 4) {
                dest[x + 0] = src[f >> 16];     f += fstep;
                dest[x + 1] = src[f >> 16];     f += fstep;
                dest[x + 2] = src[f >> 16];     f += fstep;
                dest[x + 3] = src[f >> 16];     f += fstep;
            }
        }
    }
#else
    conback->width = cb->width;
    conback->height = cb->height;
    uint8_p ncdata = cb->data;
#endif

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glpic_p gl = (glpic_p)conback->data;
    gl->texnum = GL_LoadTexture("conback", conback->width, conback->height, ncdata, false, false);
    gl->sl = 0;
    gl->sh = 1;
    gl->tl = 0;
    gl->th = 1;
    conback->width = vid.width;
    conback->height = vid.height;

    // free loaded console
    Hunk_FreeToLowMark(start);

    // save a texture slot for translated picture
    translate_texture = texture_extension_number++;

    // save slots for scraps
    scrap_texnum = texture_extension_number;
    texture_extension_number += MAX_SCRAPS;

    //
    // get the other pics we need
    //
    draw_disc = Draw_PicFromWad("disc");
    draw_backtile = Draw_PicFromWad("backtile");
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
    if (num == 32)
        return;  // space

    num &= 255;

    if (y <= -8)
        return;   // totally off screen

    int row = num >> 4;
    int col = num & 15;

    float frow = row * 0.0625;
    float fcol = col * 0.0625;
    float size = 0.0625;

    GL_Bind(char_texture);

    glBegin(GL_QUADS);
    glTexCoord2f(fcol, frow);
    glVertex2f(x, y);
    glTexCoord2f(fcol + size, frow);
    glVertex2f(x + 8, y);
    glTexCoord2f(fcol + size, frow + size);
    glVertex2f(x + 8, y + 8);
    glTexCoord2f(fcol, frow + size);
    glVertex2f(x, y + 8);
    glEnd();
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
void Draw_DebugChar(char num) {}

/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic(int x, int y, qPic_p pic, float alpha) {
    if (scrap_dirty)
        Scrap_Upload();
    glpic_p gl = (glpic_p)pic->data;
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glCullFace(GL_FRONT);
    glColor4f(1, 1, 1, alpha);
    GL_Bind(gl->texnum);
    glBegin(GL_QUADS);
    glTexCoord2f(gl->sl, gl->tl);
    glVertex2f(x, y);
    glTexCoord2f(gl->sh, gl->tl);
    glVertex2f(x + pic->width, y);
    glTexCoord2f(gl->sh, gl->th);
    glVertex2f(x + pic->width, y + pic->height);
    glTexCoord2f(gl->sl, gl->th);
    glVertex2f(x, y + pic->height);
    glEnd();
    glColor4f(1, 1, 1, 1);
    glEnable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, qPic_p pic) {
    if (!pic)
       return;

    if (scrap_dirty)
        Scrap_Upload();
    glpic_p gl = (glpic_p)pic->data;
    glColor4f(1, 1, 1, 1);
    GL_Bind(gl->texnum);
    glBegin(GL_QUADS);
    glTexCoord2f(gl->sl, gl->tl);
    glVertex2f(x, y);
    glTexCoord2f(gl->sh, gl->tl);
    glVertex2f(x + pic->width, y);
    glTexCoord2f(gl->sh, gl->th);
    glVertex2f(x + pic->width, y + pic->height);
    glTexCoord2f(gl->sl, gl->th);
    glVertex2f(x, y + pic->height);
    glEnd();
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic(int x, int y, qPic_p pic) {
    if ((x < 0) ||
        ((uint32_t)(x + pic->width) > vid.width) ||
        (y < 0) ||
        ((uint32_t)(y + pic->height) > vid.height)
        ) {
        Host_SysError("Draw_TransPic: bad coordinates");
    }

    Draw_Pic(x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate(int x, int y, qPic_p pic, uint8_p translation) {
    uint32_t  trans[64 * 64];

    GL_Bind(translate_texture);

    // int c = pic->width * pic->height;

    uint32_p dest = trans;
    for (int v = 0; v < 64; v++, dest += 64) {
        uint8_p src = &menuplyr_pixels[((v * pic->height) >> 6) * pic->width];
        for (int u = 0; u < 64; u++) {
            int p = src[(u * pic->width) >> 6];
            if (p == 255)   dest[u] = p;
            else            dest[u] = d_8to24table[translation[p]];
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(x, y);
    glTexCoord2f(1, 0);
    glVertex2f(x + pic->width, y);
    glTexCoord2f(1, 1);
    glVertex2f(x + pic->width, y + pic->height);
    glTexCoord2f(0, 1);
    glVertex2f(x, y + pic->height);
    glEnd();
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines) {
    int y = (vid.height * 3) >> 2;

    if (lines > y)  Draw_Pic(0, lines - vid.height, conback);
    else            Draw_AlphaPic(0, lines - vid.height, conback, (float)(1.2 * lines) / y);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h) {
    glColor3f(1, 1, 1);
    GL_Bind(*(int*)draw_backtile->data);
    glBegin(GL_QUADS);
    glTexCoord2f(x / 64.0, y / 64.0);
    glVertex2f(x, y);
    glTexCoord2f((x + w) / 64.0, y / 64.0);
    glVertex2f(x + w, y);
    glTexCoord2f((x + w) / 64.0, (y + h) / 64.0);
    glVertex2f(x + w, y + h);
    glTexCoord2f(x / 64.0, (y + h) / 64.0);
    glVertex2f(x, y + h);
    glEnd();
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(host_basepal[c * 3] / 255.0,
        host_basepal[c * 3 + 1] / 255.0,
        host_basepal[c * 3 + 2] / 255.0);

    glBegin(GL_QUADS);

    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);

    glEnd();
    glColor3f(1, 1, 1);
    glEnable(GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void) {
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 0.8);
    glBegin(GL_QUADS);

    glVertex2f(0, 0);
    glVertex2f(vid.width, 0);
    glVertex2f(vid.width, vid.height);
    glVertex2f(0, vid.height);

    glEnd();
    glColor4f(1, 1, 1, 1);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc(void) {
    if (!draw_disc)
        return;
    glDrawBuffer(GL_FRONT);
    Draw_Pic(vid.width - 24, 0, draw_disc);
    glDrawBuffer(GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc(void) {
}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D(void) {
    glViewport(glx, gly, glwidth, glheight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, vid.width, vid.height, 0, -99999, 99999);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    // glDisable (GL_ALPHA_TEST);

    glColor4f(1, 1, 1, 1);
}

//====================================================================

/*
================
GL_FindTexture
================
*/
int GL_FindTexture(cString identifier) {
    glTexture_p glt = gltextures;
    for (int i = 0; i < numgltextures; i++, glt++) {
        if (!strcmp(identifier, glt->identifier))
            return gltextures[i].texnum;
    }

    return -1;
}

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture(uint32_p in, int inwidth, int inheight, uint32_p out, int outwidth, int outheight) {
    uint32_t fracstep = inwidth * 0x10000 / outwidth;
    for (int i = 0; i < outheight; i++, out += outwidth) {
        uint32_p inrow = in + inwidth * (i * inheight / outheight);
        uint32_t frac = fracstep >> 1;
        for (int j = 0; j < outwidth; j += 4) {
            out[j + 0] = inrow[frac >> 16];     frac += fracstep;
            out[j + 1] = inrow[frac >> 16];     frac += fracstep;
            out[j + 2] = inrow[frac >> 16];     frac += fracstep;
            out[j + 3] = inrow[frac >> 16];     frac += fracstep;
        }
    }
}

/*
================
GL_Resample8BitTexture -- JACK
================
*/
void GL_Resample8BitTexture(uint8_p in, int inwidth, int inheight, uint8_p out, int outwidth, int outheight) {
    uint32_t fracstep = inwidth * 0x10000 / outwidth;
    for (int i = 0; i < outheight; i++, out += outwidth) {
        uint8_p inrow = in + inwidth * (i * inheight / outheight);
        uint32_t frac = fracstep >> 1;
        for (int j = 0; j < outwidth; j += 4) {
            out[j + 1] = inrow[frac >> 16];     frac += fracstep;
            out[j + 1] = inrow[frac >> 16];     frac += fracstep;
            out[j + 2] = inrow[frac >> 16];     frac += fracstep;
            out[j + 3] = inrow[frac >> 16];     frac += fracstep;
        }
    }
}


/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap(uint8_p in, int width, int height) {
    width <<= 2;
    height >>= 1;
    uint8_p out = in;
    for (int i = 0; i < height; i++, in += width) {
        for (int j = 0; j < width; j += 8, out += 4, in += 8) {
            out[0] = (in[0] + in[4] + in[width + 0] + in[width + 4]) >> 2;
            out[1] = (in[1] + in[5] + in[width + 1] + in[width + 5]) >> 2;
            out[2] = (in[2] + in[6] + in[width + 2] + in[width + 6]) >> 2;
            out[3] = (in[3] + in[7] + in[width + 3] + in[width + 7]) >> 2;
        }
    }
}

/*
================
GL_MipMap8Bit

Mipping for 8 bit textures
================
*/
void GL_MipMap8Bit(uint8_p in, int width, int height) {
    // width <<=2;
    height >>= 1;
    uint8_p out = in;
    for (int i = 0; i < height; i++, in += width) {
        for (int j = 0; j < width; j += 2, out += 1, in += 2) {
            uint8_p at1 = (uint8_p)(d_8to24table + in[0]);
            uint8_p at2 = (uint8_p)(d_8to24table + in[1]);
            uint8_p at3 = (uint8_p)(d_8to24table + in[width + 0]);
            uint8_p at4 = (uint8_p)(d_8to24table + in[width + 1]);

            uint16_t r = (at1[0] + at2[0] + at3[0] + at4[0]); r >>= 5;
            uint16_t g = (at1[1] + at2[1] + at3[1] + at4[1]); g >>= 5;
            uint16_t b = (at1[2] + at2[2] + at3[2] + at4[2]); b >>= 5;

            out[0] = d_15to8table[(r << 0) + (g << 5) + (b << 10)];
        }
    }
}

/*
===============
GL_Upload32
===============
*/
void GL_Upload32(uint32_p data, int width, int height, bool mipmap, bool alpha) {
    static uint32_t _scaled[1024 * 512]; // [512*256];

    int scaled_width = 1;
    for (; scaled_width < width; scaled_width <<= 1) {}
    int scaled_height = 1;
    for (; scaled_height < height; scaled_height <<= 1) {}

    scaled_width >>= (int)gl_picmip.value;
    scaled_height >>= (int)gl_picmip.value;

    if (scaled_width > gl_max_size.value)       scaled_width = gl_max_size.value;
    if (scaled_height > gl_max_size.value)      scaled_height = gl_max_size.value;

    if (scaled_width * scaled_height > sizeof(_scaled) / 4)
        Host_SysError("GL_LoadTexture: too big");

    int samples = alpha ? gl_alpha_format : gl_solid_format;

#if 0
    if (mipmap)
        gluBuild2DMipmaps(GL_TEXTURE_2D, samples, width, height, GL_RGBA, GL_UNSIGNED_BYTE, trans);
    else if (scaled_width == width && scaled_height == height)
        glTexImage2D(GL_TEXTURE_2D, 0, samples, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
    else {
        gluScaleImage(GL_RGBA, width, height, GL_UNSIGNED_BYTE, trans,
            scaled_width, scaled_height, GL_UNSIGNED_BYTE, _scaled);
        glTexImage2D(GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _scaled);
    }
#else
    texels += scaled_width * scaled_height;

    if (scaled_width == width && scaled_height == height) {
        if (!mipmap) {
            glTexImage2D(GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            goto done;
        }
        memcpy(_scaled, data, width * height * 4);
    }
    else
        GL_ResampleTexture(data, width, height, _scaled, scaled_width, scaled_height);

    glTexImage2D(GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _scaled);
    if (mipmap) {
        int miplevel = 0;
        while (scaled_width > 1 || scaled_height > 1) {
            GL_MipMap((uint8_p)_scaled, scaled_width, scaled_height);
            scaled_width >>= 1;
            scaled_height >>= 1;
            if (scaled_width < 1)   scaled_width = 1;
            if (scaled_height < 1)  scaled_height = 1;
            miplevel++;
            glTexImage2D(GL_TEXTURE_2D, miplevel, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _scaled);
        }
    }
done:;
#endif


    if (mipmap) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }
    else {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }
}

void GL_Upload8_EXT(uint8_p data, int width, int height, bool mipmap, bool alpha) {
    static uint8_t _scaled[1024 * 512]; // [512*256];

    int s = width * height;
    // if there are no transparent pixels, make it a 3 component
    // texture even if it was specified as otherwise
    if (alpha) {
        bool noalpha = true;
        for (int i = 0; i < s; i++) {
            if (data[i] == 255)
                noalpha = false;
        }

        if (alpha && noalpha)
            alpha = false;
    }
    int scaled_width = 1;
    for (; scaled_width < width; scaled_width <<= 1) {}
    int scaled_height = 1;
    for (; scaled_height < height; scaled_height <<= 1) {}

    scaled_width >>= (int)gl_picmip.value;
    scaled_height >>= (int)gl_picmip.value;

    if (scaled_width > gl_max_size.value)   scaled_width = gl_max_size.value;
    if (scaled_height > gl_max_size.value)  scaled_height = gl_max_size.value;

    if (scaled_width * scaled_height > sizeof(_scaled))
        Host_SysError("GL_LoadTexture: too big");

    // int samples = 1; // alpha ? gl_alpha_format : gl_solid_format;

    texels += scaled_width * scaled_height;

    if (scaled_width == width && scaled_height == height) {
        if (!mipmap) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data);
            goto done;
        }
        memcpy(_scaled, data, width * height);
    }
    else
        GL_Resample8BitTexture(data, width, height, _scaled, scaled_width, scaled_height);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, _scaled);
    if (mipmap) {
        int  miplevel;

        miplevel = 0;
        while ((scaled_width > 1) ||
            (scaled_height > 1)
            ) {
            GL_MipMap8Bit((uint8_p)_scaled, scaled_width, scaled_height);
            scaled_width >>= 1;
            scaled_height >>= 1;
            if (scaled_width < 1)       scaled_width = 1;
            if (scaled_height < 1)      scaled_height = 1;
            miplevel++;
            glTexImage2D(GL_TEXTURE_2D, miplevel, GL_COLOR_INDEX8_EXT, scaled_width, scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, _scaled);
        }
    }
done:;


    if (mipmap) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }
    else {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }
}

/*
===============
GL_Upload8
===============
*/
void GL_Upload8(uint8_p data, int width, int height, bool mipmap, bool alpha) {
    static uint32_t _trans[640 * 480];  // FIXME, temporary

    int s = width * height;
    // if there are no transparent pixels, make it a 3 component
    // texture even if it was specified as otherwise
    if (alpha) {
        bool noalpha = true;
        for (int i = 0; i < s; i++) {
            int p = data[i];
            if (p == 255)
                noalpha = false;
            _trans[i] = d_8to24table[p];
        }

        if (alpha && noalpha)
            alpha = false;
    }
    else {
        if (s & 3)
            Host_SysError("GL_Upload8: s&3");
        for (int i = 0; i < s; i += 4) {
            _trans[i + 0] = d_8to24table[data[i + 0]];
            _trans[i + 1] = d_8to24table[data[i + 1]];
            _trans[i + 2] = d_8to24table[data[i + 2]];
            _trans[i + 3] = d_8to24table[data[i + 3]];
        }
    }

    if (VID_Is8bit() && !alpha && (data != scrap_texels[0])) {
        GL_Upload8_EXT(data, width, height, mipmap, alpha);
        return;
    }
    GL_Upload32(_trans, width, height, mipmap, alpha);
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture(cString identifier, int width, int height, uint8_p data, bool mipmap, bool alpha) {
    glTexture_p glt;

    // see if the texture is allready present
    if (identifier[0]) {
        glt = gltextures;
        for (int i = 0; i < numgltextures; i++, glt++) {
            if (!strcmp(identifier, glt->identifier)) {
                if (width != glt->width || height != glt->height)
                    Host_SysError("GL_LoadTexture: cache mismatch");
                return gltextures[i].texnum;
            }
        }
    }
    else {
        glt = &gltextures[numgltextures];
        numgltextures++;
    }

    strcpy(glt->identifier, identifier);
    glt->texnum = texture_extension_number;
    glt->width = width;
    glt->height = height;
    glt->mipmap = mipmap;

    GL_Bind(texture_extension_number);

    GL_Upload8(data, width, height, mipmap, alpha);

    texture_extension_number++;

    return texture_extension_number - 1;
}

/*
================
GL_LoadPicTexture
================
*/
int GL_LoadPicTexture(qPic_p pic) {
    return GL_LoadTexture("", pic->width, pic->height, pic->data, false, true);
}

/****************************************/

static GLenum _oldTarget = TEXTURE0_SGIS;

void GL_SelectTexture(GLenum target) {
    if (!gl_mtexable)
        return;
    qglSelectTextureSGIS(target);
    if (target == _oldTarget)
        return;
    cnttextures[_oldTarget - TEXTURE0_SGIS] = currenttexture;
    currenttexture = cnttextures[target - TEXTURE0_SGIS];
    _oldTarget = target;
}
