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

#include "sound.h"
#include <string.h>
#include "msg.h"
#include "host.h"
#include "protocol.h"
#include "console.h"
#include "client.h"
#include "mathlib.h"
#include "model.h"
#include <stdlib.h>

//
// temp entity events
//

typedef enum {
    TE_SPIKE        = 0u,
    TE_SUPERSPIKE   = 1u,
    TE_GUNSHOT      = 2u,
    TE_EXPLOSION    = 3u,
    TE_TAREXPLOSION = 4u,
    TE_LIGHTNING1   = 5u,
    TE_LIGHTNING2   = 6u,
    TE_WIZSPIKE     = 7u,
    TE_KNIGHTSPIKE  = 8u,
    TE_LIGHTNING3   = 9u,
    TE_LAVASPLASH   = 10u,
    TE_TELEPORT     = 11u,
    TE_EXPLOSION2   = 12u,

    // PGM 01/21/97
    TE_BEAM         = 13u,
    // PGM 01/21/97

#ifdef QUAKE2
    TE_IMPLOSION    = 14u,
    TE_RAILTRAIL    = 15u,
#endif
} te_t;

int      num_temp_entities;
r_Entity_t  cl_temp_entities[MAX_TEMP_ENTITIES];
Beam_t    cl_beams[MAX_BEAMS];

sfx_p cl_sfx_wizhit;
sfx_p cl_sfx_knighthit;
sfx_p cl_sfx_tink1;
sfx_p cl_sfx_ric1;
sfx_p cl_sfx_ric2;
sfx_p cl_sfx_ric3;
sfx_p cl_sfx_r_exp3;
#ifdef QUAKE2
sfx_p cl_sfx_imp;
sfx_p cl_sfx_rail;
#endif

/*
=================
CL_ParseTEnt
=================
*/
void CL_InitTEnts() {
    cl_sfx_wizhit = S_PrecacheSound("wizard/hit.wav");
    cl_sfx_knighthit = S_PrecacheSound("hknight/hit.wav");
    cl_sfx_tink1 = S_PrecacheSound("weapons/tink1.wav");
    cl_sfx_ric1 = S_PrecacheSound("weapons/ric1.wav");
    cl_sfx_ric2 = S_PrecacheSound("weapons/ric2.wav");
    cl_sfx_ric3 = S_PrecacheSound("weapons/ric3.wav");
    cl_sfx_r_exp3 = S_PrecacheSound("weapons/r_exp3.wav");
#ifdef QUAKE2
    cl_sfx_imp = S_PrecacheSound("shambler/sattck1.wav");
    cl_sfx_rail = S_PrecacheSound("weapons/lstart.wav");
#endif
}

