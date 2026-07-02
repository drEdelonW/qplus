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
// cl_tent.c -- client side temporary entities

#include "client.h"
#include "sound.h"
#include <string.h>
#include <stdlib.h>
#include "msg.h"
#include "model.h"
#include "host.h"
#include "console.h"
#include "mathlib.h"
#include "render.h"
#include "Light.h"
#include "vector_tools.h"

//
// temp entity events
//
typedef enum {
    TE_SPIKE = 0u,                  // 0  - spike hitting wall (tink/ricochet sound)
    TE_SUPERSPIKE,                  // 1  - super spike hitting wall
    TE_GUNSHOT,                     // 2  - bullet hitting wall
    TE_EXPLOSION,                   // 3  - rocket explosion
    TE_TAREXPLOSION,                // 4  - tarbaby explosion
    TE_LIGHTNING1,                  // 5  - lightning bolt (progs/bolt.mdl)
    TE_LIGHTNING2,                  // 6  - lightning bolt (progs/bolt2.mdl)
    TE_WIZSPIKE,                    // 7  - spike hitting wall (wizard hit sound)
    TE_KNIGHTSPIKE,                 // 8  - spike hitting wall (knight hit sound)
    TE_LIGHTNING3,                  // 9  - lightning bolt (progs/bolt3.mdl)
    TE_LAVASPLASH,                  // 10 - lava splash
    TE_TELEPORT,                    // 11 - teleport splash
    TE_EXPLOSION2,                  // 12 - color mapped explosion

    // PGM 01/21/97
    TE_BEAM,                        // 13 - grappling hook beam (progs/beam.mdl)
    // PGM 01/21/97

#ifdef QUAKE2
    TE_IMPLOSION,                   // 14 - implosion
    TE_RAILTRAIL,                   // 15 - railgun trail
#endif
} TempEntEvent_t;

int         num_temp_entities;
r_Entity_t  cl_temp_entities[MAX_TEMP_ENTITIES];

typedef struct {
    sfx_p wizhit;
    sfx_p knighthit;
    sfx_p tink1;
    sfx_p ric1;
    sfx_p ric2;
    sfx_p ric3;
    sfx_p r_exp3;
#ifdef QUAKE2
    sfx_p imp;
    sfx_p rail;
#endif
} clSFX_t;
clSFX_t cl_sfx;
/*
=================
CL_ParseTEnt
=================
*/
void CL_InitTEnts() {
    cl_sfx = (clSFX_t){
        .wizhit = S_PrecacheSound("wizard/hit.wav"),
        .knighthit = S_PrecacheSound("hknight/hit.wav"),
        .tink1 = S_PrecacheSound("weapons/tink1.wav"),
        .ric1 = S_PrecacheSound("weapons/ric1.wav"),
        .ric2 = S_PrecacheSound("weapons/ric2.wav"),
        .ric3 = S_PrecacheSound("weapons/ric3.wav"),
        .r_exp3 = S_PrecacheSound("weapons/r_exp3.wav"),
#ifdef QUAKE2
        .imp = S_PrecacheSound("shambler/sattck1.wav"),
        .rail = S_PrecacheSound("weapons/lstart.wav"),
#endif
    };
}

#include "Beam.h"
Beam_t    cl_beams[MAX_BEAMS];
/*
=================
CL_ParseBeam
=================
*/
void CL_ParseBeam(Model_p m) {
    int16_t ent = MSG_ReadShort();

    vec3_t  start = MSG_ReadVector();
    vec3_t  end = MSG_ReadVector();

    // override any beam with the same entity
    for (int i = 0; i < MAX_BEAMS; i++)
        if (cl_beams[i].entity == ent) {
            cl_beams[i] = (Beam_t){
                .entity = ent,
                .model = m,
                .endtime = (LegDt_t)(GetClSimTime() + 0.2f),
                .start = start,
                .end = end,
            };
            return;
        }

    // find a free beam
    for (int i = 0; i < MAX_BEAMS; i++)
        if (!(cl_beams[i].model) ||
            (cl_beams[i].endtime < GetClSimTime())
            ) {
            cl_beams[i] = (Beam_t){
                .entity = ent,
                .model = m,
                .endtime = (LegDt_t)(GetClSimTime() + 0.2f),
                .start = start,
                .end = end,
            };
            return;
        }

    Con_Printf("beam list overflow!\n");
}

