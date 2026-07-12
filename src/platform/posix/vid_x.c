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
// vid_x.c -- general x video driver

#define _BSD

#include "vid.h"

#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>

#include "x_prv.h"
#include "sys.h"
#include "z_hunk.h"
#include "console.h"
#include "common.h"
#include "endian_tools.h"
#include "d_local.h"
#include "host.h"
#include "q_tools.h"
#include "screen.h"
#include "view.h"
#include "render.h"


VidDef_t vid; // global video state

Rgb16_t d_8to16table[InksNum]; // extern
static PIXEL16 st2d_8to16table[InksNum];
static PIXEL24 st2d_8to24table[InksNum];
static qPal_t current_palette;   // 768 byte

bool    doShm = false;  // extern
Display* x_disp;    // shared with in_x
static Colormap x_cmap;
Window          x_win;
static GC       x_gc;
static Visual* x_vis;
static XVisualInfo* x_visinfo;
int          x_shmeventtype;

bool   oktodraw = false;

static bool current_framebuffer;
static XImage* x_framebuffer[2] = { 0, 0 };
static XShmSegmentInfo x_shminfo[2];

static bool verbose = false;

static int32_t X11_highhunkmark;
static size_t X11_buffersize;

static size_t vid_surfcachesize;
static TypeLess_ptr vid_surfcache;

#if 1 /* =================================[ begin palette tools ]================================= */
static uint32_t r_mask, g_mask, b_mask;
static int8_t r_shift = -8;
static int8_t g_shift = -8;
static int8_t b_shift = -8;   // should be signed because (-8)
static bool shiftmask_fl = false;

void shiftmask_init() {
    r_mask = x_vis->red_mask;
    g_mask = x_vis->green_mask;
    b_mask = x_vis->blue_mask;

    for (uint32_t x = 1; x < r_mask; x = TWICE(x)) r_shift++;
    for (uint32_t x = 1; x < g_mask; x = TWICE(x)) g_shift++;
    for (uint32_t x = 1; x < b_mask; x = TWICE(x)) b_shift++;

    shiftmask_fl = true;
}

static inline uint32_t ChanShiftMask(int value, int shift, uint32_t mask) {
    if (shift > 0)  return ((uint32_t)value << shift) & mask;
    if (shift < 0)  return ((uint32_t)value >> (-shift)) & mask;
    /*           */ return (uint32_t)value & mask;
}
static inline Rgb32_t PackRgb(int r, int g, int b) {   // TODO: rework for [qRgb24] color geting
    return ChanShiftMask(r, r_shift, r_mask) |
        ChanShiftMask(g, g_shift, g_mask) |
        ChanShiftMask(b, b_shift, b_mask);
}

PIXEL16 xlib_rgb16(int r, int g, int b) {
    if (!shiftmask_fl) shiftmask_init();
    return (PIXEL16)PackRgb(r, g, b);
}

PIXEL24 xlib_rgb24(int r, int g, int b) {
    if (!shiftmask_fl) shiftmask_init();
    return (PIXEL24)PackRgb(r, g, b);
}
#endif /* =================================[ palette tools end ]================================= */


void st2_fixup(XImage* framebuf, int x, int y, int width, int height) {
    if ((x < 0) || (y < 0))     return;

    for (int yi = y; yi < (y + height); yi++) {
        uint8_p src = (uint8_p)&framebuf->data[yi * framebuf->bytes_per_line];

        // Duff's Device
        register int count = width;
        register int n = (count + 7) / 8;
        PIXEL16* dest = ((PIXEL16*)src) + x + width - 1;
        src += x + width - 1;

        switch (count % 8) {
        case 0: do {
            /*     */ *dest-- = st2d_8to16table[*src--];
        case 7:   *dest-- = st2d_8to16table[*src--];
        case 6:   *dest-- = st2d_8to16table[*src--];
        case 5:   *dest-- = st2d_8to16table[*src--];
        case 4:   *dest-- = st2d_8to16table[*src--];
        case 3:   *dest-- = st2d_8to16table[*src--];
        case 2:   *dest-- = st2d_8to16table[*src--];
        case 1:   *dest-- = st2d_8to16table[*src--];
        } while (--n > 0);
        }

        //  for(int xi = (x+width-1); xi >= x; xi--) {
        //   dest[xi] = st2d_8to16table[src[xi]];
        //  }
    }
}

