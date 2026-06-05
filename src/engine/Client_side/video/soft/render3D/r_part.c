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

#ifndef GLQUAKE
#   include "r_shared.h"
#else
#   include "qOpenGL.h"
#   include "cvar_q1.h"
#endif
#include "client.h"
#include "common.h"
#include "console.h"
#include "mathlib.h"
#include "Particle.h"
#include "q_tools.h"
#include "msg.h"
#include <stdlib.h>
#include "z_hunk.h"

int  ramp1[8] = { 0x6F, 0x6D, 0x6B, 0x69, 0x67, 0x65, 0x63, 0x61 };
int  ramp2[8] = { 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x68, 0x66 };
int  ramp3[8] = { 0x6D, 0x6B, 0x06, 0x05, 0x04, 0x03, 0x00, 0x00 };

static Particle_p _activeParticles;
static Particle_p _freeParticles;
static Particle_p _particles;
static int _rNumParticles;

vec3_t   r_pright, r_pup, r_ppn;


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
    vec3_t org = {
        ent->origin[0],
        ent->origin[1],
        ent->origin[2]
    };
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
                prt->org[0] = org[0] + i + (rand() & 3);
                prt->org[1] = org[1] + j + (rand() & 3);
                prt->org[2] = org[2] + k + (rand() & 3);

                VectorNormalize(dir);
                float vel = 50 + (rand() & 63);
                VectorScale(dir, vel, prt->vel);
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
static float _beamLength = 16;
// vec3_t avelocity = { 23, 7, 3 }; // not used
// float partstep = 0.01;  // not used
// float timescale = 0.01; // not used

void R_EntityParticles(r_Entity_p ent) {
    if (!_aVelocities[0][0]) {
        for (int i = 0; i < (NUMVERTEXNORMALS * 3); i++) {
            _aVelocities[0][i] = (rand() & 255) * 0.01;
        }
    }

    float dist = 64;
    for (int i = 0; i < NUMVERTEXNORMALS; i++) {
        float angle = cl.time * _aVelocities[i][0];
        float sy = sin(angle);
        float cy = cos(angle);
        angle = cl.time * _aVelocities[i][1];
        float sp = sin(angle);
        float cp = cos(angle);
        // angle = cl.time * _aVelocities[i][2];
        // float sr = sin(angle);
        // float cr = cos(angle);

        vec3_t forward = {
            cp * cy,
            cp * sy,
            -sp
        };

        if (!_freeParticles)    return;

        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        prt->die = cl.time + 0.01;
        prt->color = 0x6F;
        prt->type = pt_explode;

        prt->org[0] = ent->origin[0] + r_avertexnormals[i][0] * dist + forward[0] * _beamLength;
        prt->org[1] = ent->origin[1] + r_avertexnormals[i][1] * dist + forward[1] * _beamLength;
        prt->org[2] = ent->origin[2] + r_avertexnormals[i][2] * dist + forward[2] * _beamLength;
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
        if (fscanf(pntFile, "%f %f %f\n", &org[0], &org[1], &org[2]) != 3) break;
        c++;

        if (!_freeParticles) { Con_Printf("Not enough free particles\n"); break; }
        Particle_p prt = _freeParticles;
        _freeParticles = prt->next;
        prt->next = _activeParticles;
        _activeParticles = prt;

        prt->die = 99999;
        prt->color = (-c) & 15;
        prt->type = pt_static;
        VectorCopy(vec3_origin, prt->vel);
        VectorCopy(org, prt->org);
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
        MSG_ReadCoord(),
        MSG_ReadCoord(),
        MSG_ReadCoord()
    };

    vec3_t dir = {
        MSG_ReadChar() * (1.0 / 16),
        MSG_ReadChar() * (1.0 / 16),
        MSG_ReadChar() * (1.0 / 16)
    };
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
            prt->org[j] = org[j] + ((rand() % 32) - 16);
            prt->vel[j] = (rand() % 512) - 256;
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
            prt->org[j] = org[j] + ((rand() % 32) - 16);
            prt->vel[j] = (rand() % 512) - 256;
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
            prt->org[j] = org[j] + ((rand() % 32) - 16);
            prt->vel[j] = (rand() % 512) - 256;
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
                    prt->org[j] = org[j] + ((rand() % 32) - 16);
                    prt->vel[j] = (rand() % 512) - 256;
                }
            }
            else {
                prt->type = pt_explode2;
                for (int j = 0; j < VECT_DIM; j++) {
                    prt->org[j] = org[j] + ((rand() % 32) - 16);
                    prt->vel[j] = (rand() % 512) - 256;
                }
            }
        }
        else {
            prt->die = cl.time + 0.1 * (rand() % 5);
            prt->color = (color & ~7) + (rand() & 7);
            prt->type = pt_slowgrav;
            for (int j = 0; j < VECT_DIM; j++) {
                prt->org[j] = org[j] + ((rand() & 15) - 8);
                prt->vel[j] = dir[j] * 15;// + (rand()%300)-150;
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
                    j * 8 + (rand() & 7),
                    i * 8 + (rand() & 7),
                    256
                };

                prt->org[0] = org[0] + dir[0];
                prt->org[1] = org[1] + dir[1];
                prt->org[2] = org[2] + (rand() & 63);

                VectorNormalize(dir);
                float vel = 50 + (rand() & 63);
                VectorScale(dir, vel, prt->vel);
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
                    j * 8,
                    i * 8,
                    k * 8
                };

                prt->org[0] = org[0] + i + (rand() & 3);
                prt->org[1] = org[1] + j + (rand() & 3);
                prt->org[2] = org[2] + k + (rand() & 3);

                VectorNormalize(dir);
                float vel = 50 + (rand() & 63);
                VectorScale(dir, vel, prt->vel);
            }
}

void R_RocketTrail(vec3_t start, vec3_t end, RocketTrailType type) {
    static int tracercount;

    vec3_t vec; VectorSubtract(end, start, vec);
    float len = VectorNormalize(vec);

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

        VectorCopy(vec3_origin, prt->vel);
        prt->die = cl.time + 2;

        switch (type) {
        case RT_ROCKET: // rocket trail
            prt->ramp = (rand() & 3);
            prt->color = ramp3[(int)prt->ramp];
            prt->type = pt_fire;
            for (int j = 0; j < VECT_DIM; j++)
                prt->org[j] = start[j] + ((rand() % 6) - 3);
            break;

        case RT_GRENADE: // smoke smoke
            prt->ramp = (rand() & 3) + 2;
            prt->color = ramp3[(int)prt->ramp];
            prt->type = pt_fire;
            for (int j = 0; j < VECT_DIM; j++)
                prt->org[j] = start[j] + ((rand() % 6) - 3);
            break;

        case RT_GIB: // blood
            prt->type = pt_grav;
            prt->color = 67 + (rand() & 3);
            for (int j = 0; j < VECT_DIM; j++)
                prt->org[j] = start[j] + ((rand() % 6) - 3);
            break;

        case RT_TRACER:
        case RT_TRACER2: // tracer
            prt->die = cl.time + 0.5;
            prt->type = pt_static;
            prt->color = ((type == 3) ? 52 : 230) +
                ((tracercount & 4) << 1);

            tracercount++;

            VectorCopy(start, prt->org);
            if (tracercount & 1) {
                prt->vel[0] = 30 * vec[1];
                prt->vel[1] = 30 * -vec[0];
            }
            else {
                prt->vel[0] = 30 * -vec[1];
                prt->vel[1] = 30 * vec[0];
            }
            break;

        case RT_ZOMGIB: // slight blood
            prt->type = pt_grav;
            prt->color = 67 + (rand() & 3);
            for (int j = 0; j < VECT_DIM; j++)
                prt->org[j] = start[j] + ((rand() % 6) - 3);
            len -= 3;
            break;

        case RT_TRACER3: // voor trail
            prt->color = 9 * 16 + 8 + (rand() & 3);
            prt->type = pt_static;
            prt->die = cl.time + 0.3;
            for (int j = 0; j < VECT_DIM; j++)
                prt->org[j] = start[j] + ((rand() & 15) - 8);
            break;
        }


        VectorAdd(start, vec, start);
    }
}


/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles() {
#ifdef GLQUAKE      // TODO: wrap this in D_StartParticles on GL_side

    GL_Bind(particletexture);
    glEnable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBegin(GL_TRIANGLES);

    vec3_t up;  VectorScale(vup, 1.5, up);
    vec3_t right;   VectorScale(vright, 1.5, right);
#else
    D_StartParticles();

    VectorScale(vright, xscaleshrink, r_pright);
    VectorScale(vup, yscaleshrink, r_pup);
    VectorCopy(vpn, r_ppn);
#endif
    float frametime = cl.time - cl.oldtime;
    float time3 = frametime * 15;
    float time2 = frametime * 10; // 15;
    float time1 = frametime * 5;
    float grav = frametime * sv_gravity.value * 0.05;
    float dvel = 4 * frametime;

    for (;; ) {
        Particle_p kill = _activeParticles;
        if (kill && (kill->die < cl.time)
            ) {
            _activeParticles = kill->next;
            kill->next = _freeParticles;
            _freeParticles = kill;
            continue;
        }
        break;
    }

    Particle_p prt = _activeParticles;
    for (; prt; prt = prt->next) {
        for (;; ) {
            Particle_p kill = prt->next;
            if (kill &&
                (kill->die < cl.time)
                ) {
                prt->next = kill->next;
                kill->next = _freeParticles;
                _freeParticles = kill;
                continue;
            }
            break;
        }

#ifdef GLQUAKE      // TODO: wrap this in D_DrawParticle on GL_side
        // hack a scale up to keep particles from disapearing
        float scale =
            (prt->org[0] - r_origin[0]) * vpn[0] +
            (prt->org[1] - r_origin[1]) * vpn[1] +
            (prt->org[2] - r_origin[2]) * vpn[2];
        if (scale < 20) scale = 1;
        else            scale = 1 + scale * 0.004;
        glColor3ubv((uint8_p)&d_8to24table[(int)prt->color]);
        glTexCoord2f(0, 0);
        glVertex3fv(prt->org);
        glTexCoord2f(1, 0);
        glVertex3f(
            prt->org[0] + up[0] * scale,
            prt->org[1] + up[1] * scale,
            prt->org[2] + up[2] * scale
        );
        glTexCoord2f(0, 1);
        glVertex3f(
            prt->org[0] + right[0] * scale,
            prt->org[1] + right[1] * scale,
            prt->org[2] + right[2] * scale
        );
#else
        D_DrawParticle(prt);
#endif
        prt->org[0] += prt->vel[0] * frametime; // prt->org += prt->vel * frametime;
        prt->org[1] += prt->vel[1] * frametime;
        prt->org[2] += prt->vel[2] * frametime;

        switch (prt->type) {
        case pt_static:     break;
        case pt_fire:
            prt->ramp += time1;
            if (prt->ramp >= 6)   prt->die = -1;
            else                prt->color = ramp3[(int)prt->ramp];
            prt->vel[2] += grav;
            break;

        case pt_explode:
            prt->ramp += time2;
            if (prt->ramp >= 8)   prt->die = -1;
            else                prt->color = ramp1[(int)prt->ramp];
            for (int i = 0; i < VECT_DIM; i++)
                prt->vel[i] += prt->vel[i] * dvel;
            prt->vel[2] -= grav;
            break;

        case pt_explode2:
            prt->ramp += time3;
            if (prt->ramp >= 8)   prt->die = -1;
            else                prt->color = ramp2[(int)prt->ramp];
            for (int i = 0; i < VECT_DIM; i++)
                prt->vel[i] -= prt->vel[i] * frametime;
            prt->vel[2] -= grav;
            break;

        case pt_blob:
            for (int i = 0; i < VECT_DIM; i++)
                prt->vel[i] += prt->vel[i] * dvel;
            prt->vel[2] -= grav;
            break;

        case pt_blob2:
            for (int i = 0; i < 2; i++)
                prt->vel[i] -= prt->vel[i] * dvel;
            prt->vel[2] -= grav;
            break;

        case pt_grav:
#ifdef QUAKE2
            prt->vel[2] -= grav * 20;
            break;
#endif
        case pt_slowgrav:
            prt->vel[2] -= grav;
            break;
        }
    }

#ifdef GLQUAKE      // TODO: wrap this in D_EndParticles on GL_side
    glEnd();
    glDisable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#else
    D_EndParticles();
#endif
}

