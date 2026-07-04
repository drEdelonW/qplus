#pragma once

#include "types.h"
#include "vector.h"
#include "BBox.h"
#include "assert.h"

typedef
#if 0
uint8_t
#else
enum {
    // 0–2 are axial planes
    PLANE_X = 0u,
    PLANE_Y = 1u,
    PLANE_Z = 2u,

    // 3–5 are non-axial planes snapped to the nearest
    PLANE_ANYX = 3u,
    PLANE_ANYY = 4u,
    PLANE_ANYZ = 5u
}
#endif
PlaneType_t; // should be size int on 32 os

// it was [mplane_t]
// Plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    vec3_t      normal;
    float       dist;
    PlaneType_t type;       // for texture axis selection and fast side tests
    uint8_t     signbits;   // signx + signy<<1 + signz<<1
    uint8_t     pad[2];
} mPlane_t;
typedef mPlane_t* mPlane_p;


// it was [dplane_t]
typedef struct {
    vec3_t      normal;
    float       dist;
    PlaneType_t type;  // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dPlane_t;     STATIC_ASSERT_SIZE(dPlane_t, 5 * 4); // 20
typedef dPlane_t* dPlane_p;

#include "Lump.h"

typedef enum {
    PsNone = 0,                // no side determined yet
    PsFront = 1 << 0,           // box is entirely on the positive (front) side of the plane
    PsBack = 1 << 1,           // box is entirely on the negative (back) side of the plane
    PsBoth = PsFront | PsBack, // box straddles the plane; can't trivially cull
} PlaneSide_t;

#ifdef __cplusplus
extern "C" {
#endif

    PlaneSide_t BoxOnPlaneSide(BBox_t eBB, mPlane_p plane);
    void Mod_LoadPlanes(Lump_p l);

#ifdef __cplusplus
}
#endif

#define BOX_ON_PLANE_SIDE(bb, p)                                        \
    (((p)->type < PLANE_ANYX)? (                              \
        ((p)->dist <= ((bb).mins).v[(p)->type])?    \
            PsFront : (                                                 \
                ((p)->dist >= ((bb).maxs).v[(p)->type])?      \
                    PsBack : PsBoth )                                   \
    ) : BoxOnPlaneSide( (bb), (p)))
