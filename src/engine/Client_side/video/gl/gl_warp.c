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



int  skytexturenum;

int  solidskytexture;
int  alphaskytexture;
float speedscale;  // for top sky and bottom sky

mSurface_p warpface;


void BoundPoly(int numverts, float_p verts, vec3_t mins, vec3_t maxs) {
    mins[0] = mins[1] = mins[2] = 9999;
    maxs[0] = maxs[1] = maxs[2] = -9999;
    float_p v = verts;
    for (int i = 0; i < numverts; i++)
        for (int j = 0; j < 3; j++, v++) {
            if (*v < mins[j])
                mins[j] = *v;
            if (*v > maxs[j])
                maxs[j] = *v;
        }
}

void SubdividePolygon(int numverts, float_p verts) {
    vec3_t mins, maxs;
    vec3_t front[64], back[64];
    float dist[64];

    if (numverts > 60)
        Sys_Error("numverts = %i", numverts);

    BoundPoly(numverts, verts, mins, maxs);

    for (int i = 0; i < 3; i++) {
        float m = (mins[i] + maxs[i]) * 0.5;
        m = gl_subdivide_size.value * floor(m / gl_subdivide_size.value + 0.5);
        if ((maxs[i] - m) < 8)
            continue;
        if ((m - mins[i]) < 8)
            continue;

        // cut it
        float_p v = verts + i;
        int j = 0;
        for (; j < numverts; j++, v += 3)
            dist[j] = *v - m;

        // wrap cases
        dist[j] = dist[0];
        v -= i;
        VectorCopy(verts, v);

        int f = 0;
        int b = 0;
        v = verts;
        for (int j = 0; j < numverts; j++, v += 3) {
            if (dist[j] >= 0) {
                VectorCopy(v, front[f]);
                f++;
            }
            if (dist[j] <= 0) {
                VectorCopy(v, back[b]);
                b++;
            }
            if ((dist[j] == 0) || (dist[j + 1] == 0))
                continue;
            if ((dist[j] > 0) != (dist[j + 1] > 0)) {
                // clip point
                float frac = dist[j] / (dist[j] - dist[j + 1]);
                for (int k = 0; k < 3; k++)
                    front[f][k] = back[b][k] = v[k] + frac * (v[3 + k] - v[k]);
                f++;
                b++;
            }
        }

        SubdividePolygon(f, front[0]);
        SubdividePolygon(b, back[0]);
        return;
    }

    glpoly_p poly = Hunk_Alloc(sizeof(glpoly_t) + (numverts - 4) * VERTEXSIZE * sizeof(float));
    poly->next = warpface->polys;
    warpface->polys = poly;
    poly->numverts = numverts;
    for (int i = 0; i < numverts; i++, verts += 3) {
        VectorCopy(verts, poly->verts[i]);
        float s = DotProduct(verts, warpface->texinfo->vecs[0]);
        float t = DotProduct(verts, warpface->texinfo->vecs[1]);
        poly->verts[i][3] = s;
        poly->verts[i][4] = t;
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
    float_p vec;
    Texture_p t;

    warpface = fa;

    //
    // convert edges back to a normal polygon
    //
    int numverts = 0;
    for (int i = 0; i < fa->numedges; i++) {
        int lindex = loadmodel->surfedges[fa->firstedge + i];

        if (lindex > 0)
            vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
        else
            vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
        VectorCopy(vec, verts[numverts]);
        numverts++;
    }

    SubdividePolygon(numverts, verts[0]);
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
        glBegin(GL_POLYGON);
        float_p v = p->verts[0];
        for (int i = 0; i < p->numverts; i++, v += VERTEXSIZE) {
            float os = v[3];
            float ot = v[4];

            float s = os + turbsin[(int)((ot * 0.125 + realtime) * TURBSCALE) & 255];
            s *= (1.0 / 64);

            float t = ot + turbsin[(int)((os * 0.125 + realtime) * TURBSCALE) & 255];
            t *= (1.0 / 64);

            glTexCoord2f(s, t);
            glVertex3fv(v);
        }
        glEnd();
    }
}




