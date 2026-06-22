/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// chase.c -- chase camera code

#include "chase.h"
#include <string.h>
#include <math.h>
#include "angle.h"
#include "cvar_q1.h"
#include "client.h"
#include "q_tools.h"
#include "world.h"


static vec3_t _chaseDest;


void Chase_Init() {
    Cvar_RegisterVariable(&chase_back);
    Cvar_RegisterVariable(&chase_up);
    Cvar_RegisterVariable(&chase_right);
    Cvar_RegisterVariable(&chase_active);
}

void Chase_Reset() {} // for respawning and teleporting start position 12 units behind head

void TraceLine(vec3_t start, vec3_t end, vec3_p impact) {
    trace_t  trace; memset(&trace, 0, sizeof(trace));
    SV_RecursiveHullCheck(cl.worldmodel->hulls, 0, 0, 1, start, end, &trace);

    *impact = trace.endpos;
}

void Chase_Update() {
    // if can't see player, reset
    Basis_t bs = GetBasis(cl.viewangles);

    _chaseDest = VectorMA(VectorMA(r_refdef.vieworg,
        -chase_back.value, bs.forward),
        -chase_right.value, bs.right
    );
    _chaseDest.z = r_refdef.vieworg.z + chase_up.value;

    // find the spot the player is looking at
    vec3_t stop;  TraceLine(    // TODO: remake to stack vector return
        r_refdef.vieworg,
        VectorMA(r_refdef.vieworg, 4096, bs.forward),
        &stop
    );

    // calculate pitch to look at the same spot from camera
    stop = VectorSubtract(stop, r_refdef.vieworg);  // stop -= r_refdef.vieworg;
    float dist = DotProduct(stop, bs.forward);
    CLAMP_LESS(dist, 1);

    r_refdef.viewangles.pitch =
        -atan(stop.z / dist) /
        M_PI * 180;

    // move towards destination
    r_refdef.vieworg = _chaseDest;
}

