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
// r_vars.c: global refresh variables

#if !id386

#include "types.h"
#include "fixed.h"  // fixed16_t
#include "qColor.h" // qColor8_p


// all global and static refresh variables are collected in a contiguous block
// to avoid cache conflicts.

//-------------------------------------------------------
// global refresh variables
//-------------------------------------------------------

#if 1
// FIXME: make into one big structure, like cl or sv
// FIXME: do separately for refresh engine and driver

float d_sdivzstepu, d_tdivzstepu, d_zistepu;
float d_sdivzstepv, d_tdivzstepv, d_zistepv;
float d_sdivzorigin, d_tdivzorigin, d_ziorigin;

fixed16_t sadjust;
fixed16_t tadjust;
fixed16_t bbextents;
fixed16_t bbextentt;
#else
// TODO: Rework gradient globals to vector like operations
#endif

qColor8_p cacheblock;
int     cachewidth;

qColor8_p d_viewbuffer;

int16_p d_pzbuffer; // TODO: is this Z buffer???
uint32_t d_zwidth;

#endif // !id386

