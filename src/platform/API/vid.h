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
// vid.h -- video driver defs
#include "keys.h"

#define VID_CBITS	6
#define VID_GRADES	(1 << VID_CBITS)


#if defined(_WIN32) && !defined(WINDED)
#if defined(_M_IX86)
#define __i386__	1
#endif
void	VID_LockBuffer();
void	VID_UnlockBuffer();
#else
#define	VID_LockBuffer()
#define	VID_UnlockBuffer()
#endif
// a pixel can be one, two, or four bytes
typedef uint8_t pixel_t;
typedef pixel_t* pixel_p;


struct vRect_s;
typedef struct vRect_s vRect_t;
typedef vRect_t* vRect_p;
struct vRect_s {
    int	    x, y, width, height;
    vRect_p pnext;
};

typedef struct {
    pixel_p     buffer;		// invisible buffer
    pixel_p     colormap;		// 256 * VID_GRADES size
    uint16_p   colormap16;	// 256 * VID_GRADES size
    int         fullbright;		// index of first fullbright color
    uint32_t    rowbytes;	// may be > width if displayed in a window
    uint32_t    width;
    uint32_t    height;
    float       aspect;		// width / height -- < 0 is taller than wide
    int         numpages;
    int         recalc_refdef;	// if true, recalc vid-based stuff
    pixel_p     conbuffer;
    int         conrowbytes;
    uint32_t    conwidth;
    uint32_t    conheight;
    int         maxwarpwidth;
    int         maxwarpheight;
    pixel_p     direct;		// direct drawing to framebuffer, if not
    //  NULL
} VidDef_t;
typedef VidDef_t* VidDef_p;

extern	VidDef_t    vid;				// global video state
extern	uint16_t    d_8to16table[256];
extern	uint32_t    d_8to24table[256];
extern void (*vid_menudrawfn)();
extern void (*vid_menukeyfn)(keycode_t key);

#ifdef __cplusplus
extern "C" {
#endif
    // called at startup and after any gamma correction
    void    VID_SetPalette(uint8_p palette);

    // called for bonus and pain flashes, and for underwater color changes
    void    VID_ShiftPalette(uint8_p palette);

    // Called at startup to set up translation tables, takes 256 8 bit RGB values
    // the palette data will go away after the call, so it must be copied off if
    // the video driver will need it again
    void    VID_Init(uint8_p palette);

    // Called at shutdown
    void    VID_Shutdown();

    // flushes the given rectangles from the view buffer to the screen
    void    VID_Update(vRect_p rects);

    // sets the mode; only used by the Quake engine for resetting to mode 0 (the
    // base mode) on memory allocation failures
    int     VID_SetMode(int modenum, uint8_p palette);

    // called only on Win32, when pause happens, so the mouse can be released
    void    VID_HandlePause(bool pause);

#ifdef __cplusplus
}
#endif