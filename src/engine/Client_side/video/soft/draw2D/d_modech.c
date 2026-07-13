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
// d_modech.c: called when mode has just changed

#include "d_local.h"
#include "render.h"

int d_vrectx;
int d_vrecty;
int d_vrectright_particle;
int d_vrectbottom_particle;
int d_y_aspect_shift;
int d_pix_min;
int d_pix_max;
int d_pix_shift;
int d_scantable[MAXHEIGHT];
int16_p zspantable[MAXHEIGHT];


/*
================
D_Patch
================
*/
void D_Patch() {
#if id386

    static bool _protectSet8 = false;

    if (!_protectSet8) {
        Sys_MakeCodeWriteable(
            (int)D_PolysetAff8Start,
            (int)D_PolysetAff8End - (int)D_PolysetAff8Start
        );
        _protectSet8 = true;
    }

#endif // id386
}


/*
================
D_ViewChanged
================
*/
#include "vid.h" // vid.zBuff.width
void D_ViewChanged() {
    scale_for_mip = xscale;
    ClampLessThen(&scale_for_mip, yscale);

    vid.zBuff.width = vid.frameBuff.width;

    d_pix_min = r_refdef.vrect.width / 320;
    ClampLessThen(&d_pix_min, 1);

    d_pix_max = (int)((float)r_refdef.vrect.width / (320.0 / 4.0) + 0.5);
    ClampLessThen(&d_pix_max, 1);

    d_pix_shift = 8 - (int)((float)r_refdef.vrect.width / 320.0 + 0.5);

    d_y_aspect_shift = (pixelAspect > 1.4) ? 1 : 0;

    d_vrectx = r_refdef.vrect.x;
    d_vrecty = r_refdef.vrect.y;
    d_vrectright_particle = r_refdef.vrectright - d_pix_max;
    d_vrectbottom_particle = r_refdef.vrectbottom - (d_pix_max << d_y_aspect_shift);

    ptrdiff_t rowbytes = (r_dowarp) ? WARP_WIDTH : Scr.SR_rowBytes;
    for (int i = 0; i < vid.frameBuff.height; i++) {
        d_scantable[i] = (rowbytes * i);
        zspantable[i] = vid.zBuff.pZBuff + (vid.zBuff.width * i);
    }

    D_Patch();
}