/*
=================
CL_ParseBeam
=================
*/
void CL_ParseBeam(Model_p m) {
    int16_t ent = MSG_ReadShort();

    vec3_t  start = {
        MSG_ReadCoord(),
        MSG_ReadCoord(),
        MSG_ReadCoord()
    };
    vec3_t  end = {
        MSG_ReadCoord(),
        MSG_ReadCoord(),
        MSG_ReadCoord()
    };

    // override any beam with the same entity
    Beam_p  b = cl_beams;
    for (int i = 0; i < MAX_BEAMS; i++, b++)
        if (b->entity == ent) {
            b->entity = ent;
            b->model = m;
            b->endtime = (float)(cl.time + 0.2);
            VectorCopy(start, b->start);
            VectorCopy(end, b->end);
            return;
        }

    // find a free beam
    b = cl_beams;
    for (int i = 0; i < MAX_BEAMS; i++, b++)
        if (!b->model || (b->endtime < cl.time)) {
            b->entity = ent;
            b->model = m;
            b->endtime = (float)(cl.time + 0.2);
            VectorCopy(start, b->start);
            VectorCopy(end, b->end);
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
    te_t type = MSG_ReadByte();
    switch (type) {
    case TE_WIZSPIKE: {      // spike hitting wall
        vec3_t  pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        R_RunParticleEffect(pos, vec3_origin, 20, 30);
        S_StartSound(-1, 0, cl_sfx_wizhit, pos, 1, 1);
    } break;

    case TE_KNIGHTSPIKE: {      // spike hitting wall
        vec3_t  pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        R_RunParticleEffect(pos, vec3_origin, 226, 20);
        S_StartSound(-1, 0, cl_sfx_knighthit, pos, 1, 1);
    } break;

    case TE_SPIKE: {    // spike hitting wall
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
#ifdef GLTEST
        Test_Spawn(pos);
#else
        R_RunParticleEffect(pos, vec3_origin, 0, 10);
#endif
        if (rand() % 5)         S_StartSound(-1, 0, cl_sfx_tink1, pos, 1, 1);
        else {
            int rnd = rand() & 3;
            if (rnd == 1)       S_StartSound(-1, 0, cl_sfx_ric1, pos, 1, 1);
            else if (rnd == 2)  S_StartSound(-1, 0, cl_sfx_ric2, pos, 1, 1);
            else                S_StartSound(-1, 0, cl_sfx_ric3, pos, 1, 1);
        }
    } break;
    case TE_SUPERSPIKE: {    // super spike hitting wall
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        R_RunParticleEffect(pos, vec3_origin, 0, 20);

        if (rand() % 5)     S_StartSound(-1, 0, cl_sfx_tink1, pos, 1, 1);
        else {
            int rnd = rand() & 3;
            if (rnd == 1)       S_StartSound(-1, 0, cl_sfx_ric1, pos, 1, 1);
            else if (rnd == 2)  S_StartSound(-1, 0, cl_sfx_ric2, pos, 1, 1);
            else                S_StartSound(-1, 0, cl_sfx_ric3, pos, 1, 1);
        }
    } break;

    case TE_GUNSHOT: {      // bullet hitting wall
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        R_RunParticleEffect(pos, vec3_origin, 0, 20);
    } break;

    case TE_EXPLOSION: {      // rocket explosion
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        R_ParticleExplosion(pos);
        dLight_p dl = CL_AllocDlight(0);
        VectorCopy(pos, dl->origin);
        dl->radius = 350;
        dl->die = (float)(cl.time + 0.5);
        dl->decay = 300;
        S_StartSound(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
    } break;

    case TE_TAREXPLOSION: {      // tarbaby explosion
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        R_BlobExplosion(pos);

        S_StartSound(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
    } break;

    case TE_LIGHTNING1:     CL_ParseBeam(Mod_ForName("progs/bolt.mdl", true));      break;  // lightning bolts
    case TE_LIGHTNING2:     CL_ParseBeam(Mod_ForName("progs/bolt2.mdl", true));     break;  // lightning bolts
    case TE_LIGHTNING3:     CL_ParseBeam(Mod_ForName("progs/bolt3.mdl", true));     break;  // lightning bolts
        // PGM 01/21/97
    case TE_BEAM:           CL_ParseBeam(Mod_ForName("progs/beam.mdl", true));      break;  // grappling hook beam
        // PGM 01/21/97

    case TE_LAVASPLASH: {
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        R_LavaSplash(pos);
    } break;

    case TE_TELEPORT: {
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        R_TeleportSplash(pos);
    } break;

    case TE_EXPLOSION2: {        // color mapped explosion
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        int colorStart = MSG_ReadByte();
        int colorLength = MSG_ReadByte();
        R_ParticleExplosion2(pos, colorStart, colorLength);
        dLight_p dl = CL_AllocDlight(0);
        VectorCopy(pos, dl->origin);
        dl->radius = 350;
        dl->die = (float)(cl.time + 0.5);
        dl->decay = 300;
        S_StartSound(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
    } break;

#ifdef QUAKE2
    case TE_IMPLOSION: {
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        S_StartSound(-1, 0, cl_sfx_imp, pos, 1, 1);
    } break;

    case TE_RAILTRAIL: {
        vec3_t pos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        vec3_t endpos = {
            MSG_ReadCoord(),
            MSG_ReadCoord(),
            MSG_ReadCoord()
        };
        S_StartSound(-1, 0, cl_sfx_rail, pos, 1, 1);
        S_StartSound(-1, 1, cl_sfx_r_exp3, endpos, 1, 1);
        R_RocketTrail(pos, endpos, 0 + 128);
        R_ParticleExplosion(endpos);
        dLight_p dl = CL_AllocDlight(-1);
        VectorCopy(endpos, dl->origin);
        dl->radius = 350;
        dl->die = cl.time + 0.5;
        dl->decay = 300;
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
        (num_temp_entities == MAX_TEMP_ENTITIES))
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
            (b->endtime < cl.time))
            continue;

        // if coming from the player, update the start position
        if (b->entity == cl.viewentity) {
            VectorCopy(cl_entities[cl.viewentity].origin, b->start);
        }

        // calculate pitch and yaw
        vec3_t dist;    VectorSubtract(b->end, b->start, dist);

        float yaw, pitch;
        if ((dist[1] == 0) &&
            (dist[0] == 0)
            ) {
            yaw = 0;
            if (dist[2] > 0)    pitch = 90;
            else                pitch = 270;
        }
        else {
            yaw = (float)(atan2(dist[1], dist[0]) * 180 / M_PI);
            if (yaw < 0)
                yaw += 360;

            float forward = (float)sqrt(dist[0] * dist[0] + dist[1] * dist[1]);
            pitch = (float)(atan2(dist[2], forward) * 180 / M_PI);
            if (pitch < 0)
                pitch += 360;
        }

        // add new entities for the lightning
        vec3_t org; VectorCopy(b->start, org);
        float d = VectorNormalize(dist);
        while (d > 0) {
            r_Entity_p  ent = CL_NewTempEntity();
            if (!ent)
                return;
            VectorCopy(org, ent->origin);
            ent->model = b->model;
            ent->angles[0] = pitch;
            ent->angles[1] = yaw;
            ent->angles[2] = (float)(rand() % 360);

            for (i = 0; i < VECT_DIM; i++)
                org[i] += dist[i] * 30;
            d -= 30;
        }
    }

}


