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

#include "Particle.h"

#ifdef GLQUAKE
#   include "qOpenGL.h"
#   include "cvar_q1.h"
#else
#   include "r_shared.h"
#   include "render.h"
#endif
#include "client.h"
#include "common.h"
#include "console.h"
#include "mathlib.h"
#include "q_tools.h"
#include "msg.h"
#include <stdlib.h>
#include "z_hunk.h"

int  ramp1[8] = { 0x6F, 0x6D, 0x6B, 0x69, 0x67, 0x65, 0x63, 0x61 }; // pt_explode
int  ramp2[8] = { 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x68, 0x66 }; // pt_explode2
int  ramp3[6] = { 0x6D, 0x6B, 0x06, 0x05, 0x04, 0x03 };             // pt_fire

static Particle_p _activeParticles;
static Particle_p _freeParticles;
static Particle_p _particles;
static int _rNumParticles;

Basis_t r_p;

/*
===============
R_InitParticles
===============
*/
void R_InitParticles() {
    int i = COM_CheckParm("-particles");

    if (i) {
        _rNumParticles = (int)(Q_atoi(com.argv[i + 1]));
        if (_rNumParticles < ABSOLUTE_MIN_PARTICLES)
            _rNumParticles = ABSOLUTE_MIN_PARTICLES;
    }
    else { _rNumParticles = MAX_PARTICLES; }

    _particles = (Particle_p)Hunk_AllocName(_rNumParticles * sizeof(Particle_t), "particles");
}

#ifdef QUAKE2
void R_DarkFieldParticles(r_Entity_p ent) {
    vec3_t org = ent->origin;
    for (int i = -16; i < 16; i += 8)
        for (int j = -16; j < 16; j += 8)
            for (int k = 0; k < 32; k += 8) {
                if (!_freeParticles)        return;

                Particle_p prt = _freeParticles;
                _freeParticles = prt->next;
                prt->next = _activeParticles;
                _activeParticles = prt;

                prt->die = cl.time + 0.2 + (rand() & 7) * 0.02;
                prt->color = 150 + rand() % 6;
                prt->type = pt_slowgrav;

                vec3_t  dir = {
                    j * 8,
                    i * 8,
                    k * 8
                };
                prt->org = {
                    .x = org.x + i + (rand() & 3),
                    .y = org.y + j + (rand() & 3),
                    .z = org.z + k + (rand() & 3)
                };

                VectorNormalize(dir);
                float vel = 50 + (rand() & 63);
                prt->vel = VectorScale(dir, vel);
            }
}
#endif


/*
===============
R_EntityParticles
===============
*/

#define NUMVERTEXNORMALS 162
static vec3_t _aVelocities[NUMVERTEXNORMALS];
static float _beamLength = 16.0f;

void R_EntityParticles(r_Entity_p ent) {
    if (!_aVelocities[0].x) {
        for (int i = 0; i < (NUMVERTEXNORMALS * 3); i++) {
            _aVelocities[0].v[i] = (rand() & 255) * 0.01;
        }
    }

    float dist = 64.0f;
    for (int i = 0; i < NUMVERTEXNORMALS; i++) {
        float angle = cl.time * _aVelocities[i].x;
        float sy = sin(angle);
        float cy = cos(angle);
        angle = cl.time * _aVelocities[i].y;
        float sp = sin(angle);
        float cp = cos(angle);
#if 0 // it was disabled
        angle = cl.time * _aVelocities[i].z;
        float sr = sin(angle);
        float cr = cos(angle);
#endif

        vec3_t forward = {
            .x = cp * cy,
            .y = cp * sy,
            .z = -sp
        };

        if (!_freeParticles)    return;

        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        prt->die = cl.time + 0.01;
        prt->color = 0x6F;
        prt->type = pt_explode;

        prt->org = VectorMA(VectorMA(
            ent->origin,
            dist, r_avertexnormals[i]),
            _beamLength, forward
        );
    }
}


/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles() {
    _freeParticles = &_particles[0];
    _activeParticles = NULL;

    for (int i = 0;i < _rNumParticles; i++)
        _particles[i].next = &_particles[i + 1];

    _particles[_rNumParticles - 1].next = NULL;
}

