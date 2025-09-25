#pragma once
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
// view.h
#include "cvar_q1.h"

extern	uint8_t		gammatable[256];	// palette is sent through this
extern	uint8_t		ramps[3][256];
extern float v_blend[4];

void V_Init();
void V_RenderView();
float V_CalcRoll(vec3_t angles, vec3_t velocity);
void V_UpdatePalette();

