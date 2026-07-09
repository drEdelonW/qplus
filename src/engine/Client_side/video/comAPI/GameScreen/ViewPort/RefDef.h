#pragma once

#include "pose.h"
#include "vRect.h"

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
#if 1
    vRect_t vrect;                  // subwindow in video for refresh
#else
    vRect_t viewport;               // 3D viewport rectangle on screen
#endif
    // FIXME: not need vrect next field here?
    vRect_t aliasvrect;             // scaled Alias version
    int     vrectright, vrectbottom;// right & bottom screen coords
    int     aliasvrectright, aliasvrectbottom; // scaled Alias versions
    float   vrectrightedge;         // rightmost right edge we care about, for use in edge list
    float   fvrectx, fvrecty;       // for floating-point compares
    float   fvrectx_adj, fvrecty_adj;// left and top edges, for clamping
    int     vrect_x_adj_shift20;    //(vrect.x + 0.5 - epsilon) << 20
    int     vrectright_adj_shift20; //(vrectright + 0.5 - epsilon) << 20
    float   fvrectright_adj, fvrectbottom_adj;  // right and bottom edges, for clamping

    float   fvrectright;            // rightmost edge, for Alias clamping
    float   fvrectbottom;           // bottommost edge, for Alias clamping
    float   horizontalFieldOfView;  // at Z = 1.0, this many X is visible 2.0 = 90 degrees
    float   xOrigin;                // should probably allways be 0.5
    float   yOrigin;                // between be around 0.3 to 0.5

#if 0
    vec3_t  viewOrg;
    ang3_t  viewAngles;
#else
    pose_t  view;
#endif

    float   fov_x, fov_y;
#ifndef GLQUAKE
    int     ambientLight;
#endif
} refdef_t;
extern bool r_cache_thrash; // set if thrashing the surface cache. OpenGL compatability;

//
// refresh
//
extern refdef_t r_refdef;
