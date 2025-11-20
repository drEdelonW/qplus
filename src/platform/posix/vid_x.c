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
#include "zone.h"
#include "console.h"
#include "common.h"
#include "endian_tools.h"
#include "d_local.h"
#include "host.h"
#include "q_tools.h"
#include "screen.h"
#include "render.h"


static int     ignorenext;
// static int     bits_per_pixel;

// typedef struct {
//     int input;
//     int output;
// } keymap_t;

VidDef_t vid; // global video state
uint16_t d_8to16table[256];

// static int num_shades = 32;
// int d_con_indirect = 0;
// int vid_buffersize;

bool    doShm;
Display* x_disp;
static Colormap x_cmap;
Window          x_win;
static GC       x_gc;
static Visual* x_vis;
static XVisualInfo* x_visinfo;
// static XImage*      x_image;
int          x_shmeventtype;
// static XShmSegmentInfo x_shminfo;

bool   oktodraw = false;

// int XShmQueryExtension(Display*);
// int XShmGetEventBase(Display*);

static int current_framebuffer;
static XImage* x_framebuffer[2] = { 0, 0 };
static XShmSegmentInfo x_shminfo[2];

static int verbose = 0;

static uint8_t current_palette[768];

static int32_t X11_highhunkmark;
static int32_t X11_buffersize;

int vid_surfcachesize;
static TypeLess_ptr vid_surfcache;

typedef uint16_t PIXEL16;
typedef uint32_t PIXEL24;
static PIXEL16 st2d_8to16table[256];
static PIXEL24 st2d_8to24table[256];
static int shiftmask_fl = 0;
static int32_t r_shift, g_shift, b_shift;
static uint32_t r_mask, g_mask, b_mask;

void shiftmask_init() {
    uint32_t x;
    r_mask = x_vis->red_mask;
    g_mask = x_vis->green_mask;
    b_mask = x_vis->blue_mask;
    for (r_shift = -8, x = 1; x < r_mask; x = x << 1)
        r_shift++;
    for (g_shift = -8, x = 1; x < g_mask; x = x << 1)
        g_shift++;
    for (b_shift = -8, x = 1; x < b_mask; x = x << 1)
        b_shift++;
    shiftmask_fl = 1;
}

PIXEL16 xlib_rgb16(int r, int g, int b) {
    if (shiftmask_fl == 0) shiftmask_init();
    PIXEL16 p = 0;

    if (r_shift > 0)        p = (r << (r_shift)) & r_mask;
    else if (r_shift < 0)   p = (r >> (-r_shift)) & r_mask;
    else                    p |= (r & r_mask);

    if (g_shift > 0)        p |= (g << (g_shift)) & g_mask;
    else if (g_shift < 0)   p |= (g >> (-g_shift)) & g_mask;
    else                    p |= (g & g_mask);

    if (b_shift > 0)        p |= (b << (b_shift)) & b_mask;
    else if (b_shift < 0)   p |= (b >> (-b_shift)) & b_mask;
    else                    p |= (b & b_mask);

    return p;
}

PIXEL24 xlib_rgb24(int r, int g, int b) {
    if (shiftmask_fl == 0) shiftmask_init();
    PIXEL24 p = 0;

    if (r_shift > 0)        p = (r << (r_shift)) & r_mask;
    else if (r_shift < 0)   p = (r >> (-r_shift)) & r_mask;
    else                    p |= (r & r_mask);

    if (g_shift > 0)        p |= (g << (g_shift)) & g_mask;
    else if (g_shift < 0)   p |= (g >> (-g_shift)) & g_mask;
    else                    p |= (g & g_mask);

    if (b_shift > 0)        p |= (b << (b_shift)) & b_mask;
    else if (b_shift < 0)   p |= (b >> (-b_shift)) & b_mask;
    else                    p |= (b & b_mask);

    return p;
}