/*
=============
EmitSkyPolys
=============
*/
void EmitSkyPolys(mSurface_p fa) {
    for (glpoly_p = fa->polys; p; p = p->next) {
        glBegin(GL_POLYGON);
        float_p v = p->verts[0];
        for (int i = 0; i < p->numverts; i++, v += VERTEXSIZE) {
            vec3_t dir; VectorSubtract(v, r_origin, dir);
            dir[2] *= 3; // flatten the sphere

            float length = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
            length = sqrt(length);
            length = 6 * 63 / length;

            dir[0] *= length;
            dir[1] *= length;

            float s = (speedscale + dir[0]) * (1.0 / 128);
            float t = (speedscale + dir[1]) * (1.0 / 128);

            glTexCoord2f(s, t);
            glVertex3fv(v);
        }
        glEnd();
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

    glEnable(GL_BLEND);
    GL_Bind(alphaskytexture);
    speedscale = realtime * 16;
    speedscale -= (int)speedscale & ~127;

    EmitSkyPolys(fa);

    glDisable(GL_BLEND);
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

    glEnable(GL_BLEND);
    GL_Bind(alphaskytexture);
    speedscale = realtime * 16;
    speedscale -= (int)speedscale & ~127;

    for (mSurface_p fa = s; fa; fa = fa->texturechain)
        EmitSkyPolys(fa);

    glDisable(GL_BLEND);
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
    uint8_t  id_length, colormap_type, image_type;
    uint16_p colormap_index, colormap_length;
    uint8_t colormap_size;
    uint16_p x_origin, y_origin, width, height;
    uint8_t pixel_size, attributes;
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
    int    columns, rows, numPixels;
    byte* pixbuf;
    int    row, column;

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

    if (targa_header.image_type != 2
        && targa_header.image_type != 10)
        Sys_Error("LoadTGA: Only type 2 and 10 targa RGB images supported\n");

    if (targa_header.colormap_type != 0
        || (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
        Sys_Error("Texture_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

    columns = targa_header.width;
    rows = targa_header.height;
    numPixels = columns * rows;

    targa_rgba = malloc(numPixels * 4);

    if (targa_header.id_length != 0)
        fseek(fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment

    if (targa_header.image_type == 2) {  // Uncompressed, RGB images
        for (row = rows - 1; row >= 0; row--) {
            pixbuf = targa_rgba + row * columns * 4;
            for (column = 0; column < columns; column++) {
                uint8_t red, green, blue, alphabyte;
                switch (targa_header.pixel_size) {
                case 24:

                    blue = getc(fin);
                    green = getc(fin);
                    red = getc(fin);
                    *pixbuf++ = red;
                    *pixbuf++ = green;
                    *pixbuf++ = blue;
                    *pixbuf++ = 255;
                    break;
                case 32:
                    blue = getc(fin);
                    green = getc(fin);
                    red = getc(fin);
                    alphabyte = getc(fin);
                    *pixbuf++ = red;
                    *pixbuf++ = green;
                    *pixbuf++ = blue;
                    *pixbuf++ = alphabyte;
                    break;
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
                    case 24:
                        blue = getc(fin);
                        green = getc(fin);
                        red = getc(fin);
                        alphabyte = 255;
                        break;
                    case 32:
                        blue = getc(fin);
                        green = getc(fin);
                        red = getc(fin);
                        alphabyte = getc(fin);
                        break;
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
                        case 24:
                            blue = getc(fin);
                            green = getc(fin);
                            red = getc(fin);
                            *pixbuf++ = red;
                            *pixbuf++ = green;
                            *pixbuf++ = blue;
                            *pixbuf++ = 255;
                            break;
                        case 32:
                            blue = getc(fin);
                            green = getc(fin);
                            red = getc(fin);
                            alphabyte = getc(fin);
                            *pixbuf++ = red;
                            *pixbuf++ = green;
                            *pixbuf++ = blue;
                            *pixbuf++ = alphabyte;
                            break;
                        }
                        column++;
                        if (column == columns) { // pixel packet run spans across rows
                            column = 0;
                            if (row > 0)
                                row--;
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
cString suf[6] = { "rt", "bk", "lf", "ft", "up", "dn" };
void R_LoadSkys(void) {

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

        glTexImage2D(GL_TEXTURE_2D, 0, gl_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, targa_rgba);
        //  glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pcx_rgb);

        free(targa_rgba);
        //  free (pcx_rgb);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}


vec3_t skyclip[6] = {
    {1,1,0},
    {1,-1,0},
    {0,-1,1},
    {0,1,1},
    {1,0,1},
    {-1,0,1}
};
int c_sky;

// 1 = s, 2 = t, 3 = 2048
int st_to_vec[6][3] = {
    {3,-1,2},
    {-3,1,2},

    {1,3,2},
    {-1,-3,2},

    {-2,-1,3},  // 0 degrees yaw, look straight up
    {2,-1,-3}  // look straight down

    // {-1,2,3},
    // {1,2,-3}
};

// s = [0]/[2], t = [1]/[2]
int vec_to_st[6][3] = {
    {-2,3,1},
    {2,3,-1},

    {1,3,2},
    {-1,3,-2},

    {-2,-1,3},
    {-2,1,-3}

    // {-1,2,3},
    // {1,2,-3}
};

float skymins[2][6], skymaxs[2][6];

void DrawSkyPolygon(int nump, vec3_t vecs) {
    c_sky++;
#if 0
    glBegin(GL_POLYGON);
    for (int i = 0; i < nump; i++, vecs += 3) {
        VectorAdd(vecs, r_origin, v);
        glVertex3fv(v);
    }
    glEnd();
    return;
#endif
    // decide which face it maps to
    vec3_t v;   VectorCopy(vec3_origin, v);
    float_p vp = vecs;
    for (int i = 0; i < nump; i++, vp += 3) {
        VectorAdd(vp, v, v);
    }
    vec3_t av = {
        fabs(v[0]),
        fabs(v[1]),
        fabs(v[2])
    };
    int  axis;
    if (av[0] > av[1] && av[0] > av[2]) {
        if (v[0] < 0)   axis = 1;
        else            axis = 0;
    }
    else if (av[1] > av[2] && av[1] > av[0]) {
        if (v[1] < 0)   axis = 3;
        else            axis = 2;
    }
    else {
        if (v[2] < 0)   axis = 5;
        else            axis = 4;
    }

    // project new texture coords
    for (int i = 0; i < nump; i++, vecs += 3) {
        float s, t, dv;
        {
            int j = vec_to_st[axis][2];
            if (j > 0)  dv = vecs[j - 1];
            else        dv = -vecs[-j - 1];
        }
        {
            int j = vec_to_st[axis][0];
            if (j < 0)      s = -vecs[-j - 1] / dv;
            else            s = vecs[j - 1] / dv;
        }
        {
            int j = vec_to_st[axis][1];
            if (j < 0)      t = -vecs[-j - 1] / dv;
            else            t = vecs[j - 1] / dv;
        }
        if (s < skymins[0][axis]) skymins[0][axis] = s;
        if (t < skymins[1][axis]) skymins[1][axis] = t;
        if (s > skymaxs[0][axis]) skymaxs[0][axis] = s;
        if (t > skymaxs[1][axis]) skymaxs[1][axis] = t;
    }
}

#define MAX_CLIP_VERTS 64
void ClipSkyPolygon(int nump, vec3_t vecs, int stage) {
    float_p norm;
    float_p v;
    bool front, back;
    float d, e;
    float dists[MAX_CLIP_VERTS];
    Side_t sides[MAX_CLIP_VERTS];
    vec3_t newv[2][MAX_CLIP_VERTS];
    int  newc[2];
    int  i, j;

    if (nump > MAX_CLIP_VERTS - 2)
        Sys_Error("ClipSkyPolygon: MAX_CLIP_VERTS");
    if (stage == 6) { // fully clipped, so draw it
        DrawSkyPolygon(nump, vecs);
        return;
    }

    front = back = false;
    norm = skyclip[stage];
    for (i = 0, v = vecs; i < nump; i++, v += 3) {
        d = DotProduct(v, norm);
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
    newc[0] = newc[1] = 0;

    for (i = 0, v = vecs; i < nump; i++, v += 3) {
        switch (sides[i]) {
        case SIDE_FRONT:
            VectorCopy(v, newv[0][newc[0]]);
            newc[0]++;
            break;
        case SIDE_BACK:
            VectorCopy(v, newv[1][newc[1]]);
            newc[1]++;
            break;
        case SIDE_ON:
            VectorCopy(v, newv[0][newc[0]]);
            newc[0]++;
            VectorCopy(v, newv[1][newc[1]]);
            newc[1]++;
            break;
        }

        if (sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
            continue;

        d = dists[i] / (dists[i] - dists[i + 1]);
        for (j = 0; j < 3; j++) {
            e = v[j] + d * (v[j + 3] - v[j]);
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

    for (mSurface_p fa = s; fa; fa = fa->texturechain) {
        for (glpoly_p p = fa->polys; p; p = p->next) {
            vec3_t verts[MAX_CLIP_VERTS];
            for (int i = 0; i < p->numverts; i++) {
                VectorSubtract(p->verts[i], r_origin, verts[i]);
            }
            ClipSkyPolygon(p->numverts, verts[0], 0);
        }
    }
}


/*
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox(void) {
    for (int i = 0; i < 6; i++) {
        skymins[0][i] = skymins[1][i] = 9999;
        skymaxs[0][i] = skymaxs[1][i] = -9999;
    }
}


void MakeSkyVec(float s, float t, int axis) {
    vec3_t v;
    vec3_t b = {
        s * 2048,
        t * 2048,
        2048
    };

    for (int j = 0; j < 3; j++) {
        int k = st_to_vec[axis][j];
        if (k < 0)      v[j] = -b[-k - 1];
        else            v[j] = b[k - 1];
        v[j] += r_origin[j];
    }

    // avoid bilerp seam
    s = (s + 1) * 0.5f;
    t = (t + 1) * 0.5f;

    if (s < 1.0f / 512)
        s = 1.0f / 512;
    else if (s > 511.0f / 512)
        s = 511.0f / 512;

    if (t < 1.0f / 512)
        t = 1.0f / 512;
    else if (t > 511.0f / 512)
        t = 511.0f / 512;

    t = 1.0f - t;
    glTexCoord2f(s, t);
    glVertex3fv(v);
}

/*
==============
R_DrawSkyBox
==============
*/
int skytexorder[6] = { 0,2,1,3,4,5 };
void R_DrawSkyBox(void) {
#if 0
    glEnable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1, 1, 1, 0.5);
    glDisable(GL_DEPTH_TEST);
#endif
    for (int i = 0; i < 6; i++) {
        if (skymins[0][i] >= skymaxs[0][i]
            || skymins[1][i] >= skymaxs[1][i])
            continue;

        GL_Bind(SKY_TEX + skytexorder[i]);
#if 0
        skymins[0][i] = -1;
        skymins[1][i] = -1;
        skymaxs[0][i] = 1;
        skymaxs[1][i] = 1;
#endif
        glBegin(GL_QUADS);
        MakeSkyVec(skymins[0][i], skymins[1][i], i);
        MakeSkyVec(skymins[0][i], skymaxs[1][i], i);
        MakeSkyVec(skymaxs[0][i], skymaxs[1][i], i);
        MakeSkyVec(skymaxs[0][i], skymins[1][i], i);
        glEnd();
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
    glTexImage2D(GL_TEXTURE_2D, 0, gl_solid_format, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
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
    glTexImage2D(GL_TEXTURE_2D, 0, gl_alpha_format, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

