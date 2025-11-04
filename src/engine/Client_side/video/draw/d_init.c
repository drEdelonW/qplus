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
// d_init.c: rasterization driver initialization

#include "q_tools.h"
#include "d_local.h"

#define NUM_MIPS	4

SurfCache_p d_initial_rover;
bool        d_roverwrapped;
int         d_minmip;
float       d_scalemip[NUM_MIPS - 1];

static float	_BaseMip[NUM_MIPS - 1] = { 1.0, 0.5 * 0.8, 0.25 * 0.8 };

void (*d_drawspans) (eSpan_p pspan);


/*
===============
D_Init
===============
*/
void D_Init() {
    r_skydirect = 1;

    Cvar_RegisterVariable(&d_subdiv16);
    Cvar_RegisterVariable(&d_mipcap);
    Cvar_RegisterVariable(&d_mipscale);

    r_drawpolys = false;
    r_worldpolysbacktofront = false;
    r_recursiveaffinetriangles = true;
    r_pixbytes = 1;
    r_aliasuvscale = 1.0;
}


/*
===============
D_CopyRects
===============
*/
void D_CopyRects(vRect_p prects, int transparent) {

    // this function is only required if the CPU doesn't have direct access to the
    // back buffer, and there's some driver interface function that the driver
    // doesn't support and requires Quake to do in software (such as drawing the
    // console); Quake will then draw into wherever the driver points vid.buffer
    // and will call this function before swapping buffers

    // UNUSED(prects);
    // UNUSED(transparent);
}


/*
===============
D_EnableBackBufferAccess
===============
*/
void D_EnableBackBufferAccess() { VID_LockBuffer(); }


/*
===============
D_TurnZOn
===============
*/
void D_TurnZOn() {
    // not needed for software version
}


/*
===============
D_DisableBackBufferAccess
===============
*/
void D_DisableBackBufferAccess() { VID_UnlockBuffer(); }


/*
===============
D_SetupFrame
===============
*/
void D_SetupFrame() {
    d_viewbuffer = (r_dowarp) ? r_warpbuffer : (TypeLess_ptr)(uint8_p)vid.buffer;

    screenwidth = (r_dowarp) ? WARP_WIDTH : vid.rowbytes;

    d_roverwrapped = false;
    d_initial_rover = sc_rover;

    d_minmip = d_mipcap.value;
    CLAMP(0, d_minmip, 3);

    for (int i = 0; i < (NUM_MIPS - 1); i++)
        d_scalemip[i] = _BaseMip[i] * d_mipscale.value;

    d_drawspans =
#if	id386
    (d_subdiv16.value) ? D_DrawSpans16 :
#endif
        D_DrawSpans8;

    d_aflatcolor = 0;
}


/*
===============
D_UpdateRects
===============
*/
void D_UpdateRects(vRect_p prect) {
    // the software driver draws these directly to the vid buffer
    // UNUSED(prect);
}

