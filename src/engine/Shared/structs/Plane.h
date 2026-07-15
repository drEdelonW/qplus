#pragma once

#include "types.h"
#include "vector.h"
#include "BBox.h"
#include "assert.h"

typedef
enum {
    // 0–2 are axial planes
    PLANE_X = 0u,
    PLANE_Y = 1u,
    PLANE_Z = 2u,

    // 3–5 are non-axial planes snapped to the nearest
    PLANE_ANYX = 3u,
    PLANE_ANYY = 4u,
    PLANE_ANYZ = 5u
} PlaneType_t; // should be size int on 32 os


// it was [mplane_t]
// Plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    vec3_t      normal;
    float       dist;
    PlaneType_t type;       // for texture axis selection and fast side tests
    uint8_t     signbits;   // signx | signy<<1 | signz<<1
    // uint8_t     pad[2];
} mPlane_t;
typedef mPlane_t* mPlane_p;


// it was [dplane_t]
typedef struct {
    vec3_t      normal;
    float       dist;
    PlaneType_t type;  // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dPlane_t;     STATIC_ASSERT_SIZE(dPlane_t, 5 * 4); // 20
typedef dPlane_t* dPlane_p;


typedef enum {
    PsFront     = 0,
    PsBack      = 1,
    PlaneSides  = 2
} PlaneSide_t;

typedef enum {  // TODO: rework this trash
    BPsNone  = 0,               // no side determined yet
    BPsFront = 1 << PsFront,    // box is entirely on the positive (front) side of the plane
    BPsBack  = 1 << PsBack,     // box is entirely on the negative (back) side of the plane
    BPsBoth  = BPsFront | BPsBack, // box straddles the plane; can't trivially cull
} BoxPlaneSide_t;

#include "Lump.h"
#ifdef __cplusplus
extern "C" {
#endif

    BoxPlaneSide_t BoxOnPlaneSide(BBox_t eBB, mPlane_p plane);
    void Mod_LoadPlanes(Lump_p l);

#ifdef __cplusplus
}
#endif

#define BOX_ON_PLANE_SIDE(bb, p)                                        \
    (((p)->type < PLANE_ANYX)? (                              \
        ((p)->dist <= ((bb).mins).v[(p)->type])?    \
            BPsFront : (                                                 \
                ((p)->dist >= ((bb).maxs).v[(p)->type])?      \
                    BPsBack : BPsBoth )                                   \
    ) : BoxOnPlaneSide( (bb), (p)))
