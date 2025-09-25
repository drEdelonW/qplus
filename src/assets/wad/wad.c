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
// wad.c

#include "quakedef.h"
#include "wad.h"
#include <string.h>

int32_t			wad_numlumps;
lumpinfo_p  wad_lumps;
uint8_p wad_base;

// void SwapPic(qpic_p pic);

/*
	==================
	W_CleanupName

	Lowercases name and pads with spaces and a terminating 0 to the length of
	lumpinfo_t->name.
	Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
	Space padding is so names can be printed nicely in tables.
	Can safely be performed in place.
	==================
*/
void W_CleanupName(cstring in, cstring out) {
	int		i;

	for (i = 0; i < 16; i++) {
		char c = in[i];
		if (!c)
			break;

		if ((c >= 'A') &&
			(c <= 'Z'))
			c += ('a' - 'A');
		out[i] = c;
	}

	for (; i < 16; i++)
		out[i] = 0;
}



/*
	====================
	W_LoadWadFile
	====================
*/
void W_LoadWadFile(cstring filename) {
	wad_base = COM_LoadHunkFile(filename);
	if (!wad_base)
		Sys_Error("W_LoadWadFile: couldn't load %s", filename);

	wadinfo_p header = (wadinfo_p)wad_base;

	if (
		(header->identification[0] != 'W') ||
		(header->identification[1] != 'A') ||
		(header->identification[2] != 'D') ||
		(header->identification[3] != '2')
		)
		Sys_Error("Wad file %s doesn't have WAD2 id\n", filename);

	wad_numlumps = LittleLong(header->numlumps);
	int infotableofs = LittleLong(header->infotableofs);
	wad_lumps = (lumpinfo_p)(wad_base + infotableofs);

	lumpinfo_p  lump_p;
	uint32_t	i;
	for (i = 0, lump_p = wad_lumps; i < wad_numlumps; i++, lump_p++) {
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
		W_CleanupName(lump_p->name, lump_p->name);
		if (lump_p->type == TYP_QPIC)
			SwapPic((qpic_p)(wad_base + lump_p->filepos));
	}
}


/*
	=============
	W_GetLumpinfo
	=============
*/
lumpinfo_p W_GetLumpinfo(cstring name) {
	int		i;
	lumpinfo_p lump_p;
	char	clean[16];

	W_CleanupName(name, clean);

	for (lump_p = wad_lumps, i = 0; i < wad_numlumps; i++, lump_p++) {
		if (!strcmp(clean, lump_p->name))
			return lump_p;
	}

	Sys_Error("W_GetLumpinfo: %s not found", name);
	return NULL;
}

typeless_ptr W_GetLumpName(cstring name) {
	lumpinfo_p lump = W_GetLumpinfo(name);

	return (typeless_ptr)(wad_base + lump->filepos);
}

typeless_ptr W_GetLumpNum(int32_t num) {
	if ((num < 0) ||
		(num > wad_numlumps)
		)
		Sys_Error("W_GetLumpNum: bad number: %i", num);

	lumpinfo_p lump = wad_lumps + num;

	return (typeless_ptr)(wad_base + lump->filepos);
}

/*
	=============================================================================

	automatic byte swapping

	=============================================================================
*/

void SwapPic(qpic_p pic) {
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);
}
