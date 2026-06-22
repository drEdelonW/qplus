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
// gl_warp.c -- sky and water polygons
#include "qOpenGL.h"
#include "host.h"
#include "mathlib.h"
#include "z_hunk.h"
#include "q_tools.h"

extern cvar_t gl_subdivide_size;

int  skytexturenum;

int  solidskytexture;
int  alphaskytexture;
float speedscale;  // for top sky and bottom sky

mSurface_p warpface;


void BoundPoly(int numverts, float_p verts, vec3_p mins, vec3_p maxs) {
    *mins = (vec3_t){
        .x = 9999.0f,
        .y = 9999.0f,
        .z = 9999.0f
    };
    *maxs = (vec3_t){
        .x = -9999.0f,
        .y = -9999.0f,
        .z = -9999.0f
    };
    for (int i = 0; i < numverts; i++) {
        vec3_t v = ((vec3_p)verts)[i];
        for (int j = 0; j < VECT_DIM; j++) {
            CLAMP_LESS(maxs->v[j], v.v[j]);
            CLAMP_MORE(mins->v[j], v.v[j]);
        }
    }
}

void SubdividePolygon(int numverts, float_p verts) {
    vec3_t front[64];
    vec3_t back[64];
    float dist[64];

    if (numverts > 60)
        Host_SysError("numverts = %i", numverts);

    vec3_t mins, maxs;
    BoundPoly(numverts, verts, &mins, &maxs);

    for (int i = 0; i < VECT_DIM; i++) {
        float m = (mins.v[i] + maxs.v[i]) * 0.5;
        m = gl_subdivide_size.value * floor(m / gl_subdivide_size.value + 0.5);
        if ((maxs.v[i] - m) < 8.0f)      continue;
        if ((m - mins.v[i]) < 8.0f)      continue;

        // cut it
        float_p v = verts + i;
        int j = 0;
        for (; j < numverts; j++, v += 3)
            dist[j] = *v - m;

        // wrap cases
        dist[j] = dist[0];
        v -= i;
        VectorCopy(*(vec3_p)verts, (vec3_p)v);

        int f = 0;
        int b = 0;
        v = verts;
        for (int j = 0; j < numverts; j++, v += 3) {
            if (dist[j] >= 0.0f) {
                VectorCopy(*(vec3_p)v, &front[f++]);
            }
            if (dist[j] <= 0.0f) {
                VectorCopy(*(vec3_p)v, &back[b++]);
            }
            if ((dist[j] == 0.0f) ||
                (dist[j + 1] == 0.0f)
                )
                continue;

            if ((dist[j] > 0) != (dist[j + 1] > 0)) {
                // clip point
                float frac = dist[j] / (dist[j] - dist[j + 1]);
                for (int k = 0; k < VECT_DIM; k++)
                    front[f].v[k] = back[b].v[k] = v[k] + frac * /*>>>*/(v[3 + k] - v[k]);/*<<< this is NOT vector yet */
                f++;
                b++;
            }
        }

        SubdividePolygon(f, front[0].v);
        SubdividePolygon(b, back[0].v);
        return;
    }

    glpoly_p poly = Hunk_Alloc(sizeof(glpoly_t) + (numverts - 4) * sizeof(glVert_t));

    poly->next = warpface->polys;
    warpface->polys = poly;
    poly->numverts = numverts;
    for (int i = 0; i < numverts; i++, verts += 3) {
        VectorCopy(*(vec3_p)verts, &poly->verts[i].v);
        float s = DotProduct(*(vec3_p)verts, *(vec3_p)warpface->texinfo->vecs[0]);
        float t = DotProduct(*(vec3_p)verts, *(vec3_p)warpface->texinfo->vecs[1]);
        poly->verts[i].tx.s = s;
        poly->verts[i].tx.t = t;
    }
}

/*
================
GL_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
void GL_SubdivideSurface(mSurface_p fa) {
    vec3_t  verts[64];

    warpface = fa;

    //
    // convert edges back to a normal polygon
    //
    int numverts = 0;
    for (int i = 0; i < fa->numedges; i++) {
        int lindex = _loadModel->surfedges[fa->firstedge + i];

        vec3_t vec =
            _loadModel->vertexes[
                (lindex > 0) ?
                    _loadModel->edges[lindex].v16[0] :
                    _loadModel->edges[-lindex].v16[1]
            ].position;

        VectorCopy(vec, &verts[numverts]);
        numverts++;
    }

    SubdividePolygon(numverts, (float_p)&verts[0]);
}

//=========================================================



// speed up sin calculations - Ed
float turbsin[] = {
    #include "gl_warp_sin.h"
};
#define TURBSCALE (256.0 / (2 * M_PI))

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
void EmitWaterPolys(mSurface_p fa) {
    for (glpoly_p p = fa->polys; p; p = p->next) {
        glBegin(GL_POLYGON); {
            for (int i = 0; i < p->numverts; i++) {
                glVert_t v = p->verts[i];
                float os = v.tx.s;
                float ot = v.tx.t;

                float s = os + turbsin[(int)((ot * 0.125 + realtime) * TURBSCALE) & 255];
                s *= (1.0 / 64);

                float t = ot + turbsin[(int)((os * 0.125 + realtime) * TURBSCALE) & 255];
                t *= (1.0 / 64);

                glTexCoord2f(s, t);     glVertex3fv(v.vf);
            }
        } glEnd();
    }
}




/*
=============
EmitSkyPolys
=============
*/
void EmitSkyPolys(mSurface_p fa) {
    for (glpoly_p p = fa->polys; p; p = p->next) {
        glBegin(GL_POLYGON); {
            for (int i = 0; i < p->numverts; i++) {
                glVert_t v = p->verts[i];
                vec3_t dir = VectorSubtract(v.v, r_origin);
                dir.z *= 3; // flatten the sphere

                float length = (6 * 63) / Length(dir);

                dir.x *= length;
                dir.y *= length;

                float s = (speedscale + dir.x) * (1.0 / 128);
                float t = (speedscale + dir.y) * (1.0 / 128);

                glTexCoord2f(s, t);     glVertex3fv(v.vf);
            }
        } glEnd();
    }
}

/*
===============
EmitBothSkyLayers

Does a sky warp on the pre-fragmented glpoly_t chain
This will be called for brushmodels, the world
will have them chained together.
===============
*/
void EmitBothSkyLayers(mSurface_p fa) {
    GL_DisableMultitexture();

    GL_Bind(solidskytexture);
    speedscale = realtime * 8;
    speedscale -= (int)speedscale & ~127;

    EmitSkyPolys(fa);

    glEnable(GL_BLEND); {
        GL_Bind(alphaskytexture);
        speedscale = realtime * 16;
        speedscale -= (int)speedscale & ~127;

        EmitSkyPolys(fa);

    } glDisable(GL_BLEND);
}

#ifndef QUAKE2
/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain(mSurface_p s) {
    GL_DisableMultitexture();

    // used when gl_texsort is on
    GL_Bind(solidskytexture);
    speedscale = realtime * 8;
    speedscale -= (int)speedscale & ~127;

    for (mSurface_p fa = s; fa; fa = fa->texturechain)
        EmitSkyPolys(fa);

    glEnable(GL_BLEND); {
        GL_Bind(alphaskytexture);
        speedscale = realtime * 16;
        speedscale -= (int)speedscale & ~127;

        for (mSurface_p fa = s; fa; fa = fa->texturechain)
            EmitSkyPolys(fa);

    } glDisable(GL_BLEND);
}

#endif

/*
=================================================================

Quake 2 environment sky

=================================================================
*/

#ifdef QUAKE2


#define SKY_TEX  2000

/*
=================================================================

PCX Loading

=================================================================
*/

typedef struct {
    char manufacturer;
    char version;
    char encoding;
    char bits_per_pixel;
    uint16_t xmin, ymin, xmax, ymax;
    uint16_t hres, vres;
    uint8_t palette[48];
    char reserved;
    char color_planes;
    uint16_t bytes_per_line;
    uint16_t palette_type;
    char filler[58];
    uint8_t  data;   // unbounded
} pcx_t;
typedef pcx_t* pcx_p;

byte* pcx_rgb;

