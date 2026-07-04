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
// r_surf.c: surface-related refresh code
#include "types.h"
#include "qOpenGL.h"
#include "Surface.h"
#include "client.h"
#include "mathlib.h"
#include "host.h"
#include "model.h"
#include "LeafModel.h"
#include "common.h"
#include <string.h>
#include "z_hunk.h"

int skytexturenum;

#ifndef GL_RGBA4
#   define GL_RGBA4    0
#endif


static int _lightMapBytes;        // 1, 2, or 4
static int _lightMapTextures;
int texture_extension_number = 1;


fixed16_t blocklights[18 * 18];

#define BLOCK_WIDTH     128
#define BLOCK_HEIGHT    128

#define MAX_LIGHTMAPS    64
int     active_lightmaps;

typedef struct glRect_s {
    uint8_t l;
    uint8_t t;
    uint8_t w;
    uint8_t h;
} glRect_t;
typedef glRect_t* glRect_p;

glpoly_p    lightmap_polys[MAX_LIGHTMAPS];
bool        lightmap_modified[MAX_LIGHTMAPS];
glRect_t    lightmap_rectchange[MAX_LIGHTMAPS];

int allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
byte    lightmaps[4 * MAX_LIGHTMAPS * BLOCK_WIDTH * BLOCK_HEIGHT];

// For gl_texsort 0
mSurface_p skychain = NULL;
mSurface_p waterchain = NULL;

void R_RenderDynamicLightmaps(mSurface_p fa);

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights(mSurface_p surf) { // TODO: merge with SoftR function almoust the same
    for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
        if (!(surf->dlightbits & (1 << lnum)))
            continue;   // not lit by this light

        float dist = DotProduct(cl_dlights[lnum].origin, surf->plane->normal) - surf->plane->dist;
        float rad = cl_dlights[lnum].radius - fabs(dist);

        float minlight = cl_dlights[lnum].minlight;
        if (rad < minlight)
            continue;

        minlight = rad - minlight;

        vec3_t impact = VectorMA(
            cl_dlights[lnum].origin,
            -dist, surf->plane->normal
        );
        mTexInfo_p tex = surf->texinfo;
        fixed4_t ts = DotProduct(impact, tex->vecs[S_AX].vx) + tex->vecs[S_AX].offs - surf->texturemins[S_AX];
        fixed4_t tt = DotProduct(impact, tex->vecs[T_AX].vx) + tex->vecs[T_AX].offs - surf->texturemins[T_AX];
        int tmax = FIXED4_TO_INT(surf->extents[T_AX]) + 1;
        int smax = FIXED4_TO_INT(surf->extents[S_AX]) + 1;

        for (int t = 0; t < tmax; t++) {
            fixed4_t td = tt - INT_TO_FIXED4(t);
            if (td < 0)     td = -td;

            for (int s = 0; s < smax; s++) {
                fixed4_t sd = ts - INT_TO_FIXED4(s);
                if (sd < 0)     sd = -sd;

                float dist = (sd > td) ?
                    sd + HALF(td) : td + HALF(sd);

                if (dist < minlight)
                    blocklights[(t * smax) + s] += (rad - dist) * 256;
            }
        }
    }
}


/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap(mSurface_p surf, byte* dest, int stride) {// TODO: merge with GL function almoust the same
    surf->cached_dlight = (surf->dlightframe == r_framecount);

    int smax = FIXED4_TO_INT(surf->extents[S_AX]) + 1;
    int tmax = FIXED4_TO_INT(surf->extents[T_AX]) + 1;
    int size = smax * tmax;

    // set to full bright if no light data
    if ((r_fullbright.value) ||
        !(cl.worldmodel->lightdata)
        ) {
        for (int i = 0; i < size; i++)
            blocklights[i] = 255 * 256;
    }
    else {  // clear to no light
        for (int i = 0; i < size; i++)
            blocklights[i] = 0;

        {   // add all the lightmaps
            uint8_p lightmap = surf->samples;
            if (lightmap)
                for (int maps = 0; (maps < MAXLIGHTMAPS) && (surf->styles[maps] != 255); maps++) {
                    fixed8_t scale = d_lightstylevalue[surf->styles[maps]];
                    surf->cached_light[maps] = scale;    // 8.8 fraction
                    for (int i = 0; i < size; i++)
                        blocklights[i] += lightmap[i] * scale;
                    lightmap += size;    // skip to next lightmap
                }
        }
        // add all the dynamic lights
        if (surf->dlightframe == r_framecount)
            R_AddDynamicLights(surf);

        // bound, invert, and shift
    }

    switch (gl_lightmap_format) {
    case GL_RGBA: {
        stride -= QUAD(smax);
        fixed16_p bl = blocklights;
        for (int i = 0; i < tmax; i++, dest += stride)
            for (int j = 0; j < smax; j++) {
                int t = DIV128(*bl++);
                if (t > 0xFF)    t = 0xFF;
                dest[3] = 0xFF - t;
                dest += 4;
            }
    } break;
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_INTENSITY: {
        fixed16_p bl = blocklights;
        for (int i = 0; i < tmax; i++, dest += stride)
            for (int j = 0; j < smax; j++) {
                int t = DIV128(*bl++);
                if (t > 0xFF)    t = 0xFF;
                dest[j] = 0xFF - t;
            }
    } break;
    default: { Host_SysError("Bad lightmap format"); } break;
    }
}




/*
=============================================================

    BRUSH MODELS

=============================================================
*/



void DrawGLWaterPoly(glpoly_p p);
void DrawGLWaterPolyLightmap(glpoly_p p);

lpMTexFUNC qglMTexCoord2fSGIS = NULL;
lpSelTexFUNC qglSelectTextureSGIS = NULL;

bool mtexenabled = false;

void GL_SelectTexture(GLenum target);

void GL_DisableMultitexture() {
    if (mtexenabled) {
        glDisable(GL_TEXTURE_2D);
        GL_SelectTexture(TEXTURE0_SGIS);
        mtexenabled = false;
    }
}

void GL_EnableMultitexture() {
    if (gl_mtexable) {
        GL_SelectTexture(TEXTURE1_SGIS);
        glEnable(GL_TEXTURE_2D);
        mtexenabled = true;
    }
}

/*
================
R_DrawSequentialPoly

Systems that have fast state and texture changes can
just do everything as it passes with no need to sort
================
*/
#if 0
void R_DrawSequentialPoly(mSurface_p s) {
    //
    // normal lightmaped poly
    //

    if (
        !(s->flags &
            (
                SURF_DRAWSKY |
                SURF_DRAWTURB |
                SURF_UNDERWATER)
            )
        ) {
        glpoly_p p = s->polys;

        Texture_p t = R_TextureAnimation(s->texinfo->texture);
        GL_Bind(t->gl_texturenum);
        glBegin(GL_POLYGON); {
            for (int i = 0; i < p->numverts; i++) {
                float_t v = p->verts[i];
                glTexCoord2f(v.vf[3], v.vf[4]);   glVertex3fv(v.vf);
            }
        } glEnd();

        GL_Bind(_lightMapTextures + s->lightmaptexturenum);
        glEnable(GL_BLEND); {
            glBegin(GL_POLYGON); {
                for (int i = 0; i < p->numverts; i++) {
                    float_t v = p->verts[i];
                    glTexCoord2f(v.vf[5], v.vf[6]);   glVertex3fv(v.vf);
                }
            } glEnd();

        } glDisable(GL_BLEND);

        return;
    }

    //
    // subdivided water surface warp
    //
    if (s->flags & SURF_DRAWTURB) {
        GL_Bind(s->texinfo->texture->gl_texturenum);
        EmitWaterPolys(s);
        return;
    }

    //
    // subdivided sky warp
    //
    if (s->flags & SURF_DRAWSKY) {
        GL_Bind(solidskytexture);
        speedscale = GetRealTime() * 8.0f;
        speedscale -= (int)speedscale;

        EmitSkyPolys(s);

        glEnable(GL_BLEND); {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            GL_Bind(alphaskytexture);
            speedscale = GetRealTime() * 16.0f;
            speedscale -= (int)speedscale;
            EmitSkyPolys(s);
            if (gl_lightmap_format == GL_LUMINANCE)
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

        } glDisable(GL_BLEND);
    }

    //
    // underwater warped with lightmap
    //
    {
        glpoly_p p = s->polys;

        Texture_p t = R_TextureAnimation(s->texinfo->texture);
        GL_Bind(t->gl_texturenum);
        DrawGLWaterPoly(p);

        GL_Bind(_lightMapTextures + s->lightmaptexturenum);
        glEnable(GL_BLEND); {
            DrawGLWaterPolyLightmap(p);
        } glDisable(GL_BLEND);
    }
}
#else
void R_DrawSequentialPoly(mSurface_p s) {
    //
    // normal lightmaped poly
    //

    if (
        !(s->flags &
            (
                SURF_DRAWSKY |
                SURF_DRAWTURB |
                SURF_UNDERWATER
                )
            )
        ) {
        R_RenderDynamicLightmaps(s);
        if (gl_mtexable) {
            glpoly_p p = s->polys;

            Texture_p t = R_TextureAnimation(s->texinfo->texture);
            // Binds world to texture env 0
            GL_SelectTexture(TEXTURE0_SGIS);
            GL_Bind(t->gl_texturenum);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            // Binds lightmap to texenv 1
            GL_EnableMultitexture(); // Same as SelectTexture (TEXTURE1)
            GL_Bind(_lightMapTextures + s->lightmaptexturenum);
            int i = s->lightmaptexturenum;
            if (lightmap_modified[i]
                ) {
                lightmap_modified[i] = false;
                glRect_p theRect = &lightmap_rectchange[i];
                glTexSubImage2D(
                    GL_TEXTURE_2D, 0,
                    0, theRect->t,
                    BLOCK_WIDTH, theRect->h, gl_lightmap_format, GL_UNSIGNED_BYTE,
                    lightmaps + (i * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * _lightMapBytes
                );
                *theRect = (glRect_t){
                    .l = BLOCK_WIDTH,
                    .t = BLOCK_HEIGHT
                };
            }
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            glBegin(GL_POLYGON); {
                for (int i = 0; i < p->numverts; i++) {
                    glVert_t v = p->verts[i];
                    qglMTexCoord2fSGIS(TEXTURE0_SGIS, v.vf[3], v.vf[4]);
                    qglMTexCoord2fSGIS(TEXTURE1_SGIS, v.vf[5], v.vf[6]);
                    glVertex3fv(v.vf);
                }
            } glEnd();
            return;
        }
        else {
            glpoly_p p = s->polys;

            Texture_p t = R_TextureAnimation(s->texinfo->texture);
            GL_Bind(t->gl_texturenum);
            glBegin(GL_POLYGON); {
                for (int i = 0; i < p->numverts; i++) {
                    glVert_t v = p->verts[i];
                    glTexCoord2f(v.vf[3], v.vf[4]);   glVertex3fv(v.vf);
                }
            } glEnd();

            GL_Bind(_lightMapTextures + s->lightmaptexturenum);
            glEnable(GL_BLEND);
            glBegin(GL_POLYGON); {
                for (int i = 0; i < p->numverts; i++) {
                    glVert_t v = p->verts[i];
                    glTexCoord2f(v.vf[5], v.vf[6]);   glVertex3fv(v.vf);
                }
            } glEnd();

            glDisable(GL_BLEND);
        }

        return;
    }

    //
    // subdivided water surface warp
    //
    if (s->flags & SURF_DRAWTURB) {
        GL_DisableMultitexture();
        GL_Bind(s->texinfo->texture->gl_texturenum);
        EmitWaterPolys(s);
        return;
    }

    //
    // subdivided sky warp
    //
    if (s->flags & SURF_DRAWSKY) {
        GL_DisableMultitexture();
        GL_Bind(solidskytexture);
        speedscale = GetRealTime() * 8;
        speedscale -= (int)speedscale & ~127;

        EmitSkyPolys(s);

        glEnable(GL_BLEND);
        GL_Bind(alphaskytexture);
        speedscale = GetRealTime() * 16.0f;
        speedscale -= (int)speedscale & ~127;
        EmitSkyPolys(s);

        glDisable(GL_BLEND);
        return;
    }

    //
    // underwater warped with lightmap
    //
    R_RenderDynamicLightmaps(s);
    if (gl_mtexable) {
        glpoly_p p = s->polys;

        Texture_p t = R_TextureAnimation(s->texinfo->texture);
        GL_SelectTexture(TEXTURE0_SGIS);
        GL_Bind(t->gl_texturenum);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        GL_EnableMultitexture();
        GL_Bind(_lightMapTextures + s->lightmaptexturenum);
        int i = s->lightmaptexturenum;
        if (lightmap_modified[i]) {
            lightmap_modified[i] = false;
            glRect_p theRect = &lightmap_rectchange[i];
            glTexSubImage2D(
                GL_TEXTURE_2D, 0, 0, theRect->t,
                BLOCK_WIDTH, theRect->h, gl_lightmap_format, GL_UNSIGNED_BYTE,
                lightmaps + (i * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * _lightMapBytes
            );
            *theRect = (glRect_t){
                .l = BLOCK_WIDTH,
                .t = BLOCK_HEIGHT
            };
        }
        sRealTime_t rt = GetRealTime();
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
        glBegin(GL_TRIANGLE_FAN); {
            for (int i = 0; i < p->numverts; i++) {
                glVert_t v = p->verts[i];
                vec3_t tv = VectorAddVal(VectorScale(v.v, 0.05f), rt);
                float t = sinf(tv.z) * 8.f;
                qglMTexCoord2fSGIS(TEXTURE0_SGIS, v.vf[3], v.vf[4]);
                qglMTexCoord2fSGIS(TEXTURE1_SGIS, v.vf[5], v.vf[6]);

                vec3_t nv = {
                    .x = v.v.x + sinf(tv.y) * t,
                    .y = v.v.y + sinf(tv.x) * t,
                    .z = v.v.z
                };
                glVertex3fv(nv.v);
            }
        } glEnd();

    }
    else {
        glpoly_p p = s->polys;

        Texture_p t = R_TextureAnimation(s->texinfo->texture);
        GL_Bind(t->gl_texturenum);
        DrawGLWaterPoly(p);

        GL_Bind(_lightMapTextures + s->lightmaptexturenum);
        glEnable(GL_BLEND); {
            DrawGLWaterPolyLightmap(p);
        } glDisable(GL_BLEND);
    }
}
#endif


/*
================
DrawGLWaterPoly

Warp the vertex coordinates
================
*/
void DrawGLWaterPoly(glpoly_p p) {
    sRealTime_t rt = GetRealTime();
    GL_DisableMultitexture();

    glBegin(GL_TRIANGLE_FAN); {
        for (int i = 0; i < p->numverts; i++) {
            glVert_t v = p->verts[i];
            vec3_t tv = VectorAddVal(VectorScale(v.v, 0.05f), rt);
            float t = sinf(tv.z) * 8.f;
            glTexCoord2f(v.vf[3], v.vf[4]);

            vec3_t nv = {
                .x = v.v.x + sinf(tv.y) * t,
                .y = v.v.y + sinf(tv.x) * t,
                .z = v.v.z
            };

            glVertex3fv(nv.v);
        }
    } glEnd();
}

void DrawGLWaterPolyLightmap(glpoly_p p) {
    sRealTime_t rt = GetRealTime();
    GL_DisableMultitexture();

    glBegin(GL_TRIANGLE_FAN); {
        for (int i = 0; i < p->numverts; i++) {
            glVert_t v = p->verts[i];
            vec3_t tv = VectorAddVal(VectorScale(v.v, 0.05f), rt);
            float t = sinf(tv.z) * 8.f;
            glTexCoord2f(v.vf[5], v.vf[6]);

            vec3_t nv = {
                .x = v.v.x + sinf(tv.y) * t,
                .y = v.v.y + sinf(tv.x) * t,
                .z = v.v.z
            };

            glVertex3fv(nv.v);
        }
    } glEnd();
}

/*
================
DrawGLPoly
================
*/
void DrawGLPoly(glpoly_p p) {
    glBegin(GL_POLYGON); {
        for (int i = 0; i < p->numverts; i++) {
            glVert_t v = p->verts[i];
            glTexCoord2f(v.vf[3], v.vf[4]);   glVertex3fv(v.vf);
        }
    } glEnd();
}


/*
================
R_BlendLightmaps
================
*/
void R_BlendLightmaps() {
    if (r_fullbright.value) return;
    if (!gl_texsort.value)  return;

    glDepthMask(0);        // don't bother writing Z

    if (gl_lightmap_format == GL_LUMINANCE)
        glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    else if (gl_lightmap_format == GL_INTENSITY) {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(0, 0, 0, 1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    if (r_lightmap.value)   // kind of fix light map
        glEnable(GL_BLEND);

    for (int i = 0; i < MAX_LIGHTMAPS; i++) {
        glpoly_p p = lightmap_polys[i];
        if (!p)     continue;

        GL_Bind(_lightMapTextures + i);
        if (lightmap_modified[i]) {
            lightmap_modified[i] = false;
            glRect_p theRect = &lightmap_rectchange[i];
#if 0
            glTexImage2D(
                GL_TEXTURE_2D, 0, _lightMapBytes,
                BLOCK_WIDTH, BLOCK_HEIGHT,
                0, gl_lightmap_format,
                GL_UNSIGNED_BYTE,
                lightmaps + i * BLOCK_WIDTH * BLOCK_HEIGHT * _lightMapBytes
            );
            glTexImage2D(
                GL_TEXTURE_2D, 0, _lightMapBytes,
                BLOCK_WIDTH, theRect->h,
                0, gl_lightmap_format,
                  GL_UNSIGNED_BYTE,
                lightmaps + (i * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * _lightMapBytes
            );
#else
            glTexSubImage2D(
                GL_TEXTURE_2D, 0,
                0, theRect->t,
                BLOCK_WIDTH, theRect->h,
                gl_lightmap_format, GL_UNSIGNED_BYTE,
                lightmaps + (i * BLOCK_HEIGHT + theRect->t) * BLOCK_WIDTH * _lightMapBytes
            );
#endif
            *theRect = (glRect_t){
                .l = BLOCK_WIDTH,
                .t = BLOCK_HEIGHT,
            };
        }
        for (; p; p = p->chain) {
            if (p->flags & SURF_UNDERWATER)
                DrawGLWaterPolyLightmap(p);
            else {
                glBegin(GL_POLYGON); {
                    for (int j = 0; j < p->numverts; j++) {
                        glVert_t v = p->verts[j];
                        glTexCoord2f(v.vf[5], v.vf[6]);   glVertex3fv(v.vf);
                    }
                } glEnd();
            }
        }
    }

    glDisable(GL_BLEND);
    if (gl_lightmap_format == GL_LUMINANCE)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    else if (gl_lightmap_format == GL_INTENSITY) {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glColor4f(1.f, 1.f, 1.f, 1.f);
    }

    glDepthMask(1);        // back to normal Z buffering
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly(mSurface_p fa) {
    c_brush_polys++;

    if (fa->flags & SURF_DRAWSKY) {    // warp texture, no lightmaps
        EmitBothSkyLayers(fa);
        return;
    }

    Texture_p t = R_TextureAnimation(fa->texinfo->texture);
    GL_Bind(t->gl_texturenum);

    if (fa->flags & SURF_DRAWTURB) {    // warp texture, no lightmaps
        EmitWaterPolys(fa);
        return;
    }

    if (fa->flags & SURF_UNDERWATER)    DrawGLWaterPoly(fa->polys);
    else                                DrawGLPoly(fa->polys);

    // add the poly to the proper lightmap chain
    {   // TODO: seems like simular. chech OTHER_CASE search
        fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
        lightmap_polys[fa->lightmaptexturenum] = fa->polys;

        // check for lightmap modification
        for (int maps = 0; (maps < MAXLIGHTMAPS) && (fa->styles[maps] != 255); maps++)
            if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
                goto dynamic;

        if ((fa->dlightframe == r_framecount) ||    // dynamic this frame
            (fa->cached_dlight)                     // dynamic previously
            ) {
        dynamic:
            if (r_dynamic.value) {
                lightmap_modified[fa->lightmaptexturenum] = true;
                glRect_p theRect = &lightmap_rectchange[fa->lightmaptexturenum];
                if (fa->light_t < theRect->t) {
                    if (theRect->h)
                        theRect->h += theRect->t - fa->light_t;
                    theRect->t = fa->light_t;
                }
                if (fa->light_s < theRect->l) {
                    if (theRect->w)
                        theRect->w += theRect->l - fa->light_s;
                    theRect->l = fa->light_s;
                }
                int smax = FIXED4_TO_INT(fa->extents[S_AX]) + 1;
                int tmax = FIXED4_TO_INT(fa->extents[T_AX]) + 1;
                if ((theRect->w + theRect->l) < (fa->light_s + smax))   theRect->w = (fa->light_s - theRect->l) + smax;
                if ((theRect->h + theRect->t) < (fa->light_t + tmax))   theRect->h = (fa->light_t - theRect->t) + tmax;

                byte* base = lightmaps + fa->lightmaptexturenum * _lightMapBytes * BLOCK_WIDTH * BLOCK_HEIGHT;
                base += fa->light_t * BLOCK_WIDTH * _lightMapBytes + fa->light_s * _lightMapBytes;
                R_BuildLightMap(fa, base, BLOCK_WIDTH * _lightMapBytes);
            }
        }
    }
}

/*
================
R_RenderDynamicLightmaps
Multitexture
================
*/
void R_RenderDynamicLightmaps(mSurface_p fa) {
    c_brush_polys++;

    if (fa->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
        return;

    {   // TODO: seems like simular. chech OTHER_CASE search
        fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
        lightmap_polys[fa->lightmaptexturenum] = fa->polys;

        // check for lightmap modification
        for (int maps = 0; (maps < MAXLIGHTMAPS) && (fa->styles[maps] != 255); maps++)
            if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
                goto dynamic;

        if ((fa->dlightframe == r_framecount) ||    // dynamic this frame
            (fa->cached_dlight)                     // dynamic previously
            ) {
        dynamic:
            if (r_dynamic.value) {
                lightmap_modified[fa->lightmaptexturenum] = true;
                glRect_p theRect = &lightmap_rectchange[fa->lightmaptexturenum];
                if (fa->light_t < theRect->t) {
                    if (theRect->h)
                        theRect->h += theRect->t - fa->light_t;
                    theRect->t = fa->light_t;
                }
                if (fa->light_s < theRect->l) {
                    if (theRect->w)
                        theRect->w += theRect->l - fa->light_s;
                    theRect->l = fa->light_s;
                }
                int smax = FIXED4_TO_INT(fa->extents[S_AX]) + 1;
                int tmax = FIXED4_TO_INT(fa->extents[T_AX]) + 1;
                if ((theRect->w + theRect->l) < (fa->light_s + smax))   theRect->w = (fa->light_s - theRect->l) + smax;
                if ((theRect->h + theRect->t) < (fa->light_t + tmax))   theRect->h = (fa->light_t - theRect->t) + tmax;

                byte* base = lightmaps + fa->lightmaptexturenum * _lightMapBytes * BLOCK_WIDTH * BLOCK_HEIGHT;
                base += fa->light_t * BLOCK_WIDTH * _lightMapBytes + fa->light_s * _lightMapBytes;
                R_BuildLightMap(fa, base, BLOCK_WIDTH * _lightMapBytes);
            }
        }
    }
}

/*
================
R_MirrorChain
================
*/
void R_MirrorChain(mSurface_p s) {
    if (mirror)
        return;

    mirror = true;
    mirror_plane = s->plane;
}


#if 0
/*
================
R_DrawWaterSurfaces
================
*/
void R_DrawWaterSurfaces() {
    if (r_wateralpha.value == 1.0)
        return;

    //
    // go back to the world matrix
    //
    glLoadMatrixf(r_world_matrix);

    glEnable(GL_BLEND);
    glColor4f(1, 1, 1, r_wateralpha.value);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    for (int i = 0; i < cl.worldmodel->numtextures; i++) {
        Texture_p t = cl.worldmodel->textures[i];
        if (!t)     continue;

        mSurface_p s = t->texturechain;
        if ((!s) ||
            (!(s->flags & SURF_DRAWTURB))
            )           continue;

        // set modulate mode explicitly
        GL_Bind(t->gl_texturenum);

        for (; s; s = s->texturechain)
            R_RenderBrushPoly(s);

        t->texturechain = NULL;
    }

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glColor4f(1, 1, 1, 1);
    glDisable(GL_BLEND);
}
#else
/*
================
R_DrawWaterSurfaces
================
*/
void R_DrawWaterSurfaces() {
    if (r_wateralpha.value == 1.0 && gl_texsort.value)
        return;

    //
    // go back to the world matrix
    //
    glLoadMatrixf(r_world_matrix);

    if (r_wateralpha.value < 1.f) {
        glEnable(GL_BLEND);
        glColor4f(1.f, 1.f, 1.f, r_wateralpha.value);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    if (!gl_texsort.value) {
        if (!waterchain)
            return;

        for (mSurface_p s = waterchain; s; s = s->texturechain) {
            GL_Bind(s->texinfo->texture->gl_texturenum);
            EmitWaterPolys(s);
        }

        waterchain = NULL;
    }
    else {
        for (int i = 0; i < cl.worldmodel->numtextures; i++) {
            Texture_p t = cl.worldmodel->textures[i];
            if (!t)         continue;

            mSurface_p s = t->texturechain;
            if ((!s) ||
                (!(s->flags & SURF_DRAWTURB))
                )               continue;

            // set modulate mode explicitly

            GL_Bind(t->gl_texturenum);

            for (; s; s = s->texturechain)
                EmitWaterPolys(s);

            t->texturechain = NULL;
        }
    }

    if (r_wateralpha.value < 1.0) {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        glColor4f(1, 1, 1, 1);
        glDisable(GL_BLEND);
    }

}

#endif

/*
================
DrawTextureChains
================
*/
void DrawTextureChains() {
    if (!gl_texsort.value) {
        GL_DisableMultitexture();

        if (skychain) {
            R_DrawSkyChain(skychain);
            skychain = NULL;
        }
        return;
    }

    for (int i = 0; i < cl.worldmodel->numtextures; i++) {
        Texture_p texture = cl.worldmodel->textures[i];
        if (!texture)   continue;
        mSurface_p surf = texture->texturechain;
        if (!surf)      continue;

        if (i == skytexturenum)
            R_DrawSkyChain(surf);
        else if (
            (i == mirrortexturenum) &&
            (r_mirroralpha.value != 1.0f)
            ) {
            R_MirrorChain(surf);
            continue;
        }
        else {
            if ((surf->flags & SURF_DRAWTURB) &&
                (r_wateralpha.value != 1.0f)
                )
                continue;    // draw translucent water later
            for (; surf; surf = surf->texturechain)
                R_RenderBrushPoly(surf);
        }

        texture->texturechain = NULL;
    }
}

bool R_CullBox(BBox_t bb); // TODO: palce it in specific header

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel(r_Entity_p e) {
    currententity = e;
    currenttexture = -1;

    Model_p clmodel = e->model;
    bool rotated;

    if (!AngleCompare(e->angles, a3Zero)) {
        rotated = true;
        if (R_CullBox(BBoxTranslate(
            BBoxSymmetric(clmodel->radius),
            e->origin))
        )   return;
    }
    else {
        rotated = false;
        if (R_CullBox(BBoxTranslate(
            clmodel->BB,
            e->origin))
        )   return;
    }

    glColor3f(1, 1, 1);
    memset(lightmap_polys, 0, sizeof(lightmap_polys));

    modelorg = VectorSubtract(r_refdef.vieworg, e->origin);
    if (rotated) {
        vec3_t temp = modelorg;

        Basis_t bs = GetBasis(e->angles);
        modelorg = (vec3_t){
            .x = DotProduct(temp, bs.forward),
            .y = -DotProduct(temp, bs.right),
            .z = DotProduct(temp, bs.up)
        };
    }

    mSurface_p psurf = &clmodel->surfaces[clmodel->firstModelSurface];

    // calculate dynamic lighting for bmodel if it's not an instanced model
    // TODO: apply r_dlightmap
    if ((clmodel->firstModelSurface != 0) &&
        (!gl_flashblend.value)
        ) {
        for (int k = 0; k < MAX_DLIGHTS; k++) {
            if ((cl_dlights[k].die < GetClSimTime()) ||
                (!cl_dlights[k].radius)
                )   continue;

            R_MarkLights(&cl_dlights[k], 1 << k,
                clmodel->nodes + clmodel->hulls[0].firstclipnode);
        }
    }

    glPushMatrix();
    e->angles.pitch = -e->angles.pitch;    // stupid quake bug
    R_RotateForEntity(e);
    e->angles.pitch = -e->angles.pitch;    // stupid quake bug

    //
    // draw texture
    //
    for (int i = 0; i < clmodel->numModelSurfaces; i++, psurf++) {
        // find which side of the node we are on
        mPlane_p pplane = psurf->plane;

        float dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

        // draw the polygon
        if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
            (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON))
            ) {

            if (gl_texsort.value)   R_RenderBrushPoly(psurf);
            else                    R_DrawSequentialPoly(psurf);
        }
    }

    R_BlendLightmaps();

    glPopMatrix();
}

/*
=============================================================

    WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode(mNode_p node) {
    if ((node->contents == CONTENTS_SOLID) ||   // solid
        (node->visframe != r_visframecount) ||
        (R_CullBox(node->bb))
        )   return;

    // if a leaf node, draw stuff
    if (node->contents < CONTENTS_NODE) {
        mLeaf_p pleaf = (mLeaf_p)node;

        mSurface_ar mark = pleaf->firstmarksurface;
        int c = pleaf->nummarksurfaces;

        if (c && (*mark)) {
            do {
                (*mark)->visframe = r_framecount;
                mark++;
            } while (--c);
        }

        // deal with model fragments in this leaf
        if (pleaf->efrags)
            R_StoreEfrags(&pleaf->efrags);

        return; // recursive exit
    }
    else {
        // node is just a decision point, so go down the apropriate sides

        // find which side of the node we are on
        mPlane_p plane = node->plane;

        double dot;
        switch (plane->type) {
        case PLANE_X: { dot = modelorg.x - plane->dist; } break;
        case PLANE_Y: { dot = modelorg.y - plane->dist; } break;
        case PLANE_Z: { dot = modelorg.z - plane->dist; } break;
        default: { dot = DotProduct(modelorg, plane->normal) - plane->dist; } break;
        }

        int side = (dot >= 0) ? 0 : 1;

        // recurse down the children, front side first
        R_RecursiveWorldNode(node->children[side]);

        // draw stuff
        int c = node->numsurfaces;

        if (c) {
            mSurface_p surf = cl.worldmodel->surfaces + node->firstsurface;

            if (dot < 0.0 - BACKFACE_EPSILON)     side = SURF_PLANEBACK;
            else if (dot > BACKFACE_EPSILON)    side = 0;
            {
                for (; c; c--, surf++) {
                    if (surf->visframe != r_framecount) // TODO: find whay we always fall out from render?
                        continue;

                    // don't backface underwater surfaces, because they warp
                    if (!(surf->flags & SURF_UNDERWATER) &&
                        ((dot < 0.0) ^ (!!(surf->flags & SURF_PLANEBACK)))
                        )
                        continue;        // wrong side

                    // if sorting by texture, just store it out
                    if (gl_texsort.value) {
                        if (!mirror ||
                            (surf->texinfo->texture != cl.worldmodel->textures[mirrortexturenum])
                            ) {
                            surf->texturechain = surf->texinfo->texture->texturechain;
                            surf->texinfo->texture->texturechain = surf;
                        }
                    }
                    else if (surf->flags & SURF_DRAWSKY) {
                        surf->texturechain = skychain;
                        skychain = surf;
                    }
                    else if (surf->flags & SURF_DRAWTURB) {
                        surf->texturechain = waterchain;
                        waterchain = surf;
                    }
                    else
                        R_DrawSequentialPoly(surf);

                }
            }

        }

        // recurse down the back side
        R_RecursiveWorldNode(node->children[!side]);
    }
}



/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld() {
#if 1
    r_Entity_t    ent;
    memset(&ent, 0, sizeof(ent));
    ent.model = cl.worldmodel;
#else
    r_Entity_t ent = {
        .model = cl.worldmodel
    };
#endif

    modelorg = r_refdef.vieworg;

    currententity = &ent;
    currenttexture = -1;

    glColor3f(1, 1, 1);
    memset(lightmap_polys, 0, sizeof(lightmap_polys));
#ifdef QUAKE2
    R_ClearSkyBox();
#endif

    R_RecursiveWorldNode(cl.worldmodel->nodes);
    DrawTextureChains();
    R_BlendLightmaps();

#ifdef QUAKE2
    R_DrawSkyBox();
#endif
}


/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves() {
    if (
        (r_oldviewleaf == r_viewleaf) &&
        (!r_novis.value)
        )
        return;

    if (mirror)
        return;

    r_visframecount++;
    r_oldviewleaf = r_viewleaf;

    uint8_p vis;
    uint8_t solid[4096];
    if (!r_novis.value) {
        vis = Mod_LeafPVS(r_viewleaf, cl.worldmodel);
    }
    else {
        memset(solid, 0xFF, EIGHTH(cl.worldmodel->numleafs + 7));
        vis = solid;
    }

    for (int i = 0; i < cl.worldmodel->numleafs; i++) {
        if (vis[EIGHTH(i)] & (1 << (i & 7))) {
            mNode_p node = (mNode_p)&cl.worldmodel->leafs[i + 1];
            do {
                if (node->visframe == r_visframecount)
                    break;
                node->visframe = r_visframecount;
                node = node->parent;
            } while (node);
        }
    }
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

// returns a texture number and the position inside it
int AllocBlock(int w, int h, int* x, int* y) {
    for (int texnum = 0; texnum < MAX_LIGHTMAPS; texnum++) {
        int best = BLOCK_HEIGHT;

        for (int i = 0; i < BLOCK_WIDTH - w; i++) {
            int best2 = 0;

            int j = 0;
            for (; j < w; j++) {
                if (allocated[texnum][i + j] >= best)
                    break;
                if (allocated[texnum][i + j] > best2)
                    best2 = allocated[texnum][i + j];
            }
            if (j == w) {    // this is a valid spot
                *x = i;
                *y = best = best2;
            }
        }

        if (best + h > BLOCK_HEIGHT)
            continue;

        for (int i = 0; i < w; i++)
            allocated[texnum][*x + i] = best + h;

        return texnum;
    }

    Host_SysError("AllocBlock: full");
}


mVertex_p r_pcurrentvertbase;
Model_p currentmodel;

int    nColinElim;

/*
================
BuildSurfaceDisplayList
================
*/
void BuildSurfaceDisplayList(mSurface_p fa) {
    // reconstruct the polygon
    mEdge_p pedges = currentmodel->edges;
    int lnumverts = fa->numedges;

    //
    // draw texture
    //
    glpoly_p poly = Hunk_Alloc(sizeof(glpoly_t) + (lnumverts - 4) * sizeof(glVert_t));
    poly->next = fa->polys;
    poly->flags = fa->flags;
    fa->polys = poly;
    poly->numverts = lnumverts;

    for (int i = 0; i < lnumverts; i++) {
        int lindex = currentmodel->surfedges[fa->firstedge + i];

        vec3_t vec;
        if (lindex > 0) {
            mEdge_p r_pedge = &pedges[lindex];
            vec = r_pcurrentvertbase[r_pedge->v16[0]].position;
        }
        else {
            mEdge_p r_pedge = &pedges[-lindex];
            vec = r_pcurrentvertbase[r_pedge->v16[1]].position;
        }

        poly->verts[i].v = vec;
        poly->verts[i].tx.s = (DotProduct(vec, fa->texinfo->vecs[S_AX].vx) + fa->texinfo->vecs[S_AX].offs) / fa->texinfo->texture->width;
        poly->verts[i].tx.t = (DotProduct(vec, fa->texinfo->vecs[T_AX].vx) + fa->texinfo->vecs[T_AX].offs) / fa->texinfo->texture->height;

        //
        // lightmap texture coordinates
        //
        poly->verts[i].lMap.s = (
            DotProduct(vec, fa->texinfo->vecs[S_AX].vx) +
            fa->texinfo->vecs[S_AX].offs -
            fa->texturemins[S_AX] + 8.0f +
            (fa->light_s * 16.0f)
            ) /
            (BLOCK_WIDTH * 16.0f); //fa->texinfo->texture->width;

        poly->verts[i].lMap.t = (
            DotProduct(vec, fa->texinfo->vecs[T_AX].vx) +
            fa->texinfo->vecs[T_AX].offs -
            fa->texturemins[T_AX] + 8.0f +
            (fa->light_t * 16.0f)
            ) /
            (BLOCK_HEIGHT * 16.0f); //fa->texinfo->texture->height;

    }

    //
    // remove co-linear points - Ed
    //
    if (!gl_keeptjunctions.value && !(fa->flags & SURF_UNDERWATER)) {
        for (int i = 0; i < lnumverts; ++i) {
            vec3_t v1, v2;
            vec3_t prev, this, next;

            prev = *(vec3_p)(&poly->verts[(i + lnumverts - 1) % lnumverts]);
            this = *(vec3_p)(&poly->verts[i]);
            next = *(vec3_p)(&poly->verts[(i + 1) % lnumverts]);

            v1 = VectorSubtract(this, prev);
            VectorNormalize(&v1);
            v2 = VectorSubtract(next, prev);
            VectorNormalize(&v2);

            // skip co-linear points
#define COLINEAR_EPSILON 0.001
            if ((fabs(v1.x - v2.x) <= COLINEAR_EPSILON) &&
                (fabs(v1.y - v2.y) <= COLINEAR_EPSILON) &&
                (fabs(v1.z - v2.z) <= COLINEAR_EPSILON)) {
                for (int j = i + 1; j < lnumverts; ++j) {
                    poly->verts[j - 1] = poly->verts[j];
                }
                --lnumverts;
                ++nColinElim;
                // retry next vertex next time, which is now current vertex
                --i;
            }
        }
    }
    poly->numverts = lnumverts;

}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap(mSurface_p surf) {
    if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
        return;

    int smax = FIXED4_TO_INT(surf->extents[S_AX]) + 1;
    int tmax = FIXED4_TO_INT(surf->extents[T_AX]) + 1;

    surf->lightmaptexturenum = AllocBlock(smax, tmax, &surf->light_s, &surf->light_t);
    byte* base = lightmaps + surf->lightmaptexturenum * _lightMapBytes * BLOCK_WIDTH * BLOCK_HEIGHT;
    base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * _lightMapBytes;
    R_BuildLightMap(surf, base, BLOCK_WIDTH * _lightMapBytes);
}


/*
==================
GL_BuildLightmaps

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void GL_BuildLightmaps() {
    memset(allocated, 0, sizeof(allocated));

    r_framecount = 1;        // no dlightcache

    if (!_lightMapTextures) {
        _lightMapTextures = texture_extension_number;
        texture_extension_number += MAX_LIGHTMAPS;
    }

    gl_lightmap_format = GL_LUMINANCE;
    // default differently on the Permedia
    if (isPermedia)                 gl_lightmap_format = GL_RGBA;
    if (COM_CheckParm("-lm_1"))     gl_lightmap_format = GL_LUMINANCE;
    if (COM_CheckParm("-lm_a"))     gl_lightmap_format = GL_ALPHA;
    if (COM_CheckParm("-lm_i"))     gl_lightmap_format = GL_INTENSITY;
    if (COM_CheckParm("-lm_2"))     gl_lightmap_format = GL_RGBA4;
    if (COM_CheckParm("-lm_4"))     gl_lightmap_format = GL_RGBA;

    switch (gl_lightmap_format) {
    case GL_LUMINANCE:
    case GL_INTENSITY:
    case GL_ALPHA:      _lightMapBytes = 1;        break;
    case GL_RGBA4:      _lightMapBytes = 2;        break;
    case GL_RGBA:       _lightMapBytes = 4;        break;
    default:                                       break;
    }

    for (int j = 1; j < MAX_MODELS; j++) {
        Model_p mdl = cl.model_precache[j];
        if (!mdl)            break;
        if (mdl->name[0] == '*')
            continue;

        r_pcurrentvertbase = mdl->vertexes;
        currentmodel = mdl;
        for (int i = 0; i < mdl->numsurfaces; i++) {
            GL_CreateSurfaceLightmap(mdl->surfaces + i);
            if (mdl->surfaces[i].flags & SURF_DRAWTURB)
                continue;
#ifndef QUAKE2
            if (mdl->surfaces[i].flags & SURF_DRAWSKY)
                continue;
#endif
            BuildSurfaceDisplayList(mdl->surfaces + i);
        }
    }

    if (!gl_texsort.value)
        GL_SelectTexture(TEXTURE1_SGIS);

    //
    // upload all lightmaps that were filled
    //

    for (int i = 0; i < MAX_LIGHTMAPS; i++) {
        if (!allocated[i][0])
            break;        // no more used

        lightmap_modified[i] = false;
        lightmap_rectchange[i] = (glRect_t){
            .l = BLOCK_WIDTH,
            .t = BLOCK_HEIGHT,
            .w = 0,
            .h = 0
        };
        GL_Bind(_lightMapTextures + i);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D, 0, _lightMapBytes,
            BLOCK_WIDTH, BLOCK_HEIGHT, 0,
            gl_lightmap_format, GL_UNSIGNED_BYTE,
            lightmaps + i * BLOCK_WIDTH * BLOCK_HEIGHT * _lightMapBytes
        );
    }

    if (!gl_texsort.value)
        GL_SelectTexture(TEXTURE0_SGIS);

}

