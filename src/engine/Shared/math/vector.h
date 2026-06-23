#pragma once
#include "assert.h"
#include <stdbool.h>


typedef float vec_t;
STATIC_ASSERT_SIZE(vec_t, sizeof(float));

typedef enum {
    S_AX = 0u,
    T_AX = 1u,
    VECT_TX_DIM = 2u,
} axis_tx_e;
typedef union {
    struct { vec_t s, t; };
    // struct { vec_t u, v; }; // TODO: make by modern naming access
    vec_t v[VECT_TX_DIM];
} vec2_t;

typedef enum {
    X_AX = 0u,
    Y_AX = 1u,
    Z_AX = 2u,
    VECT_DIM = 3u,
} axis_e;

/* Euler angle indices: up/down, left/right, roll (fall over) */
enum {
    PITCH       = 0u,  /* up/down */
    YAW         = 1u,  /* left/right */
    ROLL        = 2u,  /* roll (fall over) */
    ANGLES_COUNT = 3u
};

typedef union {
    struct { vec_t x, y, z; };
    struct { float pitch, yaw, roll; };
    vec_t v[VECT_DIM];
} vec3_t;
STATIC_ASSERT_SIZE(vec3_t, 3 * sizeof(vec_t));
typedef vec3_t* vec3_p;

#if 1   // TODO: rework textured vertex
typedef vec_t vec5_t[5];    // vec3_t(x/y/z) + vec2(s/t)
#else
typedef union {
    struct {
        vec3_t vx;  //  Vertex          vec3_t(x/y/z)
        vec2_t vt;  //  Texture Coord   vec2_t(s/t)
    };
    struct {
        vec_t x, y, z;
        vec_t s, t;
    };
    vec_t arr[5];
} vec5_t;
#endif
STATIC_ASSERT_SIZE(vec5_t, 5 * sizeof(vec_t));
typedef vec5_t* vec5_p;

#ifdef __cplusplus
extern "C" {
#endif

    void    VectorCopy(vec3_t const in, vec3_p out); // src/engine/Client_side/video/soft/render3D/r_alias.c:383
    vec3_t  Scalar2Vector(vec_t scale);
    vec3_t  VectorAdd(vec3_t const veca, vec3_t const vecb /*, vec3_p out */ );         // va + vb
    vec3_t  VectorSubtract(vec3_t const veca, vec3_t const vecb /*, vec3_p out */ );    // va - vb
    vec3_t  VectorScale(vec3_t const in, vec_t const scale /*, vec3_p out */ );         // va + s
    vec3_t  VectorMA(vec3_t veca, float scale, vec3_t vecb /*, vec3_p vecc */ );        // va + (vb * s)

    void    VectorInverse(vec3_p v);                            // va = -va
    bool    VectorCompare(vec3_t const v1, vec3_t const v2);    // va == vb (all dimension)
    vec_t   Length(vec3_t const v);         // returns vector length
    float   VectorNormalize(vec3_p v);

    vec_t   DotProduct(vec3_t const v1, vec3_t const v2);
    vec3_t  CrossProduct(vec3_t const v1, vec3_t const v2 /*, vec3_p cross */ );

    vec3_t  PerpendicularVector(/* vec3_p dst, */ const vec3_t src);
    vec3_t  ProjectPointOnPlane(/* vec3_p dst, */ const vec3_t p, const vec3_t normal);

    vec3_t  TransformVector(vec3_t in /*, vec3_p out */ );  // TODO: move from  src/engine/Client_side/video/soft/render3D/r_misc.c
    vec3_t  VectorAddVal(vec3_t v, vec_t val);

#ifdef __cplusplus
}
#endif