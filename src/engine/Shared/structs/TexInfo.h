#pragma once

#include "types.h"
#include "vector.h"

#if 0
typedef float txVec_t[VECT_TX_DIM][4];   // [s/t][xyz offset]
#else
typedef union {
    struct {
        vec3_t  vx;
        vec_t   offs;
    };
    vec_t   V[4];
}txVec_t[VECT_TX_DIM];
#endif
STATIC_ASSERT_SIZE(txVec_t, 2*(3*4 + 4));

#include "Texture_pre.h"    // Texture_p
typedef struct {
    txVec_t     vecs;
    float       mipadjust;
    Texture_p   texture;
    int32_t     flags;
} mTexInfo_t;
typedef mTexInfo_t* mTexInfo_p;

typedef struct {
    txVec_t vecs;
    int32_t miptex;
    int32_t flags;
} TexInfo_t;
typedef TexInfo_t* TexInfo_p;