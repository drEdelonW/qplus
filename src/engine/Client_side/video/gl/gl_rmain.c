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
// r_main.c
#include "qOpenGL.h"
#include "world.h"
#include "Sprite.h"
#include "console.h"
#include "client.h"
#include "angle.h"
#include <string.h>
#include "mathlib.h"
#include "model.h"
#include "cvar_q1.h"
#include "gamedefs.h"
#include "view.h"
#include "sound.h"
#include "host.h"
#include "q_tools.h"
#include "LeafModel.h"

r_Entity_t r_worldentity; // was Entity_t

bool r_cache_thrash;  // compatability

vec3_t  modelorg;
vec3_t  r_entorigin;
r_Entity_p currententity;
int   r_visframecount; // bumped when going to a new PVS
int   r_framecount;  // used for dlight push checking

int     c_brush_polys, c_alias_polys;   // FYI: DEBUG metrics
bool    envmap;    // true during envmap command capture
int     currenttexture = -1;  // to avoid unnecessary texture sets
int     cnttextures[2] = { -1, -1 };     // cached
int     particletexture; // little dot for particles
int     playertextures;  // up to 16 color translated skins

int     mirrortexturenum; // quake texturenum, not gltexturenum
bool    mirror;
mPlane_p mirror_plane;

//
// view origin
//
Basis_t BS;
vec3_t r_origin;

float r_world_matrix[16];
float r_base_world_matrix[16];

//
// screen size info
//
refdef_t r_refdef;
mLeaf_p r_viewleaf, r_oldviewleaf;


fixed8_t d_lightstylevalue[256]; // 8.8 fraction of base light value


void R_MarkLeaves();

cvar_t r_norefresh = { "r_norefresh", "0" };
// cvar_t r_lightmap = { "r_lightmap", "0" };
cvar_t r_shadows = { "r_shadows", "0" };
cvar_t r_mirroralpha = { "r_mirroralpha", "1" };
cvar_t r_wateralpha = { "r_wateralpha", "1" };
cvar_t r_dynamic = { "r_dynamic", "1" };
cvar_t r_novis = { "r_novis", "0" };

cvar_t gl_finish = { "gl_finish", "0" };
cvar_t gl_clear = { "gl_clear", "0" };
cvar_t gl_cull = { "gl_cull", "1" };
cvar_t gl_texsort = { "gl_texsort", "1" };
cvar_t gl_smoothmodels = { "gl_smoothmodels", "1" };
cvar_t gl_affinemodels = { "gl_affinemodels", "0" };
cvar_t gl_polyblend = { "gl_polyblend", "1" };
cvar_t gl_flashblend = { "gl_flashblend", "1" };
cvar_t gl_playermip = { "gl_playermip", "0" };
cvar_t gl_nocolors = { "gl_nocolors", "0" };
cvar_t gl_keeptjunctions = { "gl_keeptjunctions", "0" };
cvar_t gl_reporttjunctions = { "gl_reporttjunctions", "0" };
cvar_t gl_doubleeyes = { "gl_doubleeys", "1" };


void R_RotateForEntity(r_Entity_p e) {
    glTranslatef(e->pose.spot.x, e->pose.spot.y, e->pose.spot.z);

    glRotatef(e->pose.facing.yaw, 0, 0, 1);
    glRotatef(-e->pose.facing.pitch, 0, 1, 0);
    glRotatef(e->pose.facing.roll, 1, 0, 0);
}

