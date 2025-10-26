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
// d_part.c: software driver module for drawing particles

#include "q_tools.h"
#include "d_local.h"


/*
==============
D_EndParticles
==============
*/
void D_EndParticles() {
    // not used by software driver
}


/*
==============
D_StartParticles
==============
*/
void D_StartParticles() {
    // not used by software driver
}


#if !id386

/*
==============
D_DrawParticle
==============
*/
void D_DrawParticle(Particle_p pparticle) {
    // transform point
    vec3_t local; VectorSubtract(pparticle->org, r_origin, local);

    vec3_t transformed = {
        DotProduct(local, r_pright),
        DotProduct(local, r_pup),
        DotProduct(local, r_ppn)
    };

    if (transformed[2] < PARTICLE_Z_CLIP)
        return;

    // project the point
    // FIXME: preadjust xcenter and ycenter
    float zi = 1.0 / transformed[2];
    int u = (int)(xcenter + zi * transformed[0] + 0.5);
    int v = (int)(ycenter - zi * transformed[1] + 0.5);

    if ((v > d_vrectbottom_particle) ||
        (u > d_vrectright_particle) ||
        (v < d_vrecty) ||
        (u < d_vrectx)) {
        return;
    }

    int16_p pz = d_pzbuffer + (d_zwidth * v) + u;
    uint8_p pdest = d_viewbuffer + d_scantable[v] + u;
    int izi = (int)(zi * 0x8000);

    int pix = izi >> d_pix_shift;

    CLAMP(d_pix_min, pix, d_pix_max);

    switch (pix) {
    case 1: {
        int count = 1 << d_y_aspect_shift;

        for (; count; count--, pz += d_zwidth, pdest += screenwidth) {
            if (pz[0] <= izi) {
                pz[0] = izi;
                pdest[0] = pparticle->color;
            }
        }
    }break;

    case 2: {
        int count = 2 << d_y_aspect_shift;

        for (; count; count--, pz += d_zwidth, pdest += screenwidth) {
            if (pz[0] <= izi) {
                pz[0] = izi;
                pdest[0] = pparticle->color;
            }

            if (pz[1] <= izi) {
                pz[1] = izi;
                pdest[1] = pparticle->color;
            }
        }
    }break;

    case 3: {
        int count = 3 << d_y_aspect_shift;

        for (; count; count--, pz += d_zwidth, pdest += screenwidth) {
            if (pz[0] <= izi) {
                pz[0] = izi;
                pdest[0] = pparticle->color;
            }

            if (pz[1] <= izi) {
                pz[1] = izi;
                pdest[1] = pparticle->color;
            }

            if (pz[2] <= izi) {
                pz[2] = izi;
                pdest[2] = pparticle->color;
            }
        }
    }break;

    case 4: {
        int count = 4 << d_y_aspect_shift;

        for (; count; count--, pz += d_zwidth, pdest += screenwidth) {
            if (pz[0] <= izi) {
                pz[0] = izi;
                pdest[0] = pparticle->color;
            }

            if (pz[1] <= izi) {
                pz[1] = izi;
                pdest[1] = pparticle->color;
            }

            if (pz[2] <= izi) {
                pz[2] = izi;
                pdest[2] = pparticle->color;
            }

            if (pz[3] <= izi) {
                pz[3] = izi;
                pdest[3] = pparticle->color;
            }
        }
    }break;

    default: {
        int count = pix << d_y_aspect_shift;

        for (; count; count--, pz += d_zwidth, pdest += screenwidth) {
            for (int i = 0; i < pix; i++) {
                if (pz[i] <= izi) {
                    pz[i] = izi;
                    pdest[i] = pparticle->color;
                }
            }
        }
    }   break;
    }
}

#endif // !id386

