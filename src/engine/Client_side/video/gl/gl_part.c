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

#include "d_local.h"

#include "Particle.h"
#include "vid.h"

static vec3_t _up;
static vec3_t _right;
/*
==============
D_StartParticles
==============
*/
void D_StartParticles() {
    GL_Bind(particletexture);
    glEnable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBegin(GL_TRIANGLES);

    _up = VectorScale(BS.up, 1.5f);
    _right = VectorScale(BS.right, 1.5f);
}


/*
==============
D_EndParticles
==============
*/
void D_EndParticles() {
    glEnd();
    glDisable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}


/*
==============
D_DrawParticle
==============
*/
void D_DrawParticle(Particle_p pparticle) {

    // hack a scale up to keep particles from disapearing
    float scale = DotProduct(VectorSubtract(pparticle->org, r_origin), BS.forward);
    scale = (scale < 20.0f) ?
        1.0f : (1.0f + scale * 0.004f);

    glColor3ubv((uint8_p)&d_8to24table[(int)pparticle->color]);

    glTexCoord2f(0.0f, 0.0f);       glVertex3fv(pparticle->org.v);
    glTexCoord2f(1.0f, 0.0f);       glVertex3fv(VectorMA(pparticle->org, scale, _up).v);
    glTexCoord2f(0.0f, 1.0f);       glVertex3fv(VectorMA(pparticle->org, scale, _right).v);
}