/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt() {
    TempEntEvent_t type = MSG_ReadByte();
    switch (type) {
    case TE_WIZSPIKE: {      // spike hitting wall
        vec3_t  pos = MSG_ReadVector();
        R_RunParticleEffect(pos, vec3_origin, 20, 30);
        S_StartSound(-1, 0, cl_sfx.wizhit, pos, 1, 1);
    } break;

    case TE_KNIGHTSPIKE: {      // spike hitting wall
        vec3_t  pos = MSG_ReadVector();
        R_RunParticleEffect(pos, vec3_origin, 226, 20);
        S_StartSound(-1, 0, cl_sfx.knighthit, pos, 1, 1);
    } break;

    case TE_SPIKE: {    // spike hitting wall
        vec3_t pos = MSG_ReadVector();
#ifdef GLTEST
        Test_Spawn(pos);
#else
        R_RunParticleEffect(pos, vec3_origin, 0, 10);
#endif
        if (rand() % 5)         S_StartSound(-1, 0, cl_sfx.tink1, pos, 1, 1);
        else {
            int rnd = rand() & 3;
            if (rnd == 1)       S_StartSound(-1, 0, cl_sfx.ric1, pos, 1, 1);
            else if (rnd == 2)  S_StartSound(-1, 0, cl_sfx.ric2, pos, 1, 1);
            else                S_StartSound(-1, 0, cl_sfx.ric3, pos, 1, 1);
        }
    } break;
    case TE_SUPERSPIKE: {    // super spike hitting wall
        vec3_t pos = MSG_ReadVector();
        R_RunParticleEffect(pos, vec3_origin, 0, 20);

        if (rand() % 5)     S_StartSound(-1, 0, cl_sfx.tink1, pos, 1, 1);
        else {
            int rnd = rand() & 3;
            if (rnd == 1)       S_StartSound(-1, 0, cl_sfx.ric1, pos, 1, 1);
            else if (rnd == 2)  S_StartSound(-1, 0, cl_sfx.ric2, pos, 1, 1);
            else                S_StartSound(-1, 0, cl_sfx.ric3, pos, 1, 1);
        }
    } break;

    case TE_GUNSHOT: {      // bullet hitting wall
        vec3_t pos = MSG_ReadVector();
        R_RunParticleEffect(pos, vec3_origin, 0, 20);
    } break;

    case TE_EXPLOSION: {      // rocket explosion
        vec3_t pos = MSG_ReadVector();
        R_ParticleExplosion(pos);
        dLight_p dl;
        *(dl = CL_AllocDlight(0)) = (dLight_t){
            .origin = pos,
            .radius = 350.0f,
            .die = (LegDt_t)(GetClSimTime() + 0.5f),
            .decay = 300.0f,
            .key = 0,
        };
        S_StartSound(-1, 0, cl_sfx.r_exp3, pos, 1, 1);
    } break;

    case TE_TAREXPLOSION: {      // tarbaby explosion
        vec3_t pos = MSG_ReadVector();
        R_BlobExplosion(pos);

        S_StartSound(-1, 0, cl_sfx.r_exp3, pos, 1, 1);
    } break;

    case TE_LIGHTNING1:     CL_ParseBeam(Mod_ForName("progs/bolt.mdl", true));      break;  // lightning bolts
    case TE_LIGHTNING2:     CL_ParseBeam(Mod_ForName("progs/bolt2.mdl", true));     break;  // lightning bolts
    case TE_LIGHTNING3:     CL_ParseBeam(Mod_ForName("progs/bolt3.mdl", true));     break;  // lightning bolts
        // PGM 01/21/97
    case TE_BEAM:           CL_ParseBeam(Mod_ForName("progs/beam.mdl", true));      break;  // grappling hook beam
        // PGM 01/21/97

    case TE_LAVASPLASH: {
        vec3_t pos = MSG_ReadVector();
        R_LavaSplash(pos);
    } break;

    case TE_TELEPORT: {
        vec3_t pos = MSG_ReadVector();
        R_TeleportSplash(pos);
    } break;

    case TE_EXPLOSION2: {        // color mapped explosion
        vec3_t pos = MSG_ReadVector();
        int colorStart = MSG_ReadByte();
        int colorLength = MSG_ReadByte();
        R_ParticleExplosion2(pos, colorStart, colorLength);
        dLight_p dl;
        *(dl = CL_AllocDlight(0)) = (dLight_t){
            .origin = pos,
            .radius = 350.0f,
            .die = (LegDt_t)(GetClSimTime() + 0.5f),
            .decay = 300.0f,
            .key = 0,
        };
        S_StartSound(-1, 0, cl_sfx.r_exp3, pos, 1, 1);
    } break;

#ifdef QUAKE2
    case TE_IMPLOSION: {
        vec3_t pos = MSG_ReadVector();
        S_StartSound(-1, 0, cl_sfx.imp, pos, 1, 1);
    } break;

    case TE_RAILTRAIL: {
        vec3_t pos = MSG_ReadVector();
        vec3_t endpos = MSG_ReadVector();
        S_StartSound(-1, 0, cl_sfx.rail, pos, 1, 1);
        S_StartSound(-1, 1, cl_sfx.r_exp3, endpos, 1, 1);
        R_RocketTrail(pos, endpos, 0 + 128);
        R_ParticleExplosion(endpos);
        dLight_p dl;
        *(dl = CL_AllocDlight(-1)) = (dLight_t){
            .origin = endpos,
            .radius = 350.0f,
            .die = (LegDt_t)(GetClSimTime() + 0.5f),
            .decay = 300.0f,
            .key = -1;
        };
    } break;
#endif

    default:    Host_SysError("CL_ParseTEnt: bad type");
}
}


/*
=================
CL_NewTempEntity
=================
*/
r_Entity_p CL_NewTempEntity() {
    if ((cl_numvisedicts == MAX_VISEDICTS) ||
        (num_temp_entities == MAX_TEMP_ENTITIES)
        )
        return NULL;

    r_Entity_p ent = &cl_temp_entities[num_temp_entities];
    memset(ent, 0, sizeof(*ent));
    num_temp_entities++;
    cl_visedicts[cl_numvisedicts] = ent;
    cl_numvisedicts++;

    ent->colormap = vid.colormap;
    return ent;
}


/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts() {
    num_temp_entities = 0;

    // update lightning
    Beam_p b = cl_beams;
    for (int i = 0; i < MAX_BEAMS; i++, b++) {
        if (!b->model ||
            (b->endtime < GetClSimTime()))
            continue;

        // if coming from the player, update the start position
        if (b->entity == cl.viewentity) {
            b->start = cl_entities[cl.viewentity].origin;
        }

        // calculate pitch and yaw
        vec3_t dist = VectorSubtract(b->end, b->start);

        // float yaw, pitch;   // TODO: wrap to vec3_t
        ang3_t tV;
        if ((dist.y == 0.0f) &&
            (dist.x == 0.0f)
            ) {
            tV.yaw = 0.0f;
            if (dist.z > 0.0f)  tV.pitch = 90.0f;
            else                tV.pitch = 270.0f;
        }
        else {
            tV.yaw = (float)(RAD2DEG(atan2(dist.y, dist.x)));
            if (tV.yaw < 0.0f)
                tV.yaw += 360.0f;

            float forward = (float)sqrt((dist.x * dist.x) + (dist.y * dist.y));
            tV.pitch = (float)(RAD2DEG(atan2(dist.z, forward)));
            if (tV.pitch < 0)
                tV.pitch += 360;
        }

        // add new entities for the lightning
        vec3_t org = b->start;
        float d = VectorNormalize(&dist);
        while (d > 0) {
            r_Entity_p  ent = CL_NewTempEntity();
            if (!ent)       return;

            tV.roll = (float)(rand() % 360);
            ent->origin = org;
            ent->model = b->model;
            ent->angles = tV;

            org = VectorMA(org, 30.0f, dist);
            d -= 30.0f;
        }
    }

}


