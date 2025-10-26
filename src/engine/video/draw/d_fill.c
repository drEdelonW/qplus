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
// d_clear: clears a specified rectangle to the specified color

#include "vid.h"


/*
================
D_FillRect
================
*/
void D_FillRect(vRect_p rect, int color) {
	int rx = rect->x;
	int ry = rect->y;
	int rwidth = rect->width;
	int rheight = rect->height;

	if (rx < 0) {
		rwidth += rx;
		rx = 0;
	}
	if (ry < 0) {
		rheight += ry;
		ry = 0;
	}
	if (rx + rwidth > vid.width)
		rwidth = vid.width - rx;
	if (ry + rheight > vid.height)
		rheight = vid.height - rx;

	if ((rwidth < 1) ||
		(rheight < 1))
		return;

	uint8_p dest = ((uint8_p)vid.buffer + ry * vid.rowbytes + rx);

	if (((rwidth & 0x03) == 0) &&
		(((uintptr_t)dest & 0x03) == 0)
		) {
		// faster aligned dword clear
		uint32_p ldest = (uint32_p)dest;
		color += color << 16;

		rwidth >>= 2;
		color += color << 8;

		for (int ry = 0; ry < rheight; ry++) {
			for (int rx = 0; rx < rwidth; rx++)
				ldest[rx] = color;
			ldest = (uint32_p)((uint8_p)ldest + vid.rowbytes);
		}
	}
	else {
		// slower byte-by-byte clear for unaligned cases
		for (int ry = 0; ry < rheight; ry++) {
			for (int rx = 0; rx < rwidth; rx++)
				dest[rx] = color;
			dest += vid.rowbytes;
		}
	}
}