void st3_fixup(XImage* framebuf, int x, int y, int width, int height) {
    if ((x < 0) || (y < 0))return;

    for (int yi = y; yi < (y + height); yi++) {
        uint8_p src = (uint8_p)&framebuf->data[yi * framebuf->bytes_per_line];

        // Duff's Device
        register int count, n;
        count = width;
        n = (count + 7) / 8;
        PIXEL24* dest = ((PIXEL24*)src) + x + width - 1;
        src += x + width - 1;

        switch (count % 8) {
        case 0: do {
            /*     */ *dest-- = st2d_8to24table[*src--];
        case 7:   *dest-- = st2d_8to24table[*src--];
        case 6:   *dest-- = st2d_8to24table[*src--];
        case 5:   *dest-- = st2d_8to24table[*src--];
        case 4:   *dest-- = st2d_8to24table[*src--];
        case 3:   *dest-- = st2d_8to24table[*src--];
        case 2:   *dest-- = st2d_8to24table[*src--];
        case 1:   *dest-- = st2d_8to24table[*src--];
        } while (--n > 0);
        }

        //  for(int xi = (x+width-1); xi >= x; xi--) {
        //   dest[xi] = st2d_8to16table[src[xi]];
        //  }
    }
}


// ========================================================================
// Tragic death handler
// ========================================================================

void TragicDeath(int signal_num) {
    XAutoRepeatOn(x_disp);
    XCloseDisplay(x_disp);
    Sys_Error("This death brought to you by the number %d\n", signal_num);
}

// ========================================================================
// makes a null cursor
// ========================================================================

static Cursor CreateNullCursor(Display* display, Window root) {
    Pixmap cursormask = XCreatePixmap(
        display,
        root,
        1, 1,
        1/*depth*/
    );
    XGCValues xgc = {
         .function = GXclear
    };
    GC gc = XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display,
        cursormask, gc,
        0, 0,
        1, 1
    );
    XColor dummycolour = {
        .pixel = 0,
        .red = 0,
        .flags = 04
    };
    Cursor cursor = XCreatePixmapCursor(
        display,
        cursormask, cursormask,
        &dummycolour, &dummycolour,
        0, 0
    );
    XFreePixmap(display, cursormask);
    XFreeGC(display, gc);
    return cursor;
}

void ResetFrameBuffer() {
    if (x_framebuffer[0]) {
        free(x_framebuffer[0]->data);
        free(x_framebuffer[0]);
    }

    if (vid.zBuff.pZBuff) {
        D_FlushCaches();
        Hunk_FreeToHighMark(X11_highhunkmark);
        vid.zBuff.pZBuff = NULL;
    }
    X11_highhunkmark = Hunk_HighMark();

    // alloc an extra line in case we want to wrap, and allocate the z-buffer
    X11_buffersize = vid.frameBuff.width * vid.frameBuff.height * sizeof(*vid.zBuff.pZBuff);

    vid_surfcachesize = D_SurfaceCacheForRes(vid.frameBuff.width, vid.frameBuff.height);

    X11_buffersize += vid_surfcachesize;

    vid.zBuff.pZBuff = Hunk_HighAllocName(X11_buffersize, "video");
    if (vid.zBuff.pZBuff == NULL)
        Sys_Error("Not enough memory for video mode\n");

    vid_surfcache = (uint8_p)vid.zBuff.pZBuff
        + vid.frameBuff.width * vid.frameBuff.height * sizeof(*vid.zBuff.pZBuff);

    D_InitCaches(vid_surfcache, vid_surfcachesize);

    int pwidth = x_visinfo->depth / 8;
    if (pwidth == 3)    pwidth = 4;
    int mem = ((vid.frameBuff.width * pwidth + 7) & ~7) * vid.frameBuff.height;

    x_framebuffer[0] = XCreateImage(
        x_disp, x_vis,
        x_visinfo->depth, ZPixmap,
        0, malloc(mem),
        vid.frameBuff.width, vid.frameBuff.height,
        32, 0
    );

    if (!x_framebuffer[0])
        Sys_Error("VID: XCreateImage failed\n");

    vid.frameBuff.pBuff = (uint8_p)(x_framebuffer[0]);
    Scr.con.pBuff = vid.frameBuff.pBuff;

}

