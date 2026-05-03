#pragma once

#include "vector.h"

#define PARTICLE_Z_CLIP 8.0

typedef enum {
    pt_static,
    pt_grav,
    pt_slowgrav,
    pt_fire,
    pt_explode,
    pt_explode2,
    pt_blob,
    pt_blob2
} ParticleType_t;

// it was [particle_t]
// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct Particle_s Particle_t;
typedef Particle_t* Particle_p;
struct Particle_s {
    // driver-usable fields
    vec3_t          org;
    float           color;
    // drivers never touch the following fields
    Particle_p      next;
    vec3_t          vel;
    float           ramp;
    float           die;
    ParticleType_t  type;
};

void D_DrawParticle(Particle_p pparticle);