/*
============
LoadPCX
============
*/
void LoadPCX(FILE* f) {
    //
    // parse the PCX file
    //
    pcx_t pcxbuf; fread(&pcxbuf, 1, sizeof(pcxbuf), f);

    pcx_p pcx = &pcxbuf;

    if ((pcx->manufacturer != 0x0a) ||
        (pcx->version != 5) ||
        (pcx->encoding != 1) ||
        (pcx->bits_per_pixel != 8) ||
        (pcx->xmax >= 320) ||
        (pcx->ymax >= 256)
        ) {
        Con_Printf("Bad pcx file\n");
        return;
    }

    // seek to palette
    fseek(f, -768, SEEK_END);
    byte palette[768]; fread(palette, 1, 768, f);

    fseek(f, sizeof(pcxbuf) - 4, SEEK_SET);

    int count = (pcx->xmax + 1) * (pcx->ymax + 1);
    pcx_rgb = malloc(count * 4);

    for (int y = 0; y <= pcx->ymax; y++) {
        byte* pix = pcx_rgb + 4 * y * (pcx->xmax + 1);
        for (int x = 0; x <= pcx->ymax; ) {
            int dataByte = fgetc(f);

            int runLength;
            if ((dataByte & 0xC0) == 0xC0) {
                runLength = dataByte & 0x3F;
                dataByte = fgetc(f);
            }
            else
                runLength = 1;

            while (runLength-- > 0) {
                pix[0] = palette[dataByte * 3];
                pix[1] = palette[dataByte * 3 + 1];
                pix[2] = palette[dataByte * 3 + 2];
                pix[3] = 255;
                pix += 4;
                x++;
            }
        }
    }
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
    uint8_t  id_length;
    uint8_t colormap_type;
    uint8_t image_type;
    uint16_p colormap_index;
    uint16_p colormap_length;
    uint8_t colormap_size;
    uint16_p x_origin;
    uint16_p y_origin;
    uint16_p width;
    uint16_p height;
    uint8_t pixel_size;
    uint8_t attributes;
} TargaHeader;


TargaHeader  targa_header;
byte* targa_rgba;

int16_t fgetLittleShort(FILE* f) {
    byte b1 = fgetc(f);
    byte b2 = fgetc(f);

    return (int16_t)(b1 + b2 * 256);
}

int fgetLittleLong(FILE* f) {
    byte b1 = fgetc(f);
    byte b2 = fgetc(f);
    byte b3 = fgetc(f);
    byte b4 = fgetc(f);

    return b1 + (b2 << 8) + (b3 << 16) + (b4 << 24);
}


/*
=============
LoadTGA
=============
*/
void LoadTGA(FILE* fin) {
    byte* pixbuf;
    int row, column;

    targa_header.id_length = fgetc(fin);
    targa_header.colormap_type = fgetc(fin);
    targa_header.image_type = fgetc(fin);

    targa_header.colormap_index = fgetLittleShort(fin);
    targa_header.colormap_length = fgetLittleShort(fin);
    targa_header.colormap_size = fgetc(fin);
    targa_header.x_origin = fgetLittleShort(fin);
    targa_header.y_origin = fgetLittleShort(fin);
    targa_header.width = fgetLittleShort(fin);
    targa_header.height = fgetLittleShort(fin);
    targa_header.pixel_size = fgetc(fin);
    targa_header.attributes = fgetc(fin);

    if ((targa_header.image_type != 2) &&
        (targa_header.image_type != 10)
        )   Host_SysError("LoadTGA: Only type 2 and 10 targa RGB images supported\n");

    if ((targa_header.colormap_type != 0) ||
        (
            (targa_header.pixel_size != 32) &&
            (targa_header.pixel_size != 24))
        )   Host_SysError("Texture_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

    int columns = targa_header.width;
    int rows = targa_header.height;
    int numPixels = columns * rows;

    targa_rgba = malloc(numPixels * 4);

    if (targa_header.id_length != 0)
        fseek(fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment

    if (targa_header.image_type == 2) {  // Uncompressed, RGB images
        for (row = rows - 1; row >= 0; row--) {
            pixbuf = targa_rgba + row * columns * 4;
            for (column = 0; column < columns; column++) {
                uint8_t red, green, blue, alphabyte;
                switch (targa_header.pixel_size) {
                case 24: {
                    blue = getc(fin);
                    green = getc(fin);
                    red = getc(fin);
                    *pixbuf++ = red;
                    *pixbuf++ = green;
                    *pixbuf++ = blue;
                    *pixbuf++ = 255;
                } break;
                case 32: {
                    blue = getc(fin);
                    green = getc(fin);
                    red = getc(fin);
                    alphabyte = getc(fin);
                    *pixbuf++ = red;
                    *pixbuf++ = green;
                    *pixbuf++ = blue;
                    *pixbuf++ = alphabyte;
                } break;
                }
            }
        }
    }
    else if (targa_header.image_type == 10) {   // Runlength encoded RGB images
        uint8_t red, green, blue, alphabyte, packetHeader, packetSize, j;
        for (row = rows - 1; row >= 0; row--) {
            pixbuf = targa_rgba + row * columns * 4;
            for (column = 0; column < columns; ) {
                packetHeader = getc(fin);
                packetSize = 1 + (packetHeader & 0x7f);
                if (packetHeader & 0x80) {        // run-length packet
                    switch (targa_header.pixel_size) {
                    case 24: {
                        blue = getc(fin);
                        green = getc(fin);
                        red = getc(fin);
                        alphabyte = 255;
                    } break;
                    case 32: {
                        blue = getc(fin);
                        green = getc(fin);
                        red = getc(fin);
                        alphabyte = getc(fin);
                    } break;
                    }

                    for (j = 0;j < packetSize;j++) {
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = alphabyte;
                        column++;
                        if (column == columns) { // run spans across rows
                            column = 0;
                            if (row > 0)
                                row--;
                            else
                                goto breakOut;
                            pixbuf = targa_rgba + row * columns * 4;
                        }
                    }
                }
                else {                            // non run-length packet
                    for (j = 0;j < packetSize;j++) {
                        switch (targa_header.pixel_size) {
                        case 24: {
                            blue = getc(fin);
                            green = getc(fin);
                            red = getc(fin);
                            *pixbuf++ = red;
                            *pixbuf++ = green;
                            *pixbuf++ = blue;
                            *pixbuf++ = 255;
                        } break;
                        case 32: {
                            blue = getc(fin);
                            green = getc(fin);
                            red = getc(fin);
                            alphabyte = getc(fin);
                            *pixbuf++ = red;
                            *pixbuf++ = green;
                            *pixbuf++ = blue;
                            *pixbuf++ = alphabyte;
                        } break;
                        }
                        column++;
                        if (column == columns) { // pixel packet run spans across rows
                            column = 0;
                            if (row > 0)    row--;
                            else
                                goto breakOut;
                            pixbuf = targa_rgba + row * columns * 4;
                        }
                    }
                }
            }
        breakOut:;
        }
    }

    fclose(fin);
}

/*
==================
R_LoadSkys
==================
*/
cString suf[6] = {
    "rt",
    "bk",
    "lf",
    "ft",
    "up",
    "dn"
};

void R_LoadSkys() {

    for (int i = 0; i < 6; i++) {
        GL_Bind(SKY_TEX + i);
        char name[NAME_LENGTH];
        snprintf(name, sizeof(name), "gfx/env/bkgtst%s.tga", suf[i]);
        FILE* f; COM_FOpenFile(name, &f);
        if (!f) {
            Con_Printf("Couldn't load %s\n", name);
            continue;
        }
        LoadTGA(f);
        //  LoadPCX (f);

        glTexImage2D(
            GL_TEXTURE_2D,
            0, gl_solid_format,
            256, 256,
            0, GL_RGBA,
            GL_UNSIGNED_BYTE,
#if 1
            targa_rgba
#else
            pcx_rgb
#endif
        );


        free(targa_rgba);
        //  free (pcx_rgb);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}


vec3_t skyclip[6] = {
    {1, 1, 0},
    {1, -1, 0},
    {0, -1, 1},
    {0, 1, 1},
    {1, 0, 1},
    {-1, 0, 1}
};
int c_sky;

// 1 = s, 2 = t, 3 = 2048
int st_to_vec[6][3] = {
    {3, -1, 2},
    {-3, 1, 2},

    {1, 3, 2},
    {-1, -3, 2},

    {-2, -1, 3},  // 0 degrees yaw, look straight up
    {2, -1, -3}  // look straight down

    // {-1, 2, 3},
    // {1, 2, -3}
};

// s = [0]/[2], t = [1]/[2]
int vec_to_st[6][3] = {
    {-2, 3, 1},
    {2, 3, -1},

    {1, 3, 2},
    {-1, 3, -2},

    {-2, -1, 3},
    {-2, 1, -3}

    // {-1, 2, 3},
    // {1, 2, -3}
};

float skymins[2][6], skymaxs[2][6];

void DrawSkyPolygon(int nump, vec3_t vecs) {
    c_sky++;
#if 0
    glBegin(GL_POLYGON); {
        for (int i = 0; i < nump; i++, vecs += 3) {
            v = VectorAdd(vecs, r_origin, );
            glVertex3fv(v);
        }
    } glEnd();
    return;
#endif
    // decide which face it maps to
    vec3_t v;   VectorCopy(vec3_origin, &v);
    float_p vp = vecs;
    for (int i = 0; i < nump; i++, vp += 3) {
        v = VectorAdd(vp, v);
    }
    vec3_t av = {
        fabs(v.x),
        fabs(v.y),
        fabs(v.z)
    };
    int  axis;
    if (
        (av[0] > av[1]) &&
        (av[0] > av[2])
        ) {
        if (v.x < 0)   axis = 1;
        else            axis = 0;
    }
    else if (
        (av[1] > av[2]) &&
        (av[1] > av[0])
        ) {
        if (v.y < 0.0f)   axis = 3;
        else            axis = 2;
    }
    else {
        if (v.z < 0.0f)   axis = 5;
        else            axis = 4;
    }

    // project new texture coords
    for (int i = 0; i < nump; i++, vecs += 3) {
        float dv; {
            int j = vec_to_st[axis][2];
            if (j > 0)  dv = vecs.v[j - 1];
            else        dv = -vecs.v[-j - 1];
        }
        float s; {
            int j = vec_to_st[axis][0];
            if (j < 0)  s = -vecs.v[-j - 1] / dv;
            else        s = vecs.v[j - 1] / dv;
        }
        float t; {
            int j = vec_to_st[axis][1];
            if (j < 0)  t = -vecs.v[-j - 1] / dv;
            else        t = vecs.v[j - 1] / dv;
        }
        if (s < skymins[0][axis]) skymins[0][axis] = s;
        if (t < skymins[1][axis]) skymins[1][axis] = t;
        if (s > skymaxs[0][axis]) skymaxs[0][axis] = s;
        if (t > skymaxs[1][axis]) skymaxs[1][axis] = t;
    }
}

#define MAX_CLIP_VERTS 64
void ClipSkyPolygon(int nump, vec3_t vecs, int stage) {
    bool front, back;
    float dists[MAX_CLIP_VERTS];
    Side_t sides[MAX_CLIP_VERTS];

    if (nump > MAX_CLIP_VERTS - 2)
        Host_SysError("ClipSkyPolygon: MAX_CLIP_VERTS");

    if (stage == 6) { // fully clipped, so draw it
        DrawSkyPolygon(nump, vecs);
        return;
    }

    front = back = false;
    float_p norm = skyclip[stage];
    int i = 0;
    for (float_p v = vecs; i < nump; i++, v += 3) {
        float d = DotProduct(v, norm);
        if (d > ON_EPSILON) {
            front = true;
            sides[i] = SIDE_FRONT;
        }
        else if (d < ON_EPSILON) {
            back = true;
            sides[i] = SIDE_BACK;
        }
        else
            sides[i] = SIDE_ON;
        dists[i] = d;
    }

    if (!front || !back) { // not clipped
        ClipSkyPolygon(nump, vecs, stage + 1);
        return;
    }

    // clip it
    sides[i] = sides[0];
    dists[i] = dists[0];
    VectorCopy(vecs, (vecs + (i * 3)));
    int newc[2] = { 0, 0 };

    vec3_t newv[2][MAX_CLIP_VERTS];

    float_p v;
    for (int i = 0, v = vecs; i < nump; i++, v += 3) {
        switch (sides[i]) {
        case SIDE_FRONT: {
            VectorCopy(v, newv[0][newc[0]]);    newc[0]++;
        } break;
        case SIDE_BACK: {
            VectorCopy(v, newv[1][newc[1]]);    newc[1]++;
        } break;
        case SIDE_ON: {
            VectorCopy(v, newv[0][newc[0]]);    newc[0]++;
            VectorCopy(v, newv[1][newc[1]]);    newc[1]++;
        } break;
        }

        if ((sides[i] == SIDE_ON) ||
            (sides[i + 1] == SIDE_ON) ||
            (sides[i + 1] == sides[i])
            )
            continue;

        float d = dists[i] / (dists[i] - dists[i + 1]);
        for (int j = 0; j < 3; j++) {
            float e = v[j] + d * (v[j + 3] - v[j]);
            newv[0][newc[0]][j] = e;
            newv[1][newc[1]][j] = e;
        }
        newc[0]++;
        newc[1]++;
    }

    // continue
    ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
    ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain(mSurface_p s) {
    c_sky = 0;
    GL_Bind(solidskytexture);

    // calculate vertex values for sky box

    for (mSurface_p fa = s; fa; fa = fa->texturechain)
        for (glpoly_p p = fa->polys; p; p = p->next) {
            vec3_t verts[MAX_CLIP_VERTS];
            for (int i = 0; i < p->numverts; i++) {
                verts[i] = VectorSubtract(p->verts[i], r_origin);
            }
            ClipSkyPolygon(p->numverts, verts[0], 0);
        }
}


/*
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox() {
    for (int i = 0; i < 6; i++) {
        skymins[0][i] = skymins[1][i] = 9999;
        skymaxs[0][i] = skymaxs[1][i] = -9999;
    }
}


void MakeSkyVec(float s, float t, int axis) {
    vec3_t v;
    vec3_t b = {
        .x = s * 2048,
        .y = t * 2048,
        .z = 2048
    };

    for (int j = 0; j < VECT_DIM; j++) {
        int k = st_to_vec[axis][j];
        if (k < 0)  v.v[j] = -b.v[-k - 1];
        else        v.v[j] = b.v[k - 1];
        v.v[j] += r_origin[j];
    }

    // avoid bilerp seam
    s = (s + 1) * 0.5f;
    t = (t + 1) * 0.5f;

    if (s < 1.0f / 512)         s = 1.0f / 512;
    else if (s > 511.0f / 512)  s = 511.0f / 512;

    if (t < 1.0f / 512)         t = 1.0f / 512;
    else if (t > 511.0f / 512)  t = 511.0f / 512;

    t = 1.0f - t;
    glTexCoord2f(s, t);     glVertex3fv(v);
}

/*
==============
R_DrawSkyBox
==============
*/
int skytexorder[6] = { 0, 2, 1, 3, 4, 5 };
void R_DrawSkyBox() {
#if 0
    glEnable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1, 1, 1, 0.5);
    glDisable(GL_DEPTH_TEST);
#endif
    for (int i = 0; i < 6; i++) {
        if ((skymins[0][i] >= skymaxs[0][i]) ||
            (skymins[1][i] >= skymaxs[1][i])
            )
            continue;

        GL_Bind(SKY_TEX + skytexorder[i]);
#if 0
        skymins[0][i] = -1;
        skymins[1][i] = -1;
        skymaxs[0][i] = 1;
        skymaxs[1][i] = 1;
#endif
        glBegin(GL_QUADS); {
            MakeSkyVec(skymins[0][i], skymins[1][i], i);
            MakeSkyVec(skymins[0][i], skymaxs[1][i], i);
            MakeSkyVec(skymaxs[0][i], skymaxs[1][i], i);
            MakeSkyVec(skymaxs[0][i], skymins[1][i], i);
        } glEnd();
    }
#if 0
    glDisable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1, 1, 1, 0.5);
    glEnable(GL_DEPTH_TEST);
#endif
}


#endif

//===============================================================

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky(Texture_p mt) {

    uint32_t trans[128 * 128];

    byte* src = (byte*)mt + mt->offsets[0];

    // make an average value for the back to avoid
    // a fringe on the top level

    int r, g, b;
    r = g = b = 0;
    for (int i = 0; i < 128; i++)
        for (int j = 0; j < 128; j++) {
            int p = src[i * 256 + j + 128];
            uint32_p rgba = &d_8to24table[p];
            trans[(i * 128) + j] = *rgba;
            r += ((byte*)rgba)[0];
            g += ((byte*)rgba)[1];
            b += ((byte*)rgba)[2];
        }

    uint32_t transpix;
    ((byte*)&transpix)[0] = r / (128 * 128);
    ((byte*)&transpix)[1] = g / (128 * 128);
    ((byte*)&transpix)[2] = b / (128 * 128);
    ((byte*)&transpix)[3] = 0;


    if (!solidskytexture)
        solidskytexture = texture_extension_number++;
    GL_Bind(solidskytexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0, gl_solid_format,
        128, 128,
        0, GL_RGBA,
        GL_UNSIGNED_BYTE,
        trans
    );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    for (int i = 0; i < 128; i++)
        for (int j = 0; j < 128; j++) {
            int p = src[i * 256 + j];
            if (p == 0)     trans[(i * 128) + j] = transpix;
            else            trans[(i * 128) + j] = d_8to24table[p];
        }

    if (!alphaskytexture)
        alphaskytexture = texture_extension_number++;
    GL_Bind(alphaskytexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0, gl_alpha_format,
        128, 128,
        0, GL_RGBA,
        GL_UNSIGNED_BYTE,
        trans
    );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

