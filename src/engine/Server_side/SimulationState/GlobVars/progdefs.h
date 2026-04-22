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
#include "types.h"
#include "vector.h"
#include "pr_qString.h"
#include "pr_Function.h"

// VM global offsets; vectors occupy 3 float slots
typedef enum {
    OFS_NULL      = 0u,
    OFS_RETURN    = 1u,     // +3
    OFS_PARM0     = 4u,     // parm0..parm7: +3 per parm
    OFS_PARM1     = 7u,
    OFS_PARM2     = 10u,
    OFS_PARM3     = 13u,
    OFS_PARM4     = 16u,
    OFS_PARM5     = 19u,
    OFS_PARM6     = 22u,
    OFS_PARM7     = 25u,
    RESERVED_OFS  = 28u
} PrOfs_e;



#ifndef QUAKE2
#   include "progdefs.q1"
#else
#   include "progdefs.q2"
#endif


// typedef entvars_t* entvars_p;  // not necessary yet