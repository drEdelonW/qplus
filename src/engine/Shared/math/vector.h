#pragma once
#include <stdbool.h>
#include "assert.h"


typedef float vec_t;
STATIC_ASSERT_SIZE(vec_t, sizeof(float));

typedef enum {
    S_AX = 0u,
    T_AX = 1u,
    VECT_TX_DIM = 2u,
} axis_tx_e;
typedef union {
    struct { vec_t s, t; };
    vec_t v[VECT_TX_DIM];
} vec2_t;

typedef enum {
    X_AX = 0u,
    Y_AX = 1u,
    Z_AX = 2u,
    VECT_DIM = 3u,
} axis_e;
#if 1
    typedef vec_t vec3_t[VECT_DIM];
#else
    typedef union {
        struct { vec_t x, y, z; };
        vec_t v[VECT_DIM];
    } vec3_t;
#endif
STATIC_ASSERT_SIZE(vec3_t, 3 * sizeof(vec_t));
typedef vec3_t* vec3_p;

typedef vec_t vec5_t[5];    // vec3_t(x/y/z) + vec2(s/t)
// STATIC_ASSERT(sizeof(vec5_t) == 5 * sizeof(vec_t), "vec5_t must be 20");
STATIC_ASSERT_SIZE(vec5_t, 5 * sizeof(vec_t));
typedef vec5_t* vec5_p;

#ifdef __cplusplus
extern "C" {
#endif

    vec_t   DotProduct(vec3_t const v1, vec3_t const v2);
    void    VectorSubtract(vec3_t const veca, vec3_t const vecb, vec3_t out);
    void    VectorAdd(vec3_t const veca, vec3_t const vecb, vec3_t out);
    void    VectorCopy(vec3_t const in, vec3_t out);

    void    VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

    bool    VectorCompare(vec3_t const v1, vec3_t const v2);
    vec_t   Length(vec3_t const v);

    void    CrossProduct(vec3_t const v1, vec3_t const v2, vec3_t cross);
    void    PerpendicularVector(vec3_t dst, const vec3_t src);
    void    ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);

    float   VectorNormalize(vec3_t v);  // returns vector length
    void    VectorInverse(vec3_t v);
    void    VectorScale(vec3_t const in, vec_t const scale, vec3_t out);


#ifdef __cplusplus
}
#endif