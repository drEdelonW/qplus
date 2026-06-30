#pragma once
/*
Copyright(C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// refresh.h -- public interface to refresh functions
#include "model_effect.h"   // RocketTrailType
#include "transform.h"
#include "rEntity.h"
#include "Texture_pre.h"
#include "render.h"

//=============================================================================

#include "RefDef.h"

extern vec3_t   r_origin;

//
// surface cache related
//


#ifdef __cplusplus
extern "C" {
#endif

    void R_Init();
    void R_InitTextures();
    void R_InitEfrags();
    void R_RenderView();  // must set r_refdef first
    void R_ViewChanged(vRect_p pvrect, int lineadj, float aspect);
    // called whenever r_refdef or vid change
    void R_InitSky(Texture_p mt); // called at level load

    void R_AddEfrags(r_Entity_p ent);
    void R_RemoveEfrags(r_Entity_p ent);

    void R_NewMap();


    void R_ParseParticleEffect();
    void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count);
    void R_RocketTrail(vec3_t start, vec3_t end, RocketTrailType type);

#ifdef QUAKE2
    void R_DarkFieldParticles(r_Entity_p ent);
#endif
    void R_EntityParticles(r_Entity_p ent);
    void R_BlobExplosion(vec3_t org);
    void R_ParticleExplosion(vec3_t org);
    void R_ParticleExplosion2(vec3_t org, int colorStart, int colorLength);
    void R_LavaSplash(vec3_t org);
    void R_TeleportSplash(vec3_t org);

    void R_PushDlights();

    int  D_SurfaceCacheForRes(int width, int height);
    void D_FlushCaches();
    void D_DeleteSurfaceCache();
    void D_InitCaches(TypeLess_ptr buffer, int size);
    void R_SetVrect(vRect_p pvrect, vRect_p pvrectin, int lineadj);


#ifdef __cplusplus
}
#endif