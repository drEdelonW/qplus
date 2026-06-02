#pragma once

#include "types.h"
#include "vector.h"
#include "assert.h"

// it was [mplane_t]
// Plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mPlane_s {
    vec3_t  normal;
    float   dist;
    uint8_t type;       // for texture axis selection and fast side tests
    uint8_t signbits;   // signx + signy<<1 + signz<<1
    uint8_t pad[2];
} mPlane_t;
typedef mPlane_t* mPlane_p;


typedef enum {
    // 0–2 are axial planes
    PLANE_X     = 0u,
    PLANE_Y     = 1u,
    PLANE_Z     = 2u,

    // 3–5 are non-axial planes snapped to the nearest
    PLANE_ANYX  = 3u,
    PLANE_ANYY  = 4u,
    PLANE_ANYZ  = 5u
} PlaneType_t; // should be size int on 32 os

// it was [dplane_t]
typedef struct {
    vec3_t      normal;
    float       dist;
    PlaneType_t type;  // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dPlane_t;
typedef dPlane_t* dPlane_p;
STATIC_ASSERT_SIZE(dPlane_t, 5*4); // 20

int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, mPlane_p plane);
