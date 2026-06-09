#pragma once

#include "types.h"
#include "vector.h"
#include "Triangle.h"
#include "SyncType.h"

#define ALIAS_VERSION   6

#define ALIAS_BASE_SIZE_RATIO   (1.0 / 11.0)
                    // normalizing factor so player model works out to about
                    //  1 pixel per triangle

#define IDPOLYHEADER    (uint32_t)(('O' << 24) + ('P' << 16) + ('D' << 8) + 'I')
// little-endian "IDPO"

/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/
typedef enum {
    ALIAS_SINGLE = 0u,
    ALIAS_GROUP,
#ifdef STM32
    ALIAS_FRAME_TYPE_FORCE_DWORD = 0x7FFFFFFF
#endif
} AliasFrameType_t;

typedef struct {
    AliasFrameType_t type;
} dAliasFrameType_t;
typedef dAliasFrameType_t* dAliasFrameType_p;
STATIC_ASSERT_SIZE(dAliasFrameType_t, 4); // 4

typedef struct {
#ifdef GLQUAKE
    int     firstpose;
    int     numposes;
    float   interval;
#else
    AliasFrameType_t    type;
#endif
    TriVertx_t  bboxmin;
    TriVertx_t  bboxmax;
    int32_t     frame;
    char        name[16];
} mAliasFrameDesc_t;
typedef mAliasFrameDesc_t* mAliasFrameDesc_p;


#define MAX_SKINS 32

typedef struct {
#ifdef GLQUAKE
    int     ident;
    int     version;
    vec3_t  scale;
    vec3_t  scale_origin;
    float   boundingradius;
    vec3_t  eyeposition;
    int     numskins;
    int     skinwidth;
    int     skinheight;
    int     numverts;
    int     numtris;
    int     numframes;
    SyncType_t synctype;
    int     flags;
    float   size;

    int     numposes;
    int     poseverts;
    int     posedata; // numposes*poseverts trivert_t
    int     commands; // gl command list with embedded s/t
    int     gl_texturenum[MAX_SKINS][4];
    int     texels[MAX_SKINS]; // only for player skins
#else
    int32_t model;
    int32_t stverts;
    int32_t skindesc;
    int32_t triangles;
#endif
    mAliasFrameDesc_t frames[1];
} AliasHdr_t;
typedef AliasHdr_t* AliasHdr_p;



typedef enum {
    ALIAS_SKIN_SINGLE = 0u,
    ALIAS_SKIN_GROUP,
#ifdef STM32
    ALIAS_SKIN_TYPE_FORCE_DWORD = 0x7FFFFFFF
#endif
} AliasSkinType_t;

typedef struct {
    AliasSkinType_t type;
} dAliasSkinType_t;
typedef dAliasSkinType_t* dAliasSkinType_p;
STATIC_ASSERT_SIZE(dAliasSkinType_t, 4); // 4

typedef struct {
    AliasSkinType_t type;
    TypeLess_ptr    pcachespot;
    int32_t         skin;
} mAliasSkinDesc_t;
typedef mAliasSkinDesc_t* mAliasSkinDesc_p;

typedef struct {
    TriVertx_t  bboxmin;
    TriVertx_t  bboxmax;
    int32_t     frame;
} mAliasGroupFrameDesc_t;

typedef struct {
    int32_t                 numframes;
    int32_t                 intervals;
    mAliasGroupFrameDesc_t  frames[1];
} mAliasGroup_t;
typedef mAliasGroup_t* mAliasGroup_p;

typedef struct {
    int32_t numskins;
    int32_t intervals;
    mAliasSkinDesc_t    skindescs[1];
} mAliasSkinGroup_t;
typedef mAliasSkinGroup_t* mAliasSkinGroup_p;



typedef struct {
    TriVertx_t  bboxmin;    // lightnormal isn't used
    TriVertx_t  bboxmax;    // lightnormal isn't used
    char        name[16];   // frame name from grabbing
} dAliasFrame_t;
typedef dAliasFrame_t* dAliasFrame_p;
STATIC_ASSERT_SIZE(dAliasFrame_t, 4*2 + 1*16); // 25


typedef struct {
    int32_t     numframes;
    TriVertx_t  bboxmin;    // lightnormal isn't used
    TriVertx_t  bboxmax;    // lightnormal isn't used
} dAliasGroup_t;
typedef dAliasGroup_t* dAliasGroup_p;
STATIC_ASSERT_SIZE(dAliasGroup_t, 4 + 4*2); // 12


typedef struct {
    int32_t numskins;
} dAliasSkinGroup_t;
typedef dAliasSkinGroup_t* dAliasSkinGroup_p;
STATIC_ASSERT_SIZE(dAliasSkinGroup_t, 4); // 4


typedef struct {
    float interval;
} dAliasInterval_t;
typedef dAliasInterval_t* dAliasInterval_p;
STATIC_ASSERT_SIZE(dAliasInterval_t, 4); // 4


typedef struct {
    float interval;
} dAliasSkinInterval_t;
typedef dAliasSkinInterval_t* dAliasSkinInterval_p;
STATIC_ASSERT_SIZE(dAliasSkinInterval_t, 4); // 4



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
STATIC_ASSERT_SIZE(Mdl_t, 9*4 + 2*4 + 3*12 + 4); // 84

extern AliasHdr_p pheader;
extern AliasHdr_p paliashdr;