void st2_fixup(XImage* framebuf, int x, int y, int width, int height) {
    register int count, n;

    if ((x < 0) || (y < 0))return;

    for (int yi = y; yi < (y + height); yi++) {
        uint8_p src = (uint8_p)&framebuf->data[yi * framebuf->bytes_per_line];

        // Duff's Device
        count = width;
        n = (count + 7) / 8;
        PIXEL16* dest = ((PIXEL16*)src) + x + width - 1;
        src += x + width - 1;

        switch (count % 8) {
        case 0: do {
            *dest-- = st2d_8to16table[*src--];
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
    register int count, n;

    if ((x < 0) || (y < 0))return;

    for (int yi = y; yi < (y + height); yi++) {
        uint8_p src = (uint8_p)&framebuf->data[yi * framebuf->bytes_per_line];

        // Duff's Device
        count = width;
        n = (count + 7) / 8;
        PIXEL24* dest = ((PIXEL24*)src) + x + width - 1;
        src += x + width - 1;

        switch (count % 8) {
        case 0: do {
            *dest-- = st2d_8to24table[*src--];
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
    Pixmap cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    XGCValues xgc = {
         .function = GXclear
    };
    GC gc = XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    XColor dummycolour = {
        .pixel = 0,
        .red = 0,
        .flags = 04
    };
    Cursor cursor = XCreatePixmapCursor(display, cursormask, cursormask,
        &dummycolour, &dummycolour, 0, 0);
    XFreePixmap(display, cursormask);
    XFreeGC(display, gc);
    return cursor;
}

void ResetFrameBuffer() {
    if (x_framebuffer[0]) {
        free(x_framebuffer[0]->data);
        free(x_framebuffer[0]);
    }

    if (d_pzbuffer) {
        D_FlushCaches();
        Hunk_FreeToHighMark(X11_highhunkmark);
        d_pzbuffer = NULL;
    }
    X11_highhunkmark = Hunk_HighMark();

    // alloc an extra line in case we want to wrap, and allocate the z-buffer
    X11_buffersize = vid.width * vid.height * sizeof(*d_pzbuffer);

    vid_surfcachesize = D_SurfaceCacheForRes(vid.width, vid.height);

    X11_buffersize += vid_surfcachesize;

    d_pzbuffer = Hunk_HighAllocName(X11_buffersize, "video");
    if (d_pzbuffer == NULL)
        Sys_Error("Not enough memory for video mode\n");

    vid_surfcache = (uint8_p)d_pzbuffer
        + vid.width * vid.height * sizeof(*d_pzbuffer);

    D_InitCaches(vid_surfcache, vid_surfcachesize);

    int pwidth = x_visinfo->depth / 8;
    if (pwidth == 3)    pwidth = 4;
    int mem = ((vid.width * pwidth + 7) & ~7) * vid.height;

    x_framebuffer[0] = XCreateImage(x_disp,
        x_vis,
        x_visinfo->depth,
        ZPixmap,
        0,
        malloc(mem),
        vid.width, vid.height,
        32,
        0
    );

    if (!x_framebuffer[0])
        Sys_Error("VID: XCreateImage failed\n");

    vid.buffer = (uint8_p)(x_framebuffer[0]);
    vid.conbuffer = vid.buffer;

}

void ResetSharedFrameBuffers() {
    int minsize = getpagesize();

    if (d_pzbuffer) {
        D_FlushCaches();
        Hunk_FreeToHighMark(X11_highhunkmark);
        d_pzbuffer = NULL;
    }

    X11_highhunkmark = Hunk_HighMark();

    // alloc an extra line in case we want to wrap, and allocate the z-buffer
    X11_buffersize = vid.width * vid.height * sizeof(*d_pzbuffer);

    vid_surfcachesize = D_SurfaceCacheForRes(vid.width, vid.height);

    X11_buffersize += vid_surfcachesize;

    d_pzbuffer = Hunk_HighAllocName(X11_buffersize, "video");
    if (d_pzbuffer == NULL)
        Sys_Error("Not enough memory for video mode\n");

    vid_surfcache = (uint8_p)d_pzbuffer
        + vid.width * vid.height * sizeof(*d_pzbuffer);

    D_InitCaches(vid_surfcache, vid_surfcachesize);

    for (int frm = 0; frm < 2; frm++) {

        // free up old frame buffer memory

        if (x_framebuffer[frm]) {
            XShmDetach(x_disp, &x_shminfo[frm]);
            free(x_framebuffer[frm]);
            shmdt(x_shminfo[frm].shmaddr);
        }

        // create the image

        x_framebuffer[frm] = XShmCreateImage(x_disp,
            x_vis,
            x_visinfo->depth,
            ZPixmap,
            0,
            &x_shminfo[frm],
            vid.width,
            vid.height);

        // grab shared memory

        int size = x_framebuffer[frm]->bytes_per_line *
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

void VID_Init(uint8_p palette) {
#
    int pnum;
    XVisualInfo template;
    int num_visuals;
    int template_mask;

    ignorenext = 0;
    vid.width = 320;
    vid.height = 200;
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    vid.numpages = 2;
    vid.colormap = host_colormap;
    // vid.cbits = VID_CBITS;
    // vid.grades = VID_GRADES;
    vid.fullbright = 256 - LittleLong(*((int*)vid.colormap + 2048));

    srandom(getpid());

    verbose = COM_CheckParm("-verbose");

    // open the display
    x_disp = XOpenDisplay(0);
    if (!x_disp) {
        if (getenv("DISPLAY"))
            Sys_Error("VID: Could not open display [%s]\n",
                getenv("DISPLAY"));
        else
            Sys_Error("VID: Could not open local display\n");
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

    // check for command-line window size
    if ((pnum = COM_CheckParm("-winsize"))) {
        if (pnum >= com.argc - 2)           Sys_Error("VID: -winsize <width> <height>\n");

        vid.width = Q_atoi(com.argv[pnum + 1]);
        vid.height = Q_atoi(com.argv[pnum + 2]);
        if (!vid.width || !vid.height)      Sys_Error("VID: Bad window width/height\n");

    }
    if ((pnum = COM_CheckParm("-width"))) {
        if (pnum >= com.argc - 1)       Sys_Error("VID: -width <width>\n");

        vid.width = Q_atoi(com.argv[pnum + 1]);
        if (!vid.width)                 Sys_Error("VID: Bad window width\n");

    }
    if ((pnum = COM_CheckParm("-height"))) {
        if (pnum >= com.argc - 1)       Sys_Error("VID: -height <height>\n");

        vid.height = Q_atoi(com.argv[pnum + 1]);
        if (!vid.height)                Sys_Error("VID: Bad window height\n");
    }

    template_mask = 0;

    // specify a visual id
    if ((pnum = COM_CheckParm("-visualid"))) {
        if (pnum >= com.argc - 1)       Sys_Error("VID: -visualid <id#>\n");

        template.visualid = Q_atoi(com.argv[pnum + 1]);
        template_mask = VisualIDMask;
    }

    // If not specified, use default visual
    else {
        int screen;
        screen = XDefaultScreen(x_disp);
        template.visualid =
            XVisualIDFromVisual(XDefaultVisual(x_disp, screen));
        template_mask = VisualIDMask;
    }

    // pick a visual- warn if more than one was available
    x_visinfo = XGetVisualInfo(x_disp, template_mask, &template, &num_visuals);
    if (num_visuals > 1) {
        printf("Found more than one visual id at depth %d:\n", template.depth);
        for (int i = 0; i < num_visuals; i++)
            printf(" -visualid %d\n", (int)(x_visinfo[i].visualid));
    }
    else if (num_visuals == 0) {
        if (template_mask == VisualIDMask)
            Sys_Error("VID: Bad visual id %d\n", template.visualid);
        else
            Sys_Error("VID: No visuals at depth %d\n", template.depth);
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
        Colormap tmpcmap = XCreateColormap(x_disp, XRootWindow(x_disp,
            x_visinfo->screen), x_vis, AllocNone);

        attribs.event_mask =
            StructureNotifyMask | KeyPressMask |
            KeyReleaseMask | ExposureMask |
            PointerMotionMask | ButtonPressMask |
            ButtonReleaseMask;
        attribs.border_pixel = 0;
        attribs.colormap = tmpcmap;

        // create the main window
        x_win = XCreateWindow(x_disp,
            XRootWindow(x_disp, x_visinfo->screen),
            0, 0, // x, y
            vid.width, vid.height,
            0, // borderwidth
            x_visinfo->depth,
            InputOutput,
            x_vis,
            attribmask,
            &attribs);
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
        XGCValues xgcvalues;
        int valuemask = GCGraphicsExposures;
        xgcvalues.graphics_exposures = False;
        x_gc = XCreateGC(x_disp, x_win, valuemask, &xgcvalues);
    }

    // map the window
    XMapWindow(x_disp, x_win);

    // wait for first exposure event
    {
        XEvent event;
        do {
            XNextEvent(x_disp, &event);
            if (event.type == Expose && !event.xexpose.count)
                oktodraw = true;
        } while (!oktodraw);
    }
    // now safe to draw

    // even if MITSHM is available, make sure it's a local connection
    if (XShmQueryExtension(x_disp)) {
        cString displayname;
        doShm = true;
        displayname = (cString)getenv("DISPLAY");
        if (displayname) {
            cString d = displayname;
            while (*d && (*d != ':')) d++;
            if (*d) *d = 0;
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

    current_framebuffer = 0;
    vid.rowbytes = x_framebuffer[0]->bytes_per_line;
    vid.buffer = (pixel_p)x_framebuffer[0]->data;
    vid.direct = 0;
    vid.conbuffer = (pixel_p)x_framebuffer[0]->data;
    vid.conrowbytes = vid.rowbytes;
    vid.conwidth = vid.width;
    vid.conheight = vid.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);

    // XSynchronize(x_disp, False);

}

void VID_ShiftPalette(uint8_p p) {
    VID_SetPalette(p);
}



void VID_SetPalette(uint8_p palette) {
    for (int i = 0; i < 256; i++) {
        st2d_8to16table[i] = xlib_rgb16(palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]);
        st2d_8to24table[i] = xlib_rgb24(palette[i * 3], palette[i * 3 + 1], palette[i * 3 + 2]);
    }

    if (x_visinfo->class == PseudoColor && x_visinfo->depth == 8) {
        if (palette != current_palette)
            memcpy(current_palette, palette, 768);
        XColor colors[256];
        for (int i = 0; i < 256; i++) {
            colors[i].pixel = i;
            colors[i].flags = DoRed | DoGreen | DoBlue;
            colors[i].red = palette[i * 3] * 257;
            colors[i].green = palette[i * 3 + 1] * 257;
            colors[i].blue = palette[i * 3 + 2] * 257;
        }
        XStoreColors(x_disp, x_cmap, colors, 256);
    }

}

// Called at shutdown

void VID_Shutdown() {
    Con_Printf("VID_Shutdown\n");
    XAutoRepeatOn(x_disp);
    XCloseDisplay(x_disp);
}



int config_notify = 0;
int config_notify_width;
int config_notify_height;

// flushes the given rectangles from the view buffer to the screen

void VID_Update(vRect_p rects) {
    // vRect_t full;

// if the window changes dimension, skip this frame

    if (config_notify) {
        fprintf(stderr, "config notify\n");
        config_notify = 0;
        vid.width = config_notify_width & ~7;
        vid.height = config_notify_height;
        if (doShm)
            ResetSharedFrameBuffers();
        else
            ResetFrameBuffer();
        vid.rowbytes = x_framebuffer[0]->bytes_per_line;
        vid.buffer = (uint8_p)x_framebuffer[current_framebuffer]->data;
        vid.conbuffer = vid.buffer;
        vid.conwidth = vid.width;
        vid.conheight = vid.height;
        vid.conrowbytes = vid.rowbytes;
        vid.recalc_refdef = 1;    // force a surface cache flush
        Con_CheckResize();
        Con_Clear_f();
        return;
    }

    // force full update if not 8bit
    if (x_visinfo->depth != 8) {

        scr.fullupdate = 0;
    }


    if (doShm) {

        while (rects) {
            if (x_visinfo->depth == 16)
                st2_fixup(x_framebuffer[current_framebuffer],
                    rects->x, rects->y, rects->width,
                    rects->height);
            else if (x_visinfo->depth == 24)
                st3_fixup(x_framebuffer[current_framebuffer],
                    rects->x, rects->y, rects->width,
                    rects->height);
            if (!XShmPutImage(x_disp, x_win, x_gc,
                x_framebuffer[current_framebuffer], rects->x, rects->y,
                rects->x, rects->y, rects->width, rects->height, True))
                Sys_Error("VID_Update: XShmPutImage failed\n");
            oktodraw = false;
            while (!oktodraw) GetEvent();
            rects = rects->pnext;
        }
        current_framebuffer = !current_framebuffer;
        vid.conbuffer = vid.buffer = (uint8_p)x_framebuffer[current_framebuffer]->data;
        // vid.conbuffer = vid.buffer;
        XSync(x_disp, False);
    }
    else {
        while (rects) {
            if (x_visinfo->depth == 16)
                st2_fixup(x_framebuffer[current_framebuffer],
                    rects->x, rects->y, rects->width,
                    rects->height);
            else if (x_visinfo->depth == 24)
                st3_fixup(x_framebuffer[current_framebuffer],
                    rects->x, rects->y, rects->width,
                    rects->height);
            XPutImage(x_disp, x_win, x_gc, x_framebuffer[0], rects->x,
                rects->y, rects->x, rects->y, rects->width, rects->height);
            rects = rects->pnext;
        }
        XSync(x_disp, False);
    }

}

static int dither;

void VID_DitherOn() {
    if (dither == 0) {
        vid.recalc_refdef = 1;
        dither = 1;
    }
}

void VID_DitherOff() {
    if (dither) {
        vid.recalc_refdef = 1;
        dither = 0;
    }
}

int Sys_OpenWindow() { return 0; }
void Sys_EraseWindow(int window) {}
void Sys_DrawCircle(int window, int x, int y, int r) {}
void Sys_DisplayWindow(int window) {}


void D_BeginDirectRect(int x, int y, uint8_p pbitmap, int width, int height) {
    // direct drawing of the "accessing disk" icon isn't supported under Linux
}

void D_EndDirectRect(int x, int y, int width, int height) {
    // direct drawing of the "accessing disk" icon isn't supported under Linux
}
