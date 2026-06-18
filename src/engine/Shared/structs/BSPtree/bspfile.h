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



typedef enum {
    CONTENTS_NODE         =  0,
  /* vvv[Leaf]vvv ^^^[Node]^^^ */
    CONTENTS_EMPTY        = -1,
    CONTENTS_SOLID        = -2,
    CONTENTS_WATER        = -3,
    CONTENTS_SLIME        = -4,
    CONTENTS_LAVA         = -5,
    CONTENTS_SKY          = -6,
    CONTENTS_ORIGIN       = -7,  // removed at CSG time
    CONTENTS_CLIP         = -8,  // changed to CONTENTS_SOLID

    CONTENTS_CURRENT_0    = -9,
    CONTENTS_CURRENT_90   = -10,
    CONTENTS_CURRENT_180  = -11,
    CONTENTS_CURRENT_270  = -12,
    CONTENTS_CURRENT_UP   = -13,
    CONTENTS_CURRENT_DOWN = -14
} contents_t;