/*
=============================================================

SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mSpriteFrame_p R_GetSpriteFrame(r_Entity_p currententity) { // TODO: seems like software function as is
    mSprite_p psprite = currententity->model->cache.data;
    int frame = currententity->frame;
    if ((frame >= psprite->numframes) ||
        (frame < 0)
        ) {
        Con_Printf("R_DrawSprite: no such frame %d\n", frame);
        frame = 0;
    }

    mSpriteFrame_p pspriteframe;
    if (psprite->frames[frame].type == SPR_SINGLE) {
        pspriteframe = psprite->frames[frame].frameptr;
    }
    else {
        mSpriteGroup_p pspritegroup = (mSpriteGroup_p)psprite->frames[frame].frameptr;
        LegDt_p pintervals = pspritegroup->intervals;   // TODO: replace by time interval specific type
        int numframes = pspritegroup->numframes;
        float fullinterval = pintervals[numframes - 1];

        LegDt_t time = GetClSimTime() + currententity->syncbase;

        // when loading in Mod_LoadSpriteGroup, we guaranteed all interval values are positive, so we don't have to worry about division by 0
        LegDt_t targettime = time - ((int)(time / fullinterval)) * fullinterval;

        int i = 0;
        for (; i < (numframes - 1); i++) {
            if (pintervals[i] > targettime)
                break;
        }
        pspriteframe = pspritegroup->frames[i];
    }

    return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel(r_Entity_p e) {
    // don't even bother culling, because it's just a single polygon without a surface cache
    mSpriteFrame_p frame = R_GetSpriteFrame(e);
    mSprite_p psprite = currententity->model->cache.data;
    vec3_t up, right;

    if (psprite->type == SPR_ORIENTED) { // bullet marks on walls
        Basis_t bs = GetBasis(currententity->pose.facing);
        up = bs.up;
        right = bs.right;
    }
    else { // normal sprite
        up = BS.up;
        right = BS.right;
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    GL_DisableMultitexture();
    GL_Bind(frame->gl_texturenum);
    glEnable(GL_ALPHA_TEST); {
        glBegin(GL_QUADS); {
            glTexCoord2f(0.0f, 1.0f);            glVertex3fv(VectorMA(VectorMA(e->pose.spot, frame->down, up), frame->left, right).v);
            glTexCoord2f(0.0f, 0.0f);            glVertex3fv(VectorMA(VectorMA(e->pose.spot, frame->up, up), frame->left, right).v);
            glTexCoord2f(1.0f, 0.0f);            glVertex3fv(VectorMA(VectorMA(e->pose.spot, frame->up, up), frame->right, right).v);
            glTexCoord2f(1.0f, 1.0f);            glVertex3fv(VectorMA(VectorMA(e->pose.spot, frame->down, up), frame->right, right).v);
        } glEnd();
    } glDisable(GL_ALPHA_TEST);
}

/*
=============================================================

ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS 162

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#if 0
float r_avertexnormals[NUMVERTEXNORMALS][3] = {
#else
vec3_t r_avertexnormals[NUMVERTEXNORMALS] = {
#endif
#   include "anorms.h"
};
#pragma GCC diagnostic pop

vec3_t shadevector;
float shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float_p shadedots = r_avertexnormal_dots[0];

int lastposenum;

/*
=============
GL_DrawAliasFrame
=============
*/
void GL_DrawAliasFrame(AliasHdr_p pAliasHdr, int posenum) {
    lastposenum = posenum;

    TriVertx_p verts = (TriVertx_p)((uint8_p)pAliasHdr + pAliasHdr->posedata);
    verts += posenum * pAliasHdr->poseverts;
    int* order = (int*)((uint8_p)pAliasHdr + pAliasHdr->commands);

    while (1) {
        // get the vertex count and primitive type
        int count = *order++;
        if (!count)     break;  // done

        GLenum mode;
        if (count < 0) {
            count = -count;
            mode = GL_TRIANGLE_FAN;
        }
        else
            mode = GL_TRIANGLE_STRIP;

        glBegin(mode); {
            do {
                // texture coordinates come from the draw list
                glTexCoord2f(((float_p)order)[0], ((float_p)order)[1]);
                order += 2;

                // normals and vertexes come from the frame list
                float l = shadedots[verts->lightnormalindex] * shadelight;
                glColor3f(l, l, l);
                glVertex3f(verts->v8[X_AX], verts->v8[Y_AX], verts->v8[Z_AX]);
                verts++;
            } while (--count);

        } glEnd();
    }
}


/*
=============
GL_DrawAliasShadow
=============
*/

