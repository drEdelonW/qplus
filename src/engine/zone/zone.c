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

#include "zone.h"
#include "zone_prv.h"

#include "sys.h"
#include "console.h"
#include "common.h"

#define	DYNAMIC_SIZE    (0xC000)    /* 48Kb */

/*
	========================
	Memory_Init
	========================
*/
void Memory_Init(typeless_ptr buf, size_t size) {
	int zonesize = DYNAMIC_SIZE;

	Hulk_Init(buf, size);

	Cache_Init();
	int p = COM_CheckParm("-zone");
	if (p) {
		if (p < (com_argc - 1))
			zonesize = Q_atoi(com_argv[p + 1]) * 1024;
		else
			Sys_Error("Memory_Init: you must specify a size in KB after -zone");
	}
	mainzone = Hunk_AllocName(zonesize, "zone");
	Z_ClearZone(mainzone, zonesize);
}

