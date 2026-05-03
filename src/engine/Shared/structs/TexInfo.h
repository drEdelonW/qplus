#pragma once

#include "types.h"
#include "Texture.h"

struct Texture_s;
typedef struct Texture_s Texture_t;
typedef Texture_t* Texture_p;

typedef struct {
    float   vecs[2][4];
    float  mipadjust;
    Texture_p texture;
    int32_t   flags;
} mTexInfo_t;
typedef mTexInfo_t* mTexInfo_p;

typedef struct TexInfo_s {
    float   vecs[2][4];  // [s/t][xyz offset]
    int32_t miptex;
    int32_t flags;
} TexInfo_t;
typedef TexInfo_t* TexInfo_p;