void GL_DrawAliasShadow(AliasHdr_p pAliasHdr, int posenum) {

    float lheight = currententity->pose.spot.z - lightspot.z;

    float height = 0;
    TriVertx_p verts = (TriVertx_p)((uint8_p)pAliasHdr + pAliasHdr->posedata);
    verts += posenum * pAliasHdr->poseverts;
    int* order = (int*)((uint8_p)pAliasHdr + pAliasHdr->commands);

    height = -lheight + 1.0;

    while (1) {
        // get the vertex count and primitive type
        int count = *order++;
        if (!count)     break;  // done

        GLenum mode;
        if (count < 0) {
            count = -count;
            mode = GL_TRIANGLE_FAN;
        }
        else
            mode = GL_TRIANGLE_STRIP;

        glBegin(mode); {

            do {
                // texture coordinates come from the draw list
                // (skipped for shadows) glTexCoord2fv ((float *)order);
                order += 2;

                // normals and vertexes come from the frame list
                vec3_t point = {
                    .x = verts->v8[X_AX] * pAliasHdr->scale.x + pAliasHdr->scale_origin.x,
                    .y = verts->v8[Y_AX] * pAliasHdr->scale.y + pAliasHdr->scale_origin.y,
                    .z = verts->v8[Z_AX] * pAliasHdr->scale.z + pAliasHdr->scale_origin.z
                };
                point.x -= shadevector.x * (point.z + lheight);
                point.y -= shadevector.y * (point.z + lheight);
                point.z = height;
                //   height -= 0.001;
                glVertex3fv(point.v);

                verts++;
            } while (--count);

        } glEnd();
    }
}



/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame(int frame, AliasHdr_p pAliasHdr) {
    if ((frame >= pAliasHdr->numframes) ||
        (frame < 0)
        ) {
        Con_DPrintf("R_AliasSetupFrame: no such frame %d\n", frame);
        frame = 0;
    }

    int pose = pAliasHdr->frames[frame].firstpose;
    int numposes = pAliasHdr->frames[frame].numposes;

    if (numposes > 1) {
        float interval = pAliasHdr->frames[frame].interval;
        pose += (int)(GetClSimTime() / interval) % numposes;
    }

    GL_DrawAliasFrame(pAliasHdr, pose);
}


/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
static mPlane_t _frustum[4];
bool R_CullBox(BBox_t bb) {
    for (int i = 0; i < 4; i++)
        if (BoxOnPlaneSide(bb, &_frustum[i]) == PsBack)
            return true;
    return false;
}


/*
=================
R_DrawAliasModel

=================
*/
#include "vid.h" // vid.colormap
void R_DrawAliasModel(r_Entity_p e) {
    Model_p clmodel = currententity->model;

    if (R_CullBox(BBoxTranslate(clmodel->BB, currententity->pose.spot)))      return;

    r_entorigin = currententity->pose.spot;
    modelorg = VectorSubtract(r_origin, r_entorigin);

    //
    // get lighting information
    //

    ambientlight = shadelight = R_LightPoint(currententity->pose.spot);

    // allways give the gun some light
    if ((e == &cl.viewent) &&
        (ambientlight < 24)
        )   ambientlight = shadelight = 24;

    for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
        if (cl_dlights[lnum].die >= GetClSimTime()) {
            vec3_t  dist = VectorSubtract(
                currententity->pose.spot,
                cl_dlights[lnum].origin
            );
            float add = cl_dlights[lnum].radius - Length(dist);

            if (add > 0) {
                ambientlight += add;
                //ZOID models should be affected by dlights as well
                shadelight += add;
            }
        }
    }

    // clamp lighting so it doesn't overbright as much
    CLAMP_MORE(&ambientlight, 128.f);

    if ((ambientlight + shadelight) > 192.0f)
        shadelight = 192.0f - ambientlight;

    // ZOID: never allow players to go totally black
    int i = currententity - cl_entities;
    if ((i >= 1) &&
        (i <= cl.maxclients) /* &&
        !strcmp (currententity->model->name, "progs/player.mdl") */
       )    if (ambientlight < 8.f) {
               ambientlight = 8.f;
               shadelight = 8.f;
           }

    // HACK HACK HACK -- no fullbright colors, so make torches full light
    if (!strcmp(clmodel->name, "progs/flame2.mdl") ||
        !strcmp(clmodel->name, "progs/flame.mdl")
        )   ambientlight = shadelight = 256.0f;

    shadedots = r_avertexnormal_dots[
        ((int)(e->pose.facing.yaw * (SHADEDOT_QUANT / 360.0f))) & (SHADEDOT_QUANT - 1)
    ];
    shadelight = shadelight / 200.0f;

    float an = DEG2RAD(e->pose.facing.yaw);

    shadevector = (vec3_t){
        .x = cosf(-an),
        .y = sinf(-an),
        .z = 1.f
    };
    VectorNormalize(&shadevector);

    //
    // locate the proper data
    //
    AliasHdr_p pAliasHdr = (AliasHdr_p)Mod_Extradata(currententity->model);

    c_alias_polys += pAliasHdr->numtris;

    //
    // draw all the triangles
    //

    GL_DisableMultitexture();

    glPushMatrix();
    R_RotateForEntity(e);

    if (!strcmp(clmodel->name, "progs/eyes.mdl") && gl_doubleeyes.value) {
        glTranslatef(
            pAliasHdr->scale_origin.x,
            pAliasHdr->scale_origin.y,
            pAliasHdr->scale_origin.z - (22 + 8)
        );
        // double size of eyes, since they are really hard to see in gl
        glScalef(
            pAliasHdr->scale.x * 2.0f,
            pAliasHdr->scale.y * 2.0f,
            pAliasHdr->scale.z * 2.0f
        );
    }
    else {
        glTranslatef(
            pAliasHdr->scale_origin.x,
            pAliasHdr->scale_origin.y,
            pAliasHdr->scale_origin.z
        );
        glScalef(
            pAliasHdr->scale.x,
            pAliasHdr->scale.y,
            pAliasHdr->scale.z
        );
    }

    int anim = (int)(GetClSimTime() * 10) & 3;
    GL_Bind(pAliasHdr->gl_texturenum[currententity->skinnum][anim]);

    // we can't dynamically colormap textures, so they are cached
    // seperately for the players.  Heads are just uncolored.
    if ((currententity->colormap != vid.colormap) &&
        (!gl_nocolors.value)
        ) {
        int i = currententity - cl_entities;
        if (i >= 1 && i <= cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
            GL_Bind(playertextures - 1 + i);
    }

    if (gl_smoothmodels.value)
        glShadeModel(GL_SMOOTH);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (gl_affinemodels.value)      glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

    R_SetupAliasFrame(currententity->frame, pAliasHdr);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glShadeModel(GL_FLAT);
    if (gl_affinemodels.value)      glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glPopMatrix();

    if (r_shadows.value) {
        glPushMatrix(); {
            R_RotateForEntity(e);
            glDisable(GL_TEXTURE_2D);   glEnable(GL_BLEND);     glColor4f(0, 0, 0, 0.5); {
                GL_DrawAliasShadow(pAliasHdr, lastposenum);
            } glEnable(GL_TEXTURE_2D);    glDisable(GL_BLEND);    glColor4f(1, 1, 1, 1);
        } glPopMatrix();
    }

}

//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList() {
    if (!r_drawentities.value)      return;

    // draw sprites seperately, because of alpha blending
    for (int i = 0; i < cl_numvisedicts; i++) {
        currententity = cl_visedicts[i];

        switch (currententity->model->type) {
        case mod_alias: { R_DrawAliasModel(currententity); } break;
        case mod_brush: { R_DrawBrushModel(currententity); } break;
        case mod_sprite: break; // will be in next loop
        default: {
            Host_SysError(
                "R_DrawEntitiesOnList first loop [0x%X] UNKNOWN type of model->type\n",
                currententity->model->type
            );
        } break;
        }
    }

    for (int i = 0; i < cl_numvisedicts; i++) {
        currententity = cl_visedicts[i];

        switch (currententity->model->type) {
        case mod_alias:
        case mod_brush: break; // already drawed
        case mod_sprite: { R_DrawSpriteModel(currententity); } break;
        default: {
            Host_SysError(
                "R_DrawEntitiesOnList second loop [0x%X] UNKNOWN type of model->type\n",
                currententity->model->type
            );
        } break;
        }
    }
}

/*
=============
R_DrawViewModel
=============
*/
static float _glDepthMin;   // TODO: make some Min/Max flat type
static float _glDepthMax;
void R_DrawViewModel() {
    if (!r_drawviewmodel.value)     return;
    if (chase_active.value)         return;
    if (envmap)                     return;
    if (!r_drawentities.value)      return;
    if (cl.items & IT_INVISIBILITY) return;
    if (cl.stats[STAT_HEALTH] <= 0) return;
    currententity = &cl.viewent;
    if (!currententity->model)      return;

    int j = R_LightPoint(currententity->pose.spot);
    CLAMP_LESS(&j, 24);  // allways give some light on gun
    int ambientlight = j;

    // add dynamic lights
    for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
        dLight_p dl = &cl_dlights[lnum];
        if (!dl->radius)        continue;
        if (!dl->radius)        continue;
        if (dl->die < GetClSimTime())  continue;

        vec3_t dist = VectorSubtract(currententity->pose.spot, dl->origin);
        float add = dl->radius - Length(dist);
        if (add > 0)
            ambientlight += add;
    }

#if 0
    int shadelight = j;
    float ambient[4], diffuse[4];
    ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
    diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;
#endif
    #warning TODO: investigate why ambient and diffuse go nowhere

    // hack the depth range to prevent view model from poking into walls
    glDepthRange(_glDepthMin, _glDepthMin + 0.3 * (_glDepthMax - _glDepthMin));
    R_DrawAliasModel(currententity);
    glDepthRange(_glDepthMin, _glDepthMax);
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend() {
    if (!gl_polyblend.value)    return;
    if (!v_blend[3])            return;

    GL_DisableMultitexture();

    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glLoadIdentity();

    glRotatef(-90, 1, 0, 0);     // put Z going up
    glRotatef(90, 0, 0, 1);     // put Z going up

    glColor4fv(v_blend);

    glBegin(GL_QUADS); {
        glVertex3f(10, 100, 100);
        glVertex3f(10, -100, 100);
        glVertex3f(10, -100, -100);
        glVertex3f(10, 100, -100);
    } glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
}


int SignbitsForPlane(mPlane_p out) {
    // for fast box on planeside test
    int bits = 0x00;
    for (int j = 0; j < VECT_DIM; j++) {
        if (out->normal.v[j] < 0.0f)
            bits |= 1 << j;
    }
    return bits;
}

void R_SetFrustum() {
    if (r_refdef.fov_x == 90.0f) {
        // front side is visible
        _frustum[0].normal = VectorAdd(BS.forward, BS.right);
        _frustum[1].normal = VectorSubtract(BS.forward, BS.right);
        _frustum[2].normal = VectorAdd(BS.forward, BS.up);
        _frustum[3].normal = VectorSubtract(BS.forward, BS.up);
    }
    else {
        _frustum[0].normal = GetRotatePointAroundVector(BS.up, BS.forward, -(90.0f - r_refdef.fov_x / 2.0f));      // rotate VPN right by FOV_X/2 degrees
        _frustum[1].normal = GetRotatePointAroundVector(BS.up, BS.forward, (90.0f - r_refdef.fov_x / 2.0f));       // rotate VPN left by FOV_X/2 degrees
        _frustum[2].normal = GetRotatePointAroundVector(BS.right, BS.forward, (90.0f - r_refdef.fov_y / 2.0f));    // rotate VPN up by FOV_X/2 degrees
        _frustum[3].normal = GetRotatePointAroundVector(BS.right, BS.forward, -(90.0f - r_refdef.fov_y / 2.0f));   // rotate VPN down by FOV_X/2 degrees
    }

    for (int i = 0; i < 4; i++) {
        _frustum[i].type = PLANE_ANYZ;
        _frustum[i].dist = DotProduct(r_origin, _frustum[i].normal);
        _frustum[i].signbits = SignbitsForPlane(&_frustum[i]);
    }
}


/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame() {
    // don't allow cheats in multiplayer
    if (cl.maxclients > 1)  Cvar_Set("r_fullbright", "0");

    R_AnimateLight();

    r_framecount++;

    // build the transformation matrix for the given view angles
    r_origin = r_refdef.view.spot;

    BS = GetBasis(r_refdef.view.facing);

    // current viewleaf
    r_oldviewleaf = r_viewleaf;
    r_viewleaf = Mod_PointInLeaf(r_origin, cl.worldmodel);

    V_SetContentsColor(r_viewleaf->contents);
    V_CalcBlend();

    r_cache_thrash = false;

    c_brush_polys = 0;   // FYI: DEBUG metrics
    c_alias_polys = 0;   // FYI: DEBUG metrics

}