cString SV_GetName();   // TODO: adjust header inclusion

void R_ReadPointFile_f() {
    char name[MAX_OSPATH];
    snprintf(name, sizeof(name), "maps/%s.pts", SV_GetName());

    FILE* pntFile;
    COM_FOpenFile(name, &pntFile);
    if (!pntFile) { Con_Printf("couldn't open %s\n", name); return; }

    Con_Printf("Reading %s...\n", name);
    int c = 0;
    for (;; ) {
        vec3_t org;
        if (fscanf(pntFile, "%f %f %f\n", &org.x, &org.y, &org.z) != 3) break;
        c++;

        if (!_freeParticles) { Con_Printf("Not enough free particles\n"); break; }
        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        prt->die = 99999;
        prt->color = (-c) & 15;
        prt->type = pt_static;
        prt->vel = vec3_origin;
        prt->org = org;
    }

    fclose(pntFile);
    Con_Printf("%i points read\n", c);
}

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect() {
    vec3_t org = {
        .x = MSG_ReadCoord(),
        .y = MSG_ReadCoord(),
        .z = MSG_ReadCoord()
    };

    vec3_t dir = {
        .x = MSG_ReadChar(),
        .y = MSG_ReadChar(),
        .z = MSG_ReadChar()
    };
    dir = VectorScale(dir, (1.0f / 16.0f));

    int msgcount = MSG_ReadByte();
    int color = MSG_ReadByte();

    int count = (msgcount == 255) ? 1024 : msgcount;
    R_RunParticleEffect(org, dir, color, count);
}

/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion(vec3_t org) {
    for (int i = 0; i < 1024; i++) {
        if (!_freeParticles)        return;

        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        prt->die = cl.time + 5;
        prt->color = ramp1[0];
        prt->ramp = rand() & 3;
        prt->type = (i & 1) ? pt_explode : pt_explode2;
        for (int j = 0; j < VECT_DIM; j++) {
            prt->org.v[j] = org.v[j] + ((rand() % 32) - 16);
            prt->vel.v[j] = (rand() % 512) - 256;
        }
    }
}

/*
===============
R_ParticleExplosion2

===============
*/
void R_ParticleExplosion2(vec3_t org, int colorStart, int colorLength) {
    int colorMod = 0;
    for (int i = 0; i < 512; i++) {
        if (!_freeParticles)        return;

        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        prt->die = cl.time + 0.3;
        prt->color = colorStart + (colorMod % colorLength);
        colorMod++;

        prt->type = pt_blob;
        for (int j = 0; j < VECT_DIM; j++) {
            prt->org.v[j] = org.v[j] + ((rand() % 32) - 16);
            prt->vel.v[j] = (rand() % 512) - 256;
        }
    }
}

/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion(vec3_t org) {
    for (int i = 0; i < 1024; i++) {
        if (!_freeParticles)        return;

        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        prt->die = cl.time + 1 + (rand() & 8) * 0.05;
        prt->type = (i & 1) ? pt_blob : pt_blob2;
        prt->color = ((i & 1) ? 66 : 150) + rand() % 6;
        for (int j = 0; j < VECT_DIM; j++) {
            prt->org.v[j] = org.v[j] + ((rand() % 32) - 16);
            prt->vel.v[j] = (rand() % 512) - 256;
        }
    }
}

/*
===============
R_RunParticleEffect

===============
*/
void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count) {
    for (int i = 0; i < count; i++) {
        if (!_freeParticles)        return;

        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        if (count == 1024) { // rocket explosion
            prt->die = cl.time + 5;
            prt->color = ramp1[0];
            prt->ramp = rand() & 3;
            if (i & 1) {
                prt->type = pt_explode;
                for (int j = 0; j < VECT_DIM; j++) {
                    prt->org.v[j] = org.v[j] + ((rand() % 32) - 16);
                    prt->vel.v[j] = (rand() % 512) - 256;
                }
            }
            else {
                prt->type = pt_explode2;
                for (int j = 0; j < VECT_DIM; j++) {
                    prt->org.v[j] = org.v[j] + ((rand() % 32) - 16);
                    prt->vel.v[j] = (rand() % 512) - 256;
                }
            }
        }
        else {
            prt->die = cl.time + 0.1 * (rand() % 5);
            prt->color = (color & ~7) + (rand() & 7);
            prt->type = pt_slowgrav;
            for (int j = 0; j < VECT_DIM; j++) {
                prt->org.v[j] = org.v[j] + ((rand() & 15) - 8);
                prt->vel.v[j] = dir.v[j] * 15;// + (rand()%300)-150;
            }
        }
    }
}