void ResetSharedFrameBuffers() {
    int minsize = getpagesize();

    if (vid.zBuff.pZBuff) {
        D_FlushCaches();
        Hunk_FreeToHighMark(X11_highhunkmark);
        vid.zBuff.pZBuff = NULL;
    }

    X11_highhunkmark = Hunk_HighMark();

    // alloc an extra line in case we want to wrap, and allocate the z-buffer
    X11_buffersize = vid.frameBuff.width * vid.frameBuff.height * sizeof(*vid.zBuff.pZBuff);

    vid_surfcachesize = D_SurfaceCacheForRes(vid.frameBuff.width, vid.frameBuff.height);

    X11_buffersize += vid_surfcachesize;

    vid.zBuff.pZBuff = Hunk_HighAllocName(X11_buffersize, "video");
    if (vid.zBuff.pZBuff == NULL)
        Sys_Error("Not enough memory for video mode\n");

    vid_surfcache = (uint8_p)vid.zBuff.pZBuff + (vid.frameBuff.width * vid.frameBuff.height * sizeof(*vid.zBuff.pZBuff));

    D_InitCaches(vid_surfcache, vid_surfcachesize);

    for (int frm = 0; frm < 2; frm++) {

        // free up old frame buffer memory

        if (x_framebuffer[frm]) {
            XShmDetach(x_disp, &x_shminfo[frm]);
            free(x_framebuffer[frm]);
            shmdt(x_shminfo[frm].shmaddr);
        }

        // create the image

        x_framebuffer[frm] = XShmCreateImage(
            x_disp,
            x_vis, x_visinfo->depth,
            ZPixmap, 0,
            &x_shminfo[frm],
            vid.frameBuff.width, vid.frameBuff.height
        );

        // grab shared memory

        int size =
            x_framebuffer[frm]->bytes_per_line *
            x_framebuffer[frm]->height;
        if (size < minsize)
            Sys_Error("VID: Window must use at least %d bytes\n", minsize);

        int key = random();
        x_shminfo[frm].shmid = shmget((key_t)key, size, IPC_CREAT | 0777);
        if (x_shminfo[frm].shmid == -1)
            Sys_Error("VID: Could not get any shared memory\n");

        // attach to the shared memory segment
        x_shminfo[frm].shmaddr =
            (TypeLess_ptr)shmat(x_shminfo[frm].shmid, 0, 0);

        printf("VID: shared memory id=%d, addr=0x%p\n",
            x_shminfo[frm].shmid,
            (TypeLess_ptr)x_shminfo[frm].shmaddr);

        x_framebuffer[frm]->data = x_shminfo[frm].shmaddr;

        // get the X server to attach to it

        if (!XShmAttach(x_disp, &x_shminfo[frm]))
            Sys_Error("VID: XShmAttach() failed\n");

        XSync(x_disp, 0);
        shmctl(x_shminfo[frm].shmid, IPC_RMID, 0);

    }

}

// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void VID_Init(qPal_p palette) {
    vid.frameBuff.width = BASEWIDTH;
    vid.frameBuff.height = BASEHEIGHT;
    Scr.pColorMapPal = host_colormap;
    Scr.numpages = 2;

    srandom(getpid());

    verbose = COM_CheckParm("-verbose");

    // open the display
    x_disp = XOpenDisplay(0);
    if (!x_disp) {
        if (getenv("DISPLAY"))  Sys_Error("VID: Could not open display [%s]\n", getenv("DISPLAY"));
        else                    Sys_Error("VID: Could not open local display\n");
    }

    // catch signals so i can turn on auto-repeat

    {
        struct sigaction sa;
        sigaction(SIGINT, 0, &sa);
        sa.sa_handler = TragicDeath;
        sigaction(SIGINT, &sa, 0);
        sigaction(SIGTERM, &sa, 0);
    }

    XAutoRepeatOff(x_disp);

    // for debugging only
    XSynchronize(x_disp, True);

    int pnum;
    // check for command-line window size
    if ((pnum = COM_CheckParm("-winsize"))) {
        if (pnum >= com.argc - 2)           Sys_Error("VID: -winsize <width> <height>\n");

        vid.frameBuff.width = Q_atoi(com.argv[pnum + 1]);
        vid.frameBuff.height = Q_atoi(com.argv[pnum + 2]);
        if (!vid.frameBuff.width || !vid.frameBuff.height)      Sys_Error("VID: Bad window width/height\n");

    }
    if ((pnum = COM_CheckParm("-width"))) {
        if (pnum >= com.argc - 1)       Sys_Error("VID: -width <width>\n");

        vid.frameBuff.width = Q_atoi(com.argv[pnum + 1]);
        if (!vid.frameBuff.width)                 Sys_Error("VID: Bad window width\n");

    }
    if ((pnum = COM_CheckParm("-height"))) {
        if (pnum >= com.argc - 1)       Sys_Error("VID: -height <height>\n");

        vid.frameBuff.height = Q_atoi(com.argv[pnum + 1]);
        if (!vid.frameBuff.height)                Sys_Error("VID: Bad window height\n");
    }

    int template_mask = 0;

    {// specify a visual id
        XVisualInfo template;
        if ((pnum = COM_CheckParm("-visualid"))) {
            if (pnum >= com.argc - 1)       Sys_Error("VID: -visualid <id#>\n");

            template.visualid = Q_atoi(com.argv[pnum + 1]);
            template_mask = VisualIDMask;
        }

        // If not specified, use default visual
        else {
            int screen = XDefaultScreen(x_disp);
            template.visualid = XVisualIDFromVisual(XDefaultVisual(x_disp, screen));
            template_mask = VisualIDMask;
        }

        {// pick a visual- warn if more than one was available
            int num_visuals;
            x_visinfo = XGetVisualInfo(x_disp, template_mask, &template, &num_visuals);
            if (num_visuals > 1) {
                printf("Found more than one visual id at depth %d:\n", template.depth);
                for (int i = 0; i < num_visuals; i++)
                    printf(" -visualid %d\n", (int)(x_visinfo[i].visualid));
            }
            else if (num_visuals == 0) {
                if (template_mask == VisualIDMask)      Sys_Error("VID: Bad visual id %d\n", template.visualid);
                else                                    Sys_Error("VID: No visuals at depth %d\n", template.depth);
            }
        }
    }

    if (verbose) {
        printf("Using visualid %d:\n", (int)(x_visinfo->visualid));
        printf(" screen %d\n", x_visinfo->screen);
        printf(" red_mask 0x%x\n", (int)(x_visinfo->red_mask));
        printf(" green_mask 0x%x\n", (int)(x_visinfo->green_mask));
        printf(" blue_mask 0x%x\n", (int)(x_visinfo->blue_mask));
        printf(" colormap_size %d\n", x_visinfo->colormap_size);
        printf(" bits_per_rgb %d\n", x_visinfo->bits_per_rgb);
    }

    x_vis = x_visinfo->visual;

    // setup attributes for main window
    {
        int attribmask = CWEventMask | CWColormap | CWBorderPixel;
        XSetWindowAttributes attribs;
        Colormap tmpcmap = XCreateColormap(
            x_disp, XRootWindow(x_disp, x_visinfo->screen),
            x_vis, AllocNone
        );

        attribs.event_mask =
            StructureNotifyMask | KeyPressMask |
            KeyReleaseMask | ExposureMask |
            PointerMotionMask | ButtonPressMask |
            ButtonReleaseMask;
        attribs.border_pixel = 0;
        attribs.colormap = tmpcmap;

        // create the main window
        x_win = XCreateWindow(
            x_disp,
            XRootWindow(x_disp, x_visinfo->screen),
            0, 0, // x, y
            vid.frameBuff.width, vid.frameBuff.height,
            0, // borderwidth
            x_visinfo->depth,
            InputOutput,
            x_vis,
            attribmask, &attribs
        );
        XStoreName(x_disp, x_win, "xquake");


        if (x_visinfo->class != TrueColor)
            XFreeColormap(x_disp, tmpcmap);
    }

    if (x_visinfo->depth == 8) {
        // create and upload the palette
        if (x_visinfo->class == PseudoColor) {
            x_cmap = XCreateColormap(x_disp, x_win, x_vis, AllocAll);
            VID_SetPalette(palette);
            XSetWindowColormap(x_disp, x_win, x_cmap);
        }
    }

    // inviso cursor
    XDefineCursor(x_disp, x_win, CreateNullCursor(x_disp, x_win));

    // create the GC
    {
        int valuemask = GCGraphicsExposures;
        XGCValues xgcvalues = { .graphics_exposures = False };
        x_gc = XCreateGC(x_disp, x_win, valuemask, &xgcvalues);
    }

    // map the window
    XMapWindow(x_disp, x_win);

    // wait for first exposure event
    {
        do {
            XEvent event;
            XNextEvent(x_disp, &event);
            if ((event.type == Expose) &&
                !(event.xexpose.count)
                )   oktodraw = true;
        } while (!oktodraw);
    }
    // now safe to draw

    // even if MITSHM is available, make sure it's a local connection
    if (XShmQueryExtension(x_disp)) {
        doShm = true;
        cString displayname = (cString)getenv("DISPLAY");
        if (displayname) {
            cString dName = displayname;
            while (*dName && (*dName != ':'))   dName++;
            if (*dName) *dName = 0x00;
            if (!(!strcasecmp(displayname, "unix") || !*displayname))
                doShm = false;
        }
    }

    if (doShm) {
        x_shmeventtype = XShmGetEventBase(x_disp) + ShmCompletion;
        ResetSharedFrameBuffers();
    }
    else
        ResetFrameBuffer();

    current_framebuffer = false;
    vid.frameBuff.rowBytes = x_framebuffer[current_framebuffer]->bytes_per_line;
    vid.frameBuff.pClr = (qColor8_p)x_framebuffer[current_framebuffer]->data;
    Scr.con.pClr = (qColor8_p)x_framebuffer[current_framebuffer]->data;
    Scr.con.rowBytes = vid.frameBuff.rowBytes;
    Scr.con.width = vid.frameBuff.width;
    Scr.con.height = vid.frameBuff.height;

    // XSynchronize(x_disp, False);

}

