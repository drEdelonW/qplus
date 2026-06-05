#pragma once

#include "vector.h"
#include "types.h"

#include "Hull.h"

#include "platformdefs.h"
// #include "zone.h"

#include "SyncType.h"


#define ALIAS_VERSION   6

#define IDPOLYHEADER    (uint32_t)(('O' << 24) + ('P' << 16) + ('D' << 8) + 'I')
// little-endian "IDPO"




// must match definition in spritegn.h
typedef struct {
    int32_t     ident;
    int32_t     version;
    vec3_t      scale;
    vec3_t      scale_origin;
    float       boundingradius;
    vec3_t      eyeposition;
    int32_t     numskins;
    int32_t     skinwidth;
    int32_t     skinheight;
    int32_t     numverts;
    int32_t     numtris;
    int32_t     numframes;
    SyncType_t  synctype;
    int32_t     flags;
    float       size;
} Mdl_t;
typedef Mdl_t* Mdl_p;


