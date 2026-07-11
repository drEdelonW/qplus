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
#include "enginedefs.h"
#include "console.h"
#include "host.h"
#include "wad.h"
#include "common.h"
#include "cmd.h"
#include "q_tools.h"
#include <string.h>
#include "sbar.h"
#include "z_hunk.h"
#include "screen.h"


// draw.c -- this is the only file outside the refresh that touches the
// vid buffer


#define GL_COLOR_INDEX8_EXT     0x80E5


cvar_t  gl_nobind = { "gl_nobind", "0" };
cvar_t  gl_max_size = { "gl_max_size", "1024" };
cvar_t  gl_picmip = { "gl_picmip", "0" };

qColor8_p _drawChars;    // 8*8 graphic characters
qPic_p draw_disc;
qPic_p draw_backtile;

int   translate_texture;
int   char_texture;

typedef struct {
    int   texnum;
    float sl;
    float tl;
    float sh;
    float th;
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
    name_t identifier;
    int  width, height;
    bool mipmap;
} glTexture_t;
typedef glTexture_t* glTexture_p;

#define MAX_GLTEXTURES 1024
glTexture_t gltextures[MAX_GLTEXTURES];
int   numgltextures;

static GLenum last_bind_error = GL_NO_ERROR;
static int error_texture_id = 0;

bool GL_PrintLastError() {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("GL_Bind error: 0x%X\n", err);
        last_bind_error = err;
    }
    return err != GL_NO_ERROR;
}

void GL_Bind(int texnum) {
    if (gl_nobind.value)    texnum = char_texture;
    if (currenttexture == texnum)
        return;

    currenttexture = texnum;
#ifdef _WIN32
    bindTexFunc // the same  but in thu GetProcAddress "glBindTexture"
#else
    glBindTexture
#endif
        (GL_TEXTURE_2D, texnum);

    if (GL_PrintLastError()) {
        error_texture_id = texnum;
    }
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

int     scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
qColor8_t scrap_texels[MAX_SCRAPS][BLOCK_WIDTH * BLOCK_HEIGHT * 4];
bool    scrap_dirty;
int     scrap_texnum;

// returns a texture number and the position inside it
int Scrap_AllocBlock(int w, int h, int* x, int* y) {
    for (int texnum = 0; texnum < MAX_SCRAPS; texnum++) {
        int best = BLOCK_HEIGHT;

        for (int i = 0; i < BLOCK_WIDTH - w; i++) {
            int best2 = 0;

            int j = 0;
            for (; j < w; j++) {
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
void GL_Upload8(qColor8_p data, int width, int height, bool mipmap, bool alpha); // TODO: fix dependence hell
void Scrap_Upload() {
    scrap_uploads++;

    for (int texnum = 0; texnum < MAX_SCRAPS; texnum++) {
        GL_Bind(scrap_texnum + texnum);
        GL_Upload8(scrap_texels[texnum], BLOCK_WIDTH, BLOCK_HEIGHT, false, true);
    }
    scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s {
    qPath_t name;
    qPic_t  pic;
    byte    padding[32]; // for appended glpic
} cachepic_t;
typedef cachepic_t* cachepic_p;

#define MAX_CACHED_PICS  128
cachepic_t menu_cachepics[MAX_CACHED_PICS];
int   menu_numcachepics;

byte  menuplyr_pixels[4096];

#ifdef GLTEST
int  pic_texels;
int  pic_count;
#endif

int GL_LoadPicTexture(qPic_p pic);

qPic_p Draw_PicFromWad(cStringRO name) {
    qPic_p p = W_GetLumpName(name);

    // load little ones into the scrap
    if ((p->width < 64) &&
        (p->height < 64)
        ) {
        int  x, y;
        int  texnum = Scrap_AllocBlock(p->width, p->height, &x, &y);
        scrap_dirty = true;
        int k = 0;
        for (int i = 0; i < p->height; i++)
            for (int j = 0; j < p->width; j++, k++)
                scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = p->data[k];

        texnum += scrap_texnum;

        *((glpic_p)p->data) = (glpic_t){
            .texnum = texnum,
            .sl = (x + 0.01) / (float)BLOCK_WIDTH,
            .tl = (y + 0.01) / (float)BLOCK_WIDTH,
            .sh = (x + p->width - 0.01) / (float)BLOCK_WIDTH,
            .th = (y + p->height - 0.01) / (float)BLOCK_WIDTH
        };
#ifdef GLTEST
        pic_count++;
        pic_texels += p->width * p->height;
#endif
    }
    else {
        *((glpic_p)p->data) = (glpic_t){
            .texnum = GL_LoadPicTexture(p),
            .sl = 0.f,
            .tl = 0.f,
            .sh = 1.f,
            .th = 1.f
        };
    }
    return p;
}


/*
================
Draw_CachePic
================
*/
qPic_p Draw_CachePic(cStringRO path) {
    cachepic_p pic = menu_cachepics;
    for (int i = 0; i < menu_numcachepics; pic++, i++)
        if (!strcmp(path, pic->name))
            return &pic->pic;

    if (menu_numcachepics == MAX_CACHED_PICS)
        Host_SysError("menu_numcachepics == MAX_CACHED_PICS");

    menu_numcachepics++;
    strcpy(pic->name, path);

    //
    // load the pic from disk
    //
    qPic_p dat = (qPic_p)COM_LoadTempFile(path);
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

    *((glpic_p)pic->pic.data) = (glpic_t){
        .texnum = GL_LoadPicTexture(dat),
        .sl = 0.f,
        .tl = 0.f,
        .sh = 1.f,
        .th = 1.f
    };
    return &pic->pic;
}


void Draw_CharToConback(int num, qColor8_p dest) {
    int row = num >> 4;
    int col = num & 0x0F;
    qColor8_p source = _drawChars + (row << 10) + (col << 3);

    int drawline = 8;

    while (drawline--) {
        for (int x = 0; x < 8; x++)
            if (source[x].i != 255)
                dest[x].i = 0x60 + source[x].i;
        source += 128;
        dest += 320;
    }

}

typedef struct {
    cString name;
    int minimize;
    int maximize;
} glMode_t;

glMode_t modes[] = {
    {"GL_NEAREST", GL_NEAREST, GL_NEAREST},
    {"GL_LINEAR", GL_LINEAR, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
    {"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
    {"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUMFILTERMODE  (sizeof(modes)/sizeof(glMode_t)) 
/*
===============
Draw_TextureMode_f
===============
*/
void Draw_TextureMode_f() {
    if (Cmd_Argc() == 1) {
        for (int i = 0; i < NUMFILTERMODE; i++)
            if (gl_filter_min == modes[i].minimize) {
                Con_Printf("%s\n", modes[i].name);
                return;
            }
        Con_Printf("current filter [0x%X] is unknown???\n", gl_filter_min);     return;
    }
    else {
        int i = 0;
        for (; i < NUMFILTERMODE; i++) {
            if (!Q_strcasecmp(modes[i].name, Cmd_Argv(1)))
                break;
        }
        if (i == NUMFILTERMODE) {
            Con_Printf("bad filter name \"%s\"\n", Cmd_Argv(1));    return;
        }

        gl_filter_min = modes[i].minimize;
        gl_filter_max = modes[i].maximize;

        // change all the existing mipmap texture objects
        glTexture_p glt = gltextures;
        for (int i = 0; i < numgltextures; i++, glt++) {
            if (glt->mipmap) {
                GL_Bind(glt->texnum);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
            }
        }
    }
}

/*
===============
Draw_Init
===============
*/
void Draw_Init() {
    Cvar_RegisterVariable(&gl_nobind);
    Cvar_RegisterVariable(&gl_max_size);
    Cvar_RegisterVariable(&gl_picmip);

    // 3dfx can only handle 256 wide textures
    if (!(Q_strncasecmp((cString)gl_renderer, "3dfx", 4)) ||
        strstr((cString)gl_renderer, "Glide")
        )
        Cvar_Set("gl_max_size", "256");

    Cmd_AddCommand("gl_texturemode", &Draw_TextureMode_f);

    // load the console background and the charset by hand, because we need to write the version string into the background before turning it into a texture
    _drawChars = W_GetLumpName("conchars");
    for (int i = 0; i < InksNum * 64; i++)
        if (_drawChars[i].i == 0)
            _drawChars[i].i = InkTransp; // proper transparent color

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
        (float)GLQUAKE_VERSION, (float)VERSION
    );
    qColor8_p dest = cb->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
    int y = strlen(ver);
    for (int x = 0; x < y; x++)
        Draw_CharToConback(ver[x], dest + (x << 3));

#if 0
    conback->width = vid.con.width;
    conback->height = vid.con.height;

    // scale console to vid size
    uint8_p ncdata
        uint8_p dest = ncdata = Hunk_AllocName(vid.con.width * vid.con.height, "conback");

    for (int y = 0; y < vid.con.height; y++, dest += vid.con.width) {
        src = cb->data + cb->width * (y * cb->height / vid.con.height);
        if (vid.con.width == cb->width)
            memcpy(dest, src, vid.con.width);
        else {
            fixed16_t f = 0;
            fixed16_t fstep = cb->width * FIXED16_ONE / vid.con.width;
            for (int x = 0; x < vid.con.width; x += 4) {
                dest[x + 0] = src[FIXED16_TO_INT(f)];     f += fstep;
                dest[x + 1] = src[FIXED16_TO_INT(f)];     f += fstep;
                dest[x + 2] = src[FIXED16_TO_INT(f)];     f += fstep;
                dest[x + 3] = src[FIXED16_TO_INT(f)];     f += fstep;
            }
        }
    }
#else
    conback->width = cb->width;
    conback->height = cb->height;
    qColor8_p ncdata = cb->data;
#endif

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    *((glpic_p)conback->data) = (glpic_t){
        .texnum = GL_LoadTexture("conback", conback->width, conback->height, ncdata, false, false),
        .sl = 0.f,
        .tl = 0.f,
        .sh = 1.f,
        .th = 1.f
    };

    conback->width = Scr.vrect.width;
    conback->height = Scr.vrect.height;

    Hunk_FreeToLowMark(start);      // free loaded console

    translate_texture = texture_extension_number++;     // save a texture slot for translated picture

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
#define CHAR_SCALE_F    (0.0625f) /* seems like character size in texture 1.f space (1/16)*/

void Draw_Character(int x, int y, int num) {
    num &= 0xFF;
    if (num == ' ')     return; // 32 - space

    if (y <= -8)    return; // totally off screen

    int row = num >> 4;
    int col = num & 0x0F;

    float frow = row * CHAR_SCALE_F;
    float fcol = col * CHAR_SCALE_F;
    float size = CHAR_SCALE_F;

    GL_Bind(char_texture);
    glBegin(GL_QUADS); {
        glTexCoord2f(fcol, frow);               glVertex2f(x, y);
        glTexCoord2f(fcol + size, frow);        glVertex2f(x + 8, y);
        glTexCoord2f(fcol + size, frow + size); glVertex2f(x + 8, y + 8);
        glTexCoord2f(fcol, frow + size);        glVertex2f(x, y + 8);
    } glEnd();
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
#if 0   // it was commented
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_FRONT);
#endif
    glColor4f(1.f, 1.f, 1.f, alpha);
    GL_Bind(gl->texnum);
    glBegin(GL_QUADS); {
        glTexCoord2f(gl->sl, gl->tl);   glVertex2f(x, y);
        glTexCoord2f(gl->sh, gl->tl);   glVertex2f(x + pic->width, y);
        glTexCoord2f(gl->sh, gl->th);   glVertex2f(x + pic->width, y + pic->height);
        glTexCoord2f(gl->sl, gl->th);   glVertex2f(x, y + pic->height);
    } glEnd();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    glEnable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, qPic_p pic) {
    if (!pic)   return;

    if (scrap_dirty)
        Scrap_Upload();

    glpic_p gl = (glpic_p)pic->data;
    glColor4f(1.f, 1.f, 1.f, 1.f);
    GL_Bind(gl->texnum);
    glBegin(GL_QUADS); {
        glTexCoord2f(gl->sl, gl->tl);   glVertex2f(x, y);
        glTexCoord2f(gl->sh, gl->tl);   glVertex2f(x + pic->width, y);
        glTexCoord2f(gl->sh, gl->th);   glVertex2f(x + pic->width, y + pic->height);
        glTexCoord2f(gl->sl, gl->th);   glVertex2f(x, y + pic->height);
    } glEnd();
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic(int x, int y, qPic_p pic) {
    if ((x < 0) ||
        (y < 0) ||
        ((x + pic->width) > Scr.vrect.width) ||
        ((y + pic->height) > Scr.vrect.height)
        )   Host_SysError("Draw_TransPic: bad coordinates");

    Draw_Pic(x, y, pic);
}


/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate(int x, int y, qPic_p pic, palMap_p translation) {
    GL_Bind(translate_texture);

    // int c = pic->width * pic->height;

    uint32_t trans[64 * 64];
    uint32_p dest = trans;
    for (int v = 0; v < 64; v++, dest += 64) {
        uint8_p src = &menuplyr_pixels[(DIV64(v * pic->height)) * pic->width];
        for (int u = 0; u < 64; u++) {
            int p = src[DIV64(u * pic->width)];
            if (p == 255)   dest[u] = p;
            else            dest[u] = d_8to24table[translation->pal[p].i];
        }
    }

    glTexImage2D(
        GL_TEXTURE_2D,
        0, gl_alpha_format,
        64, 64,
        0, GL_RGBA,
        GL_UNSIGNED_BYTE, trans
    );

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glColor3f(1, 1, 1);
    glBegin(GL_QUADS); {
        glTexCoord2f(0.f, 0.f);   glVertex2f(x, y);
        glTexCoord2f(1.f, 0.f);   glVertex2f(x + pic->width, y);
        glTexCoord2f(1.f, 1.f);   glVertex2f(x + pic->width, y + pic->height);
        glTexCoord2f(0.f, 1.f);   glVertex2f(x, y + pic->height);
    } glEnd();
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines) {
    int y = QUARTER(Scr.vrect.height * 3);

    if (lines > y)  Draw_Pic(0, lines - Scr.vrect.height, conback);
    else            Draw_AlphaPic(0, lines - Scr.vrect.height, conback, (float)(1.2 * lines) / y);
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h) {
    int x1 = x + w;
    int y1 = y + h;
    glColor3f(1.f, 1.f, 1.f);
    GL_Bind(*(int*)draw_backtile->data);
    glBegin(GL_QUADS); {
        glTexCoord2f(x / 64.f, y / 64.f);         glVertex2f(x, y);
        glTexCoord2f((x1) / 64.f, y / 64.f);      glVertex2f(x1, y);
        glTexCoord2f((x1) / 64.f, (y1) / 64.f);   glVertex2f(x1, y1);
        glTexCoord2f(x / 64.f, (y1) / 64.f);      glVertex2f(x, y1);
    } glEnd();
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c) {

    glDisable(GL_TEXTURE_2D); {
        glColor3f(
            host_basepal->ink[c].r / 255.f,
            host_basepal->ink[c].g / 255.f,
            host_basepal->ink[c].b / 255.f
        ); {
            glBegin(GL_QUADS); {
                glVertex2f(x, y);   int x1 = x + w;
                glVertex2f(x1, y);  int y1 = y + h;
                glVertex2f(x1, y1);
                glVertex2f(x, y1);
            } glEnd();
        }glColor3f(1.f, 1.f, 1.f);
    }glEnable(GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen() {
    glEnable(GL_BLEND); {
        glDisable(GL_TEXTURE_2D); {
            glColor4f(0.f, 0.f, 0.f, 0.8f); {
                glBegin(GL_QUADS); {
                    glVertex2f(0.f, 0.f);
                    glVertex2f(Scr.vrect.width, 0.f);
                    glVertex2f(Scr.vrect.width, Scr.vrect.height);
                    glVertex2f(0.f, Scr.vrect.height);
                } glEnd();
            } glColor4f(1.f, 1.f, 1.f, 1.f);
        } glEnable(GL_TEXTURE_2D);
    } glDisable(GL_BLEND);

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
void Draw_BeginDisc() {
    if (!draw_disc)     return;

    glDrawBuffer(GL_FRONT);
    Draw_Pic(Scr.vrect.width - 24, 0, draw_disc);
    glDrawBuffer(GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc() {}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D() {
    glViewport(glx, gly, glwidth, glheight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, Scr.vrect.width, Scr.vrect.height, 0, -99999, 99999);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    // glDisable (GL_ALPHA_TEST);

    glColor4f(1.f, 1.f, 1.f, 1.f);
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
void GL_ResampleTexture(
    uint32_p in, int inwidth, int inheight,
    uint32_p out, int outwidth, int outheight
) {
    fixed16_t fracstep = inwidth * FIXED16_ONE / outwidth;
    for (int i = 0; i < outheight; i++, out += outwidth) {
        uint32_p inrow = in + inwidth * (i * inheight / outheight);
        fixed16_t frac = HALF(fracstep);
        for (int j = 0; j < outwidth; j += 4) {
            out[j + 0] = inrow[FIXED16_TO_INT(frac)];     frac += fracstep;
            out[j + 1] = inrow[FIXED16_TO_INT(frac)];     frac += fracstep;
            out[j + 2] = inrow[FIXED16_TO_INT(frac)];     frac += fracstep;
            out[j + 3] = inrow[FIXED16_TO_INT(frac)];     frac += fracstep;
        }
    }
}

/*
================
GL_Resample8BitTexture -- JACK
================
*/
void GL_Resample8BitTexture(
    qColor8_p in, int inwidth, int inheight,
    qColor8_p out, int outwidth, int outheight
) {
    fixed16_t fracstep = inwidth * FIXED16_ONE / outwidth;
    for (int i = 0; i < outheight; i++, out += outwidth) {
        qColor8_p inrow = in + inwidth * (i * inheight / outheight);
        fixed16_t frac = HALF(fracstep);
        for (int j = 0; j < outwidth; j += 4) {
            out[j + 1] = inrow[FIXED16_TO_INT(frac)];     frac += fracstep;
            out[j + 1] = inrow[FIXED16_TO_INT(frac)];     frac += fracstep;
            out[j + 2] = inrow[FIXED16_TO_INT(frac)];     frac += fracstep;
            out[j + 3] = inrow[FIXED16_TO_INT(frac)];     frac += fracstep;
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
    width = QUAD(width);
    height = HALF(height);
    uint8_p out = in;
    for (int i = 0; i < height; i++, in += width) {
        for (int j = 0; j < width; j += 8, out += 4, in += 8) {
            out[0] = QUARTER(in[0] + in[4] + in[width + 0] + in[width + 4]);
            out[1] = QUARTER(in[1] + in[5] + in[width + 1] + in[width + 5]);
            out[2] = QUARTER(in[2] + in[6] + in[width + 2] + in[width + 6]);
            out[3] = QUARTER(in[3] + in[7] + in[width + 3] + in[width + 7]);
        }
    }
}

/*
================
GL_MipMap8Bit

Mipping for 8 bit textures
================
*/
void GL_MipMap8Bit(qColor8_p in, int width, int height) {
    // width = QUAD(width);
    height = HALF(height);
    qColor8_p out = in;
    for (int i = 0; i < height; i++, in += width) {
        for (int j = 0; j < width; j += 2, out += 1, in += 2) {
            uint8_p at1 = (uint8_p)(d_8to24table + in[0].i);
            uint8_p at2 = (uint8_p)(d_8to24table + in[1].i);
            uint8_p at3 = (uint8_p)(d_8to24table + in[width + 0].i);
            uint8_p at4 = (uint8_p)(d_8to24table + in[width + 1].i);

            uint16_t r = DIV32(at1[0] + at2[0] + at3[0] + at4[0]);
            uint16_t g = DIV32(at1[1] + at2[1] + at3[1] + at4[1]);
            uint16_t b = DIV32(at1[2] + at2[2] + at3[2] + at4[2]);

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
    for (; scaled_width < width; scaled_width = TWICE(scaled_width)) {}
    int scaled_height = 1;
    for (; scaled_height < height; scaled_height = TWICE(scaled_height)) {}

    scaled_width >>= (int)gl_picmip.value;
    scaled_height >>= (int)gl_picmip.value;

    CLAMP_MORE(&scaled_width, gl_max_size.value);
    CLAMP_MORE(&scaled_height, gl_max_size.value);

    if (scaled_width * scaled_height > sizeof(_scaled) / 4)
        Host_SysError("GL_LoadTexture: too big");

    int samples = alpha ? gl_alpha_format : gl_solid_format;

#if 0
    if (mipmap)
        gluBuild2DMipmaps(GL_TEXTURE_2D, samples, width, height, GL_RGBA, GL_UNSIGNED_BYTE, trans);
    else if ((scaled_width == width) &&
        (scaled_height == height)
        )
        glTexImage2D(GL_TEXTURE_2D, 0, samples, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
    else {
        gluScaleImage(GL_RGBA, width, height, GL_UNSIGNED_BYTE, trans, scaled_width, scaled_height, GL_UNSIGNED_BYTE, _scaled);
        glTexImage2D(GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _scaled);
    }
#else
    texels += scaled_width * scaled_height;

    if ((scaled_width == width) &&
        (scaled_height == height)
        ) {
        if (!mipmap) {
            glTexImage2D(
                GL_TEXTURE_2D,
                0, samples,
                scaled_width, scaled_height,
                0, GL_RGBA,
                GL_UNSIGNED_BYTE, data
            );      GL_PrintLastError();
            goto done;
        }
        memcpy(_scaled, data, width * height * 4);
    }
    else
        GL_ResampleTexture(data, width, height, _scaled, scaled_width, scaled_height);

    glTexImage2D(
        GL_TEXTURE_2D,
        0, samples,
        scaled_width, scaled_height,
        0, GL_RGBA,
        GL_UNSIGNED_BYTE, _scaled
    );      GL_PrintLastError();

    if (mipmap) {
        int miplevel = 0;
        while (
            (scaled_width > 1) ||
            (scaled_height > 1)
            ) {
            GL_MipMap((uint8_p)_scaled, scaled_width, scaled_height);
            scaled_width = HALF(scaled_width);
            CLAMP_LESS(&scaled_width, 1);

            scaled_height = HALF(scaled_height);
            CLAMP_LESS(&scaled_height, 1);
            miplevel++;
            glTexImage2D(
                GL_TEXTURE_2D,
                miplevel, samples,
                scaled_width, scaled_height,
                0, GL_RGBA,
                GL_UNSIGNED_BYTE, _scaled
            );      GL_PrintLastError();
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

void GL_Upload8_EXT(qColor8_p data, int width, int height, bool mipmap, bool alpha) {
    static qColor8_t _scaled[1024 * 512]; // [512*256];

    int s = width * height;
    // if there are no transparent pixels, make it a 3 component
    // texture even if it was specified as otherwise
    if (alpha) {
        bool noalpha = true;
        for (int i = 0; i < s; i++) {
            if (data[i].i == InkTransp)
                noalpha = false;
        }

        if (alpha && noalpha)
            alpha = false;
    }
    int scaled_width = 1;
    for (; scaled_width < width; scaled_width = TWICE(scaled_width)) {}

    int scaled_height = 1;
    for (; scaled_height < height; scaled_height = TWICE(scaled_height)) {}

    scaled_width >>= (int)gl_picmip.value;
    scaled_height >>= (int)gl_picmip.value;

    CLAMP_MORE(&scaled_width, gl_max_size.value);
    CLAMP_MORE(&scaled_height, gl_max_size.value);

    if (scaled_width * scaled_height > sizeof(_scaled))
        Host_SysError("GL_LoadTexture: too big");

    // int samples = 1; // alpha ? gl_alpha_format : gl_solid_format;

    texels += scaled_width * scaled_height;

    if (scaled_width == width && scaled_height == height) {
        if (!mipmap) {
            glTexImage2D(
                GL_TEXTURE_2D,
                0, GL_COLOR_INDEX8_EXT,
                scaled_width, scaled_height,
                0, GL_COLOR_INDEX,
                GL_UNSIGNED_BYTE, data
            );
            goto done;
        }
        memcpy(_scaled, data, width * height);
    }
    else
        GL_Resample8BitTexture(data,
            width, height,
            _scaled,
            scaled_width, scaled_height
        );

    glTexImage2D(
        GL_TEXTURE_2D,
        0, GL_COLOR_INDEX8_EXT,
        scaled_width, scaled_height,
        0, GL_COLOR_INDEX,
        GL_UNSIGNED_BYTE, _scaled
    );
    if (mipmap) {
        int miplevel = 0;
        while ((scaled_width > 1) ||
            (scaled_height > 1)
            ) {
            GL_MipMap8Bit(_scaled, scaled_width, scaled_height);
            scaled_width = HALF(scaled_width);
            CLAMP_LESS(&scaled_width, 1);

            scaled_height = HALF(scaled_height);
            CLAMP_LESS(&scaled_height, 1);
            miplevel++;
            glTexImage2D(
                GL_TEXTURE_2D,
                miplevel, GL_COLOR_INDEX8_EXT,
                scaled_width, scaled_height,
                0, GL_COLOR_INDEX,
                GL_UNSIGNED_BYTE, _scaled
            );
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
void GL_Upload8(
    qColor8_p data,
    int width, int height,
    bool mipmap,
    bool alpha
) {
    static uint32_t _trans[640 * 480];  // FIXME, temporary

    int s = width * height;
    // if there are no transparent pixels, make it a 3 component
    // texture even if it was specified as otherwise
    if (alpha) {
        bool noalpha = true;
        for (int i = 0; i < s; i++) {
            qColor8_t p = data[i];
            if (p.i == InkTransp)       noalpha = false;
            _trans[i] = d_8to24table[p.i];
        }

        if (alpha && noalpha)
            alpha = false;
    }
    else {
        if (s & 3)      Host_SysError("GL_Upload8: s&3");

        for (int i = 0; i < s; i += 4) {
            _trans[i + 0] = d_8to24table[data[i + 0].i];
            _trans[i + 1] = d_8to24table[data[i + 1].i];
            _trans[i + 2] = d_8to24table[data[i + 2].i];
            _trans[i + 3] = d_8to24table[data[i + 3].i];
        }
    }

    if (
        VID_Is8bit() &&
        !(alpha) &&
        (data != scrap_texels[0])
        )       GL_Upload8_EXT(data, width, height, mipmap, alpha);
    else        GL_Upload32(_trans, width, height, mipmap, alpha);

    return;
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture(
    cString identifier,
    int width, int height,
    qColor8_p data,
    bool mipmap, bool alpha
) {
    glTexture_p glt;

    // see if the texture is allready present
    if (identifier[0]) {
        glt = gltextures;
        for (int i = 0; i < numgltextures; i++, glt++)
            if (!strcmp(identifier, glt->identifier)) {
                if ((width != glt->width) ||
                    (height != glt->height)
                    )   Host_SysError("GL_LoadTexture: cache mismatch");
                return gltextures[i].texnum;
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
    if (!gl_mtexable)   return;

    qglSelectTextureSGIS(target);
    if (target == _oldTarget)   return;

    cnttextures[_oldTarget - TEXTURE0_SGIS] = currenttexture;
    currenttexture = cnttextures[target - TEXTURE0_SGIS];
    _oldTarget = target;
}