void VID_ShiftPalette(qPal_p p) {
    VID_SetPalette(p);
}



void VID_SetPalette(qPal_p palette) {
    for (int i = 0; i < InksNum; i++) {
        st2d_8to16table[i] = xlib_rgb16(
            palette->ink[i].r,
            palette->ink[i].g,
            palette->ink[i].b
        );
        st2d_8to24table[i] = xlib_rgb24(
            palette->ink[i].r,
            palette->ink[i].g,
            palette->ink[i].b
        );
    }

    if ((x_visinfo->class == PseudoColor) &&
        (x_visinfo->depth == 8)
        ) {
        if (palette != &current_palette)
#if 0
            memcpy(current_palette, palette, 768);
#else
            current_palette = *palette;
#endif
        XColor colors[InksNum];
        for (int i = 0; i < InksNum; i++) {
            colors[i] = (XColor){
                .pixel = i,
                .flags = DoRed | DoGreen | DoBlue,
                .red = palette->ink[i].r * 257,
                .green = palette->ink[i].g * 257,
                .blue = palette->ink[i].b * 257
            };
        }
        XStoreColors(x_disp, x_cmap, colors, InksNum);
    }

}

// Called at shutdown

void VID_Shutdown() {
    Con_Printf("VID_Shutdown\n");
    XAutoRepeatOn(x_disp);
    XCloseDisplay(x_disp);
}



