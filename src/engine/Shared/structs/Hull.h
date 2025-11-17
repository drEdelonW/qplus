#pragma once
#include "vector.h"
#include "types.h"

#include "Plane.h"
#include "ClipNode.h"

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    dClipNode_p clipnodes;
    mPlane_p    planes;
    int32_t     firstclipnode;
    int32_t     lastclipnode;
    vec3_t      clip_mins;
    vec3_t      clip_maxs;
} Hull_t;
typedef Hull_t* Hull_p;
