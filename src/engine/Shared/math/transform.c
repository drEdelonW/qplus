#include "transform.h"
#include <math.h>

Basis_t BS;
Basis_t base_BS;

void AngleVectors(ang3_t angles, vec3_p forward, vec3_p right, vec3_p up) {
#if 0
    angles = VectorScale(angles, (M_PI * 2.0f / 360.0f));
    float sy = sin(angles.yaw);
    float cy = cos(angles.yaw);
    float sp = sin(angles.pitch);
    float cp = cos(angles.pitch);
    float sr = sin(angles.roll);
    float cr = cos(angles.roll);

    /* yaw -> pitch -> roll (z -> y -> z) */ // TODO: recheck it
    *forward = (vec3_t){
        .x = cp * cy,
        .y = cp * sy,
        .z = -sp
    };
    *right = (vec3_t){
        .x = (-sr * sp * cy) + (cr * sy),
        .y = (-sr * sp * sy) + (-cr * cy),
        .z = (-sr * cp)
    };
    *up = (vec3_t){
        .x = (cr * sp * cy) + (sr * sy),
        .y = (cr * sp * sy) + (-sr * cy),
        .z = cr * cp
    };
#else
    /*
    * Decomposes Euler angles into an orthonormal orientation basis.
    *
    * Gimbal chain: Rz(yaw) -> Ry(pitch) -> Rx(roll)
    *   intrinsic ZYX: each rotation about the current body-local axis
    *   (equivalent to extrinsic XYZ; matrix form R = Rz * Ry * Rx)
    *
    * Quake rest-pose axes (yaw=pitch=roll=0):
    *   forward = +X  ( 1,  0,  0)
    *   right   = -Y  ( 0, -1,  0)   <- Y axis points LEFT
    *   up      = +Z  ( 0,  0,  1)
    *
    * Intermediate frames:
    *
    *   Ring 1 - Rz(yaw):
    *     fwd0 = ( cy,  sy,  0)
    *     rgt0 = ( sy, -cy,  0)   [= Rz(yaw) * (0,-1,0)]
    *     up0  = (  0,   0,  1)   [invariant]
    *
    *   Ring 2 - Ry(pitch), axis perpendicular to rgt0 in horizontal plane:
    *     fwd  = cp*fwd0 - sp*up0  ->  (cp*cy,  cp*sy,  -sp)   [roll-invariant]
    *     up1  = sp*fwd0 + cp*up0  ->  (sp*cy,  sp*sy,   cp)
    *     rgt0 unchanged (pitch axis is orthogonal to rgt0)
    *
    *   Ring 3 - Rx(roll), axis = fwd:
    *     rgt  = cr*rgt0 - sr*up1
    *     up   = sr*rgt0 + cr*up1
    */
    angles = AngleScale(angles, M_PI * 2.0f / 360.0f);

    const float sy = sinf(angles.yaw),   cy = cosf(angles.yaw);   /* ring 1: yaw   about Z */
    const float sp = sinf(angles.pitch), cp = cosf(angles.pitch); /* ring 2: pitch about Y */
    const float sr = sinf(angles.roll),  cr = cosf(angles.roll);  /* ring 3: roll  about X */

    /* Intermediate vectors from rings 1 and 2 */
    const vec3_t rgt0 = {{  sy,    -cy,    0  }};  /* right after yaw   */
    const vec3_t up1  = {{  sp*cy,  sp*sy, cp }};  /* up    after pitch */

    /* forward is the roll axis, so it is roll-invariant */
    *forward = (vec3_t){{ cp*cy,  cp*sy,  -sp }};

    /* ring 3: roll mixes rgt0 and up1 */
    *right = (vec3_t){{
        cr*rgt0.x - sr*up1.x,   /*  cr*sy  - sr*sp*cy */
        cr*rgt0.y - sr*up1.y,   /* -cr*cy  - sr*sp*sy */
        cr*rgt0.z - sr*up1.z,   /* -sr*cp             */
    }};
    *up = (vec3_t){{
        sr*rgt0.x + cr*up1.x,   /*  sr*sy  + cr*sp*cy */
        sr*rgt0.y + cr*up1.y,   /* -sr*cy  + cr*sp*sy */
        sr*rgt0.z + cr*up1.z,   /*  cr*cp             */
    }};
#endif
}


void AngleToBasis(ang3_t angles, Basis_p bs) {
    AngleVectors(angles, &bs->forward, &bs->right, &bs->up);
}

Basis_t GetBasis(ang3_t angles) {
    Basis_t out;
    AngleVectors(angles, &out.forward, &out.right, &out.up);
    return out;
}