#if 0
bool config_notify = false;
int config_notify_width;
int config_notify_height;
#else
CfgNotify_t xCfg = {
    .notify = false,
    .notify_width = 0,
    .notify_height = 0
};
#endif
// flushes the given rectangles from the view buffer to the screen

void VID_Update(vRect_p p_rects) {
    // vRect_t full;

// if the window changes dimension, skip this frame

    if (xCfg.notify) {
        fprintf(stderr, "config notify\n");
        xCfg.notify = false;

        vid.frameBuff.width = xCfg.notify_width & ~7;
        vid.frameBuff.height = xCfg.notify_height;

        if (doShm)      ResetSharedFrameBuffers();
        else            ResetFrameBuffer();

        Scr.canvas.rowBytes = x_framebuffer[0]->bytes_per_line;
        vid.frameBuff.pBuff = (uint8_p)x_framebuffer[current_framebuffer]->data;
        Scr.con.pBuff = vid.frameBuff.pBuff;
        Scr.con.width = vid.frameBuff.width;
        Scr.con.height = vid.frameBuff.height;
        Scr.con.rowBytes = Scr.canvas.rowBytes;

        SCR_RequestCalcRefdef();    // force a surface cache flush
        Con_CheckResize();
        Con_Clear_f();
        return;
    }

    // force full update if not 8bit
    if (x_visinfo->depth != 8) {
        SCR_RequestRedraw();
    }


    if (doShm) {
        while (p_rects) {
            /**/ if (x_visinfo->depth == 16)
                st2_fixup(x_framebuffer[current_framebuffer],
                    p_rects->x, p_rects->y,
                    p_rects->width, p_rects->height
                );
            else if (x_visinfo->depth == 24)
                st3_fixup(x_framebuffer[current_framebuffer],
                    p_rects->x, p_rects->y,
                    p_rects->width, p_rects->height
                );

            if (!XShmPutImage(
                x_disp, x_win,
                x_gc, x_framebuffer[current_framebuffer],
                p_rects->x, p_rects->y,
                p_rects->x, p_rects->y,
                p_rects->width, p_rects->height,
                True
            )
                )   Sys_Error("VID_Update: XShmPutImage failed\n");

            oktodraw = false;
            while (!oktodraw)
                GetEvent();

            p_rects = p_rects->pNext;   /* TODO: check is here something not NULL ? */
        }
        current_framebuffer = !current_framebuffer;
        vid.frameBuff.pBuff = (uint8_p)x_framebuffer[current_framebuffer]->data;
        XSync(x_disp, False);
    }
    else {
        while (p_rects) {
            /**/ if (x_visinfo->depth == 16)
                st2_fixup(x_framebuffer[current_framebuffer],
                    p_rects->x, p_rects->y,
                    p_rects->width, p_rects->height
                );
            else if (x_visinfo->depth == 24)
                st3_fixup(x_framebuffer[current_framebuffer],
                    p_rects->x, p_rects->y,
                    p_rects->width, p_rects->height
                );

            XPutImage(x_disp, x_win,
                x_gc, x_framebuffer[0],
                p_rects->x, p_rects->y,
                p_rects->x, p_rects->y,
                p_rects->width, p_rects->height
            );

#if 0   /* TODO: check is here something not NULL ? */
            p_rects = p_rects->pnext;
#else
            p_rects = p_rects->pNext;
#endif
        }
        XSync(x_disp, False);
    }

}