/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash(vec3_t org) {
    for (int i = -16; i < 16; i++)
        for (int j = -16; j < 16; j++)
            for (int k = 0; k < 1; k++) {
                if (!_freeParticles)    return;

                Particle_p prt = _freeParticles;
                _freeParticles = prt->next;
                prt->next = _activeParticles;
                _activeParticles = prt;

                prt->die = cl.time + 2 + (rand() & 31) * 0.02;
                prt->color = 224 + (rand() & 7);
                prt->type = pt_slowgrav;

                vec3_t  dir = {
                    .x = j * 8 + (rand() & 7),
                    .y = i * 8 + (rand() & 7),
                    .z = 256.0f
                };

                prt->org.x = org.x + dir.x;
                prt->org.y = org.y + dir.y;
                prt->org.z = org.z + (rand() & 63);

                VectorNormalize(&dir);
                float vel = 50 + (rand() & 63);
                prt->vel = VectorScale(dir, vel);
            }
}

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash(vec3_t org) {
    for (int i = -16; i < 16; i += 4)
        for (int j = -16; j < 16; j += 4)
            for (int k = -24; k < 32; k += 4) {
                if (!_freeParticles)    return;
                Particle_p prt = _freeParticles;
                _freeParticles = prt->next;
                prt->next = _activeParticles;
                _activeParticles = prt;

                prt->die = cl.time + 0.2 + (rand() & 7) * 0.02;
                prt->color = 7 + (rand() & 7);
                prt->type = pt_slowgrav;

                vec3_t dir = {
                    .x = j * 8,
                    .y = i * 8,
                    .z = k * 8
                };

                prt->org.x = org.x + i + (rand() & 3);
                prt->org.y = org.y + j + (rand() & 3);
                prt->org.z = org.z + k + (rand() & 3);

                VectorNormalize(&dir);
                float vel = 50 + (rand() & 63);
                prt->vel = VectorScale(dir, vel);
            }
}

void R_RocketTrail(vec3_t start, vec3_t end, RocketTrailType type) {
    static int tracercount;

    vec3_t vec = VectorSubtract(end, start);
    float len = VectorNormalize(&vec);

    int dec;
    if (type < 128)
        dec = 3;
    else {
        dec = 1;
        type -= 128;
    }

    while (len > 0) {
        len -= dec;

        if (!_freeParticles)    return;

        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        prt->vel = vec3_origin;
        prt->die = cl.time + 2;

        switch (type) {
        case RT_ROCKET: {// rocket trail
            prt->ramp = (rand() & 3);
            prt->color = ramp3[(int)prt->ramp];
            prt->type = pt_fire;
            for (int j = 0; j < VECT_DIM; j++)
                prt->org.v[j] = start.v[j] + ((rand() % 6) - 3);
        } break;

        case RT_GRENADE: {// smoke smoke
            prt->ramp = (rand() & 3) + 2;
            prt->color = ramp3[(int)prt->ramp];
            prt->type = pt_fire;
            for (int j = 0; j < VECT_DIM; j++)
                prt->org.v[j] = start.v[j] + ((rand() % 6) - 3);
        } break;

        case RT_GIB: {// blood
            prt->type = pt_grav;
            prt->color = 67 + (rand() & 3);
            for (int j = 0; j < VECT_DIM; j++)
                prt->org.v[j] = start.v[j] + ((rand() % 6) - 3);
        } break;

        case RT_TRACER:
        case RT_TRACER2: {// tracer
            prt->die = cl.time + 0.5;
            prt->type = pt_static;
            prt->color = ((type == 3) ? 52 : 230) +
                ((tracercount & 4) << 1);

            tracercount++;

            prt->org = start;
            if (tracercount & 1) {
                prt->vel.x = 30 * vec.y;
                prt->vel.y = 30 * -vec.x;
            }
            else {
                prt->vel.x = 30 * -vec.y;
                prt->vel.y = 30 * vec.x;
            }
        } break;

        case RT_ZOMGIB: {// slight blood
            prt->type = pt_grav;
            prt->color = 67 + (rand() & 3);
            for (int j = 0; j < VECT_DIM; j++)
                prt->org.v[j] = start.v[j] + ((rand() % 6) - 3);
            len -= 3;
        } break;

        case RT_TRACER3: {// voor trail
            prt->color = 9 * 16 + 8 + (rand() & 3);
            prt->type = pt_static;
            prt->die = cl.time + 0.3;
            for (int j = 0; j < VECT_DIM; j++)
                prt->org.v[j] = start.v[j] + ((rand() & 15) - 8);
        } break;
        }


        start = VectorAdd(start, vec);
    }
}

static Particle_p Particle_KillFromHead(Particle_p head) {
    while (head &&
        (head->die < cl.time)
        ) {
        Particle_p kill = head;
        head = kill->next;
        kill->next = _freeParticles;
        _freeParticles = kill;
    }
    return head;
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles() {
    D_StartParticles(); {
        float frametime = cl.time - cl.oldtime;
        float time3 = frametime * 15;
        float time2 = frametime * 10; // 15;
        float time1 = frametime * 5;
        float grav = frametime * sv_gravity.value * 0.05;
        float dvel = frametime * 4;

        _activeParticles = Particle_KillFromHead(_activeParticles);

        Particle_p prt = _activeParticles;
        for (; prt; prt = prt->next) {
            prt->next = Particle_KillFromHead(prt->next);

            D_DrawParticle(prt);

            prt->org = VectorMA(prt->org, frametime, prt->vel);

            switch (prt->type) {
            case pt_static:     break;
            case pt_fire: {
                prt->ramp += time1;
                if (prt->ramp >= 6.0f)  prt->die = -1;
                else                    prt->color = ramp3[(int)prt->ramp];
                prt->vel.z += grav; // fly up
            } break;

            case pt_explode: {
                prt->ramp += time2;
                if (prt->ramp >= 8.0f)  prt->die = -1;
                else                    prt->color = ramp1[(int)prt->ramp];
#if 0
                for (int i = 0; i < VECT_DIM; i++)
                    prt->vel.v[i] += prt->vel.v[i] * dvel;
#else
                prt->vel = VectorMA(prt->vel, dvel, prt->vel);
#endif
                prt->vel.z -= grav; // fall down
            } break;

            case pt_explode2: {
                prt->ramp += time3;
                if (prt->ramp >= 8.0f)  prt->die = -1;
                else                    prt->color = ramp2[(int)prt->ramp];
#if 0
                for (int i = 0; i < VECT_DIM; i++)
                    prt->vel.v[i] -= prt->vel.v[i] * frametime;
#else
                prt->vel = VectorMA(prt->vel, -frametime, prt->vel);
#endif
                prt->vel.z -= grav; // fall down
            } break;

            case pt_blob: {
#if 0
                for (int i = 0; i < VECT_DIM; i++)
                    prt->vel.v[i] += prt->vel.v[i] * dvel;
#else
                prt->vel = VectorMA(prt->vel, dvel, prt->vel);
#endif
                prt->vel.z -= grav; // fall down
            } break;

            case pt_blob2: {
                for (int i = 0; i < 2; i++)
                    prt->vel.v[i] -= prt->vel.v[i] * dvel;
                prt->vel.z -= grav; // fall down
            } break;

            case pt_grav: {
#ifdef QUAKE2
                prt->vel.z -= grav * 20;
#endif
            } break;

            case pt_slowgrav: {
                prt->vel.z -= grav; // fall down
            } break;
            }
        }

    } D_EndParticles();
}

