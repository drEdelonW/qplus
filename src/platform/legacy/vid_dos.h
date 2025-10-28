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
// vid_dos.h: header file for DOS-specific video stuff

typedef struct vmode_s {
    struct vmode_s* pnext;
    cString name;
    cString header;
    unsigned	width;
    unsigned	height;
    float		aspect;
    unsigned	rowbytes;
    int			planar;
    int			numpages;
    TypeLess_ptr pextradata;
    int			(*setmode)(VidDef_p vid, struct vmode_s* pcurrentmode);
    void		(*swapbuffers)(VidDef_p vid, struct vmode_s* pcurrentmode,
        vRect_p rects);
    void		(*setpalette)(VidDef_p vid, struct vmode_s* pcurrentmode,
        uint8_p palette);
    void		(*begindirectrect)(VidDef_p vid, struct vmode_s* pcurrentmode,
        int x, int y, uint8_p pbitmap, int width,
        int height);
    void		(*enddirectrect)(VidDef_p vid, struct vmode_s* pcurrentmode,
        int x, int y, int width, int height);
} vmode_t;
typrdef vmode_t* vmode_p;

// vid_wait settings
#define VID_WAIT_NONE			0
#define VID_WAIT_VSYNC			1
#define VID_WAIT_DISPLAY_ENABLE	2

extern int		numvidmodes;
extern vmode_p pvidmodes;

extern int		VGA_width, VGA_height, VGA_rowbytes, VGA_bufferrowbytes;
extern uint8_p VGA_pagebase;
extern vmode_p VGA_pcurmode;

CVAR_EXTERN(vid_wait);
CVAR_EXTERN(vid_nopageflip);
CVAR_EXTERN(_vid_wait_override);

extern uint8_t colormap256[32][256];

extern TypeLess_ptr vid_surfcache;
extern int	vid_surfcachesize;

void VGA_Init();
void VID_InitVESA();
void VID_InitExtra();
void VGA_WaitVsync();
void VGA_ClearVideoMem(int planar);
void VGA_SetPalette(VidDef_p vid, vmode_p pcurrentmode, uint8_p pal);
void VGA_SwapBuffersCopy(VidDef_p vid, vmode_p pcurrentmode, vRect_p rects);
bool VGA_FreeAndAllocVidbuffer(VidDef_p vid, int allocnewbuffer);
bool VGA_CheckAdequateMem(int width, int height, int rowbytes, int allocnewbuffer);
void VGA_BeginDirectRect(VidDef_p vid, struct vmode_s* pcurrentmode, int x, int y, uint8_p pbitmap, int width, int height);
void VGA_EndDirectRect(VidDef_p vid, struct vmode_s* pcurrentmode, int x, int y, int width, int height);
void VGA_UpdateLinearScreen(TypeLess_ptr srcptr, TypeLess_ptr destptr, int width, int height, int srcrowbytes, int destrowbytes);