void MYgluPerspective(
    GLdouble fovy,
    GLdouble aspect,
    GLdouble zNear,
    GLdouble zFar
) {
    GLdouble ymax = zNear * tan(fovy * M_PI / 360.0f);
    GLdouble ymin = -ymax;

    GLdouble xmin = ymin * aspect;
    GLdouble xmax = ymax * aspect;

    glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL() {
    //
    // set up viewpoint
    //
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    vRect_t vrect = r_refdef.vrect;
#if 0
    int x = vrect.x * (glwidth / Scr.vrect.width);
    int x2 = (vrect.x + vrect.width) * (glwidth / Scr.vrect.width);
    int y = (Scr.vrect.height - vrect.y) * (glheight / Scr.vrect.height);
    int y2 = (Scr.vrect.height - (vrect.y + vrect.height)) * (glheight / Scr.vrect.height);
#else
    float sx = (float)glwidth / Scr.vrect.width;
    float sy = (float)glheight / Scr.vrect.height;
    int yx = vrect.x;
    int ty = Scr.vrect.height - vrect.y;
    int x = yx * sx;
    int y = ty * sy;
    int x2 = (yx + vrect.width) * sx;
    int y2 = (ty - vrect.height) * sy;
#endif
    // fudge around because of frac screen scale
    if (x > 0)          x--;
    if (y < glheight)   y++;
    if (x2 < glwidth)   x2++;
    if (y2 < 0)         y2--;

    int w = x2 - x;
    int h = y - y2;

    if (envmap) {
        x = y2 = 0;
        w = h = 256;
    }

    glViewport(glx + x, gly + y2, w, h);
    float screenaspect = (float)vrect.width / vrect.height;
#if 0
    yfov = 2 * DEG2RAD(atan((float)vrect.height / vrect.width));
#endif
    MYgluPerspective(r_refdef.fov_y, screenaspect, 4, 4096);

    if (mirror) {
        if (mirror_plane->normal.z)     glScalef(1, -1, 1);
        else                            glScalef(-1, 1, 1);
        glCullFace(GL_BACK);
    }
    else
        glCullFace(GL_FRONT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(-90, 1, 0, 0);     // put Z going up
    glRotatef(90, 0, 0, 1);     // put Z going up
    glRotatef(-r_refdef.view.facing.roll, 1, 0, 0);
    glRotatef(-r_refdef.view.facing.pitch, 0, 1, 0);
    glRotatef(-r_refdef.view.facing.yaw, 0, 0, 1);
    glTranslatef(-r_refdef.view.spot.x, -r_refdef.view.spot.y, -r_refdef.view.spot.z);

    glGetFloatv(GL_MODELVIEW_MATRIX, r_world_matrix);

    //
    // set drawing parms
    //
    if (gl_cull.value)  glEnable(GL_CULL_FACE);
    else                glDisable(GL_CULL_FACE);

    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene() {
    R_SetupFrame();
    R_SetFrustum();
    R_SetupGL();
    R_MarkLeaves(); // done here so we know if we're in water
    R_DrawWorld();  // adds static entities to the list
    S_ExtraUpdate(); // don't let sound get messed up if going slow
    R_DrawEntitiesOnList();
    GL_DisableMultitexture();
    R_RenderDlights();
    R_DrawParticles();
#ifdef GLTEST
    Test_Draw();
#endif
}


/*
=============
R_Clear
=============
*/
extern cvar_t   gl_ztrick;
void R_Clear() {
    if (r_mirroralpha.value != 1.0) {
        if (gl_clear.value)     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        else                    glClear(GL_DEPTH_BUFFER_BIT);
        _glDepthMin = 0.0f;
        _glDepthMax = 0.5f;
        glDepthFunc(GL_LEQUAL);
    }
    else if (gl_ztrick.value) {
        static int _trickFrame;

        if (gl_clear.value)
            glClear(GL_COLOR_BUFFER_BIT);

        _trickFrame++;
        if (_trickFrame & 1) {
            _glDepthMin = 0.0f;
            _glDepthMax = 0.49999f;
            glDepthFunc(GL_LEQUAL);
        }
        else {
            _glDepthMin = 1.0f;
            _glDepthMax = 0.5f;
            glDepthFunc(GL_GEQUAL);
        }
    }
    else {
        if (gl_clear.value)     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        else                    glClear(GL_DEPTH_BUFFER_BIT);
        _glDepthMin = 0.0f;
        _glDepthMax = 1.0f;
        glDepthFunc(GL_LEQUAL);
    }

    glDepthRange(_glDepthMin, _glDepthMax);
}

/*
=============
R_Mirror
=============
*/
void R_Mirror() {
    if (!mirror)        return;

    memcpy(r_base_world_matrix, r_world_matrix, sizeof(r_base_world_matrix));

    r_refdef.view.spot = VectorMA(r_refdef.view.spot,
        (-2 * (DotProduct(r_refdef.view.spot, mirror_plane->normal) - mirror_plane->dist)), mirror_plane->normal
    );
    BS.forward = VectorMA(BS.forward,
        (-2 * DotProduct(BS.forward, mirror_plane->normal)), mirror_plane->normal
    );

    r_refdef.view.facing = (ang3_t){
        .pitch = DEG2RAD(-asin(BS.forward.z)),
        .yaw = DEG2RAD(atan2(BS.forward.y, BS.forward.x)),
        .roll = -r_refdef.view.facing.roll
    };

    if (cl_numvisedicts < MAX_VISEDICTS) {
        cl_visedicts[cl_numvisedicts] = &cl_entities[cl.viewentity];
        cl_numvisedicts++;
    }

    glDepthRange((_glDepthMin = 0.5f), (_glDepthMax = 1.0f));    glDepthFunc(GL_LEQUAL);

    R_RenderScene();
    R_DrawWaterSurfaces();

    glDepthRange((_glDepthMin = 0.0f), (_glDepthMax = 0.5f));    glDepthFunc(GL_LEQUAL);

    // blend on top
    glEnable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);

    if (mirror_plane->normal.z) glScalef(1, -1, 1);
    else                        glScalef(-1, 1, 1);

    glCullFace(GL_FRONT);
    glMatrixMode(GL_MODELVIEW);

    glLoadMatrixf(r_base_world_matrix);

    glColor4f(1, 1, 1, r_mirroralpha.value);

    for (mSurface_p surf = cl.worldmodel->textures[mirrortexturenum]->texturechain; surf; surf = surf->texturechain)
        R_RenderBrushPoly(surf);

    cl.worldmodel->textures[mirrortexturenum]->texturechain = NULL;
    glDisable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
// #define GLFOG
void R_RenderView() {
    if (r_norefresh.value)      return;

    if (!(r_worldentity.model) ||
        !(cl.worldmodel)
        )                       Host_SysError("R_RenderView: NULL worldmodel");

    LegTime_t time1;
    if (r_speeds.value) {
        glFinish();
        time1 = Host_FloatTime();
        c_brush_polys = 0;
        c_alias_polys = 0;
    }

    mirror = false;

    if (gl_finish.value)
        glFinish();

    R_Clear();

    // render normal view

/***** Experimental silly looking fog ******
****** Use r_fullbright if you enable ******/
#ifdef GLFOG
    GLfloat colors[4] = {
        (GLfloat)0.0f,  // R
        (GLfloat)0.5f,  // G
        (GLfloat)1.0f,  // B
        (GLfloat)0.2f   // A
    }; // fog color id BLUE

    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, colors);
    glFogf(GL_FOG_END, 512.0f);
    glEnable(GL_FOG);
#endif
    /********************************************/

    R_RenderScene();
    R_DrawViewModel();
    R_DrawWaterSurfaces();

    //  More fog right here :)
#ifdef GLFOG
    glDisable(GL_FOG);
#endif
    //  End of all fog code...

    R_Mirror(); // render mirror view

    R_PolyBlend();

    if (r_speeds.value) {
        //  glFinish();
        LegTime_t time2 = Host_FloatTime();
        Con_Printf("%3i ms  %4i wpoly %4i epoly\n", (int)((time2 - time1) * 1000), c_brush_polys, c_alias_polys);
    }
}
