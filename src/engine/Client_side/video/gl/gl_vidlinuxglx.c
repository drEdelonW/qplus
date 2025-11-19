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
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include <dlfcn.h>

#include <GL/glx.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>

#define WARP_WIDTH              320
#define WARP_HEIGHT             200

static Display* _dpy = NULL;
static int _scrNum;
static Window _win;
static GLXContext _ctx = NULL;

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )


uint16_t	d_8to16table[256];
uint32_t		d_8to24table[256];
uint8_t	d_15to8table[65536];

cvar_t	vid_mode = { "vid_mode","0",false };

static bool _isMouseAvail;
static bool _isMouseActive;
static int  _mx, _my;
static int	_oldMouseX, _oldMouseY;

static cvar_t in_mouse = { "in_mouse", "1", false };
static cvar_t in_dgamouse = { "in_dgamouse", "1", false };
static cvar_t m_filter = { "m_filter", "0" };

bool dgamouse = false;
bool vidmode_ext = false;

static int _winX, _winY;

static int _scrWidth, _scrHeight;

static XF86VidModeModeInfo** _vidModes;
// static int default_dotclock_vidmode;
static int _numVidModes;
static bool vidmode_active = false;

/*-----------------------------------------------------------------------*/

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;

cvar_t	gl_ztrick = { "gl_ztrick","1" };

cStringRO gl_vendor;
cStringRO gl_renderer;
cStringRO gl_version;
cStringRO gl_extensions;

void (*qglColorTableEXT) (int, int, int, int, int, const TypeLess_ptr);
void (*qgl3DfxSetPaletteEXT) (GLuint*);

static float vid_gamma = 1.0;

bool is8bit = false;
bool isPermedia = false;
bool gl_mtexable = false;

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect(int x, int y, byte* pbitmap, int width, int height) {
}

void D_EndDirectRect(int x, int y, int width, int height) {
}

static int XLateKey(XKeyEvent* ev) {

    int key;
    char buf[NAME_LENGTH];
    KeySym keysym;

    key = 0;

    XLookupString(ev, buf, sizeof buf, &keysym, 0);

    switch (keysym) {
    case XK_KP_Page_Up:
    case XK_Page_Up:	 key = K_PGUP; break;

    case XK_KP_Page_Down:
    case XK_Page_Down:	 key = K_PGDN; break;

    case XK_KP_Home:
    case XK_Home:	 key = K_HOME; break;

    case XK_KP_End:
    case XK_End:	 key = K_END; break;

    case XK_KP_Left:
    case XK_Left:	 key = K_LEFTARROW; break;

    case XK_KP_Right:
    case XK_Right:	key = K_RIGHTARROW;		break;

    case XK_KP_Down:
    case XK_Down:	 key = K_DOWNARROW; break;

    case XK_KP_Up:
    case XK_Up:		 key = K_UPARROW;	 break;

    case XK_Escape: key = K_ESCAPE;		break;

    case XK_KP_Enter:
    case XK_Return: key = K_ENTER;		 break;

    case XK_Tab:		key = K_TAB;			 break;

    case XK_F1:		 key = K_F1;				break;

    case XK_F2:		 key = K_F2;				break;

    case XK_F3:		 key = K_F3;				break;

    case XK_F4:		 key = K_F4;				break;

    case XK_F5:		 key = K_F5;				break;

    case XK_F6:		 key = K_F6;				break;

    case XK_F7:		 key = K_F7;				break;

    case XK_F8:		 key = K_F8;				break;

    case XK_F9:		 key = K_F9;				break;

    case XK_F10:		key = K_F10;			 break;

    case XK_F11:		key = K_F11;			 break;

    case XK_F12:		key = K_F12;			 break;

    case XK_BackSpace: key = K_BACKSPACE; break;

    case XK_KP_Delete:
    case XK_Delete: key = K_DEL; break;

    case XK_Pause:	key = K_PAUSE;		 break;

    case XK_Shift_L:
    case XK_Shift_R:	key = K_SHIFT;		break;

    case XK_Execute:
    case XK_Control_L:
    case XK_Control_R:	key = K_CTRL;		 break;

    case XK_Alt_L:
    case XK_Meta_L:
    case XK_Alt_R:
    case XK_Meta_R: key = K_ALT;			break;

    case XK_KP_Begin: key = '5';	break;

    case XK_KP_Insert:
    case XK_Insert:key = K_INS; break;

    case XK_KP_Multiply: key = '*'; break;
    case XK_KP_Add:  key = '+'; break;
    case XK_KP_Subtract: key = '-'; break;
    case XK_KP_Divide: key = '/'; break;

#if 0
    case 0x021: key = '1';break;/* [!] */
    case 0x040: key = '2';break;/* [@] */
    case 0x023: key = '3';break;/* [#] */
    case 0x024: key = '4';break;/* [$] */
    case 0x025: key = '5';break;/* [%] */
    case 0x05e: key = '6';break;/* [^] */
    case 0x026: key = '7';break;/* [&] */
    case 0x02a: key = '8';break;/* [*] */
    case 0x028: key = '9';;break;/* [(] */
    case 0x029: key = '0';break;/* [)] */
    case 0x05f: key = '-';break;/* [_] */
    case 0x02b: key = '=';break;/* [+] */
    case 0x07c: key = '\'';break;/* [|] */
    case 0x07d: key = '[';break;/* [}] */
    case 0x07b: key = ']';break;/* [{] */
    case 0x022: key = '\'';break;/* ["] */
    case 0x03a: key = ';';break;/* [:] */
    case 0x03f: key = '/';break;/* [?] */
    case 0x03e: key = '.';break;/* [>] */
    case 0x03c: key = ',';break;/* [<] */
#endif

    default:
        key = *(uint8_p)buf;
        if (key >= 'A' && key <= 'Z')
            key = key - 'A' + 'a';
        break;
    }

    return key;
}

static Cursor CreateNullCursor(Display* display, Window root) {
    Pixmap cursormask;
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc = XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
        &dummycolour, &dummycolour, 0, 0);
    XFreePixmap(display, cursormask);
    XFreeGC(display, gc);
    return cursor;
}

static void install_grabs(void) {

    // inviso cursor
    XDefineCursor(_dpy, _win, CreateNullCursor(_dpy, _win));

    XGrabPointer(_dpy, _win,
        True,
        0,
        GrabModeAsync, GrabModeAsync,
        _win,
        None,
        CurrentTime);

    if (in_dgamouse.value) {
        int MajorVersion, MinorVersion;

        if (!XF86DGAQueryVersion(_dpy, &MajorVersion, &MinorVersion)) {
            // unable to query, probalby not supported
            Con_Printf("Failed to detect XF86DGA Mouse\n");
            in_dgamouse.value = 0;
        }
        else {
            dgamouse = true;
            XF86DGADirectVideo(_dpy, DefaultScreen(_dpy), XF86DGADirectMouse);
            XWarpPointer(_dpy, None, _win, 0, 0, 0, 0, 0, 0);
        }
    }
    else {
        XWarpPointer(_dpy, None, _win,
            0, 0, 0, 0,
            vid.width / 2, vid.height / 2);
    }

    XGrabKeyboard(_dpy, _win,
        False,
        GrabModeAsync, GrabModeAsync,
        CurrentTime);

    _isMouseActive = true;

    //	XSync(_dpy, True);
}

static void uninstall_grabs(void) {
    if (!_dpy || !_win)
        return;

    if (dgamouse) {
        dgamouse = false;
        XF86DGADirectVideo(_dpy, DefaultScreen(_dpy), 0);
    }

    XUngrabPointer(_dpy, CurrentTime);
    XUngrabKeyboard(_dpy, CurrentTime);

    // inviso cursor
    XUndefineCursor(_dpy, _win);

    _isMouseActive = false;
}

static void HandleEvents(void) {
    XEvent event;
    KeySym ks;
    int b;
    bool dowarp = false;
    int mwx = vid.width / 2;
    int mwy = vid.height / 2;

    if (!_dpy)
        return;

    while (XPending(_dpy)) {
        XNextEvent(_dpy, &event);

        switch (event.type) {
        case KeyPress:
        case KeyRelease:
            Key_Event(XLateKey(&event.xkey), event.type == KeyPress);
            break;

        case MotionNotify:
            if (_isMouseActive) {
                if (dgamouse) {
                    _mx += (event.xmotion.x + _winX) * 2;
                    _my += (event.xmotion.y + _winY) * 2;
                }
                else {
                    _mx += ((int)event.xmotion.x - mwx) * 2;
                    _my += ((int)event.xmotion.y - mwy) * 2;
                    mwx = event.xmotion.x;
                    mwy = event.xmotion.y;

                    if (_mx || _my)
                        dowarp = true;
                }
            }
            break;

            break;

        case ButtonPress:
            b = -1;
            if (event.xbutton.button == 1)          b = 0;
            else if (event.xbutton.button == 2)     b = 2;
            else if (event.xbutton.button == 3)     b = 1;

            if (b >= 0)     Key_Event(K_MOUSE1 + b, true);
            break;

        case ButtonRelease: // TODO: combine this two
            b = -1;
            if (event.xbutton.button == 1)          b = 0;
            else if (event.xbutton.button == 2)     b = 2;
            else if (event.xbutton.button == 3)     b = 1;

            if (b >= 0)     Key_Event(K_MOUSE1 + b, false);
            break;

        case CreateNotify:
            _winX = event.xcreatewindow.x;
            _winY = event.xcreatewindow.y;
            break;

        case ConfigureNotify:
            _winX = event.xconfigure.x;
            _winY = event.xconfigure.y;
            break;
        }
    }

    if (dowarp) {
        /* move the mouse to the window center again */
        XWarpPointer(_dpy, None, _win, 0, 0, 0, 0, vid.width / 2, vid.height / 2);
    }

}

static void IN_DeactivateMouse(void) {
    if (!_isMouseAvail || !_dpy || !_win)
        return;

    if (_isMouseActive) {
        uninstall_grabs();
        _isMouseActive = false;
    }
}

static void IN_ActivateMouse(void) {
    if (!_isMouseAvail || !_dpy || !_win)
        return;

    if (!_isMouseActive) {
        _mx = _my = 0; // don't spazz
        install_grabs();
        _isMouseActive = true;
    }
}


void VID_Shutdown(void) {
    if (!_ctx || !_dpy)
        return;
    IN_DeactivateMouse();
    if (_dpy) {
        if (_ctx)
            glXDestroyContext(_dpy, _ctx);
        if (_win)
            XDestroyWindow(_dpy, _win);
        if (vidmode_active)
            XF86VidModeSwitchToMode(_dpy, _scrNum, _vidModes[0]);
        XCloseDisplay(_dpy);
    }
    vidmode_active = false;
    _dpy = NULL;
    _win = 0;
    _ctx = NULL;
}

void signal_handler(int sig) {
    printf("Received signal %d, exiting...\n", sig);
    Sys_Quit();
    exit(0);
}

void InitSig(void) {
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGTRAP, signal_handler);
    signal(SIGIOT, signal_handler);
    signal(SIGBUS, signal_handler);
    signal(SIGFPE, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
}

void VID_ShiftPalette(uint8_p p) {
    //	VID_SetPalette(p);
}

void	VID_SetPalette(uint8_p palette) {
    byte* pal;
    uint32_t r, g, b;
    uint32_t v;
    int     r1, g1, b1;
    int		j, k, l, m;
    uint16_t i;
    uint32_p table;
    FILE* f;
    char s[255];
    int dist, bestdist;

    //
    // 8 8 8 encoding
    //
    pal = palette;
    table = d_8to24table;
    for (i = 0; i < 256; i++) {
        r = pal[0];
        g = pal[1];
        b = pal[2];
        pal += 3;

        v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
        *table++ = v;
    }
    d_8to24table[255] &= 0xffffff;	// 255 is transparent

    for (i = 0; i < (1 << 15); i++) {
        /* Maps
        000000000000000
        000000000011111 = Red  = 0x1F
        000001111100000 = Blue = 0x03E0
        111110000000000 = Grn  = 0x7C00
        */
        r = ((i & 0x1F) << 3) + 4;
        g = ((i & 0x03E0) >> 2) + 4;
        b = ((i & 0x7C00) >> 7) + 4;
        pal = (uint8_p)d_8to24table;
        for (v = 0, k = 0, bestdist = 10000 * 10000; v < 256; v++, pal += 4) {
            r1 = (int)r - (int)pal[0];
            g1 = (int)g - (int)pal[1];
            b1 = (int)b - (int)pal[2];
            dist = (r1 * r1) + (g1 * g1) + (b1 * b1);
            if (dist < bestdist) {
                k = v;
                bestdist = dist;
            }
        }
        d_15to8table[i] = k;
    }
}

void CheckMultiTextureExtensions(void) {
    TypeLess_ptr prjobj;

    if (strstr(gl_extensions, "GL_SGIS_multitexture ") && !COM_CheckParm("-nomtex")) {
        Con_Printf("Found GL_SGIS_multitexture...\n");

        if ((prjobj = dlopen(NULL, RTLD_LAZY)) == NULL) {
            Con_Printf("Unable to open symbol list for main program.\n");
            return;
        }

        qglMTexCoord2fSGIS = (TypeLess_ptr)dlsym(prjobj, "glMTexCoord2fSGIS");
        qglSelectTextureSGIS = (TypeLess_ptr)dlsym(prjobj, "glSelectTextureSGIS");

        if (qglMTexCoord2fSGIS && qglSelectTextureSGIS) {
            Con_Printf("Multitexture extensions found.\n");
            gl_mtexable = true;
        }
        else
            Con_Printf("Symbol not found, disabled.\n");

        dlclose(prjobj);
    }
}

/*
===============
GL_Init
===============
*/
void GL_Init(void) {
    gl_vendor = glGetString(GL_VENDOR);
    Con_Printf("GL_VENDOR: %s\n", gl_vendor);
    gl_renderer = glGetString(GL_RENDERER);
    Con_Printf("GL_RENDERER: %s\n", gl_renderer);

    gl_version = glGetString(GL_VERSION);
    Con_Printf("GL_VERSION: %s\n", gl_version);
    gl_extensions = glGetString(GL_EXTENSIONS);
    Con_Printf("GL_EXTENSIONS: %s\n", gl_extensions);

    //	Con_Printf ("%s %s\n", gl_renderer, gl_version);

    CheckMultiTextureExtensions();

    glClearColor(1, 0, 0, 0);
    glCullFace(GL_FRONT);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.666);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel(GL_FLAT);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering(int* x, int* y, int* width, int* height) {

    *x = *y = 0;
    *width = _scrWidth;
    *height = _scrHeight;

    //    if (!wglMakeCurrent( maindc, baseRC ))
    //		Host_SysError ("wglMakeCurrent failed");

    //	glViewport (*x, *y, *width, *height);
}


void GL_EndRendering(void) {
    glFlush();
    glXSwapBuffers(_dpy, _win);
}

bool VID_Is8bit(void) {
    return is8bit;
}

void VID_Init8bitPalette(void) {
    // Check for 8bit Extensions and initialize them.
    int i;
    TypeLess_ptr prjobj;

    if ((prjobj = dlopen(NULL, RTLD_LAZY)) == NULL) {
        Con_Printf("Unable to open symbol list for main program.\n");
        return;
    }

    if (strstr(gl_extensions, "3DFX_set_global_palette") &&
        (qgl3DfxSetPaletteEXT = dlsym(prjobj, "gl3DfxSetPaletteEXT")) != NULL) {
        GLubyte table[256][4];
        int8_p oldpal;

        Con_SafePrintf("8-bit GL extensions enabled.\n");
        glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
        oldpal = (int8_p)d_8to24table; //d_8to24table3dfx;
        for (i = 0;i < 256;i++) {
            table[i][2] = *oldpal++;
            table[i][1] = *oldpal++;
            table[i][0] = *oldpal++;
            table[i][3] = 255;
            oldpal++;
        }
        qgl3DfxSetPaletteEXT((GLuint*)table);
        is8bit = true;

    }
    else if (strstr(gl_extensions, "GL_EXT_shared_texture_palette") &&
        (qglColorTableEXT = dlsym(prjobj, "glColorTableEXT")) != NULL) {
        char thePalette[256 * 3];
        int8_p oldPalette, * newPalette;

        Con_SafePrintf("8-bit GL extensions enabled.\n");
        glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
        oldPalette = (int8_p)d_8to24table; //d_8to24table3dfx;
        newPalette = thePalette;
        for (i = 0;i < 256;i++) {
            *newPalette++ = *oldPalette++;
            *newPalette++ = *oldPalette++;
            *newPalette++ = *oldPalette++;
            oldPalette++;
        }
        qglColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE, (TypeLess_ptr)thePalette);
        is8bit = true;
    }

    dlclose(prjobj);
}

static void Check_Gamma(uint8_p pal) {
    float	f, inf;
    uint8_t	palette[768];
    int		i;

    if ((i = COM_CheckParm("-gamma")) == 0) {
        if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
            (gl_vendor && strstr(gl_vendor, "3Dfx")))
            vid_gamma = 1;
        else
            vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
    }
    else
        vid_gamma = Q_atof(com.argv[i + 1]);

    for (i = 0; i < 768; i++) {
        f = pow((pal[i] + 1) / 256.0, vid_gamma);
        inf = f * 255 + 0.5;
        if (inf < 0)
            inf = 0;
        if (inf > 255)
            inf = 255;
        palette[i] = inf;
    }

    memcpy(pal, palette, sizeof(palette));
}

void VID_Init(uint8_p palette) {
    int i;
    int attrib[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE, 1,
        None
    };
    char	gldir[MAX_OSPATH];
    int width = 640, height = 480;
    XSetWindowAttributes attr;
    uint32_t mask;
    Window root;
    XVisualInfo* visinfo;
    bool fullscreen = true;
    int MajorVersion, MinorVersion;
    int actualWidth, actualHeight;

    Cvar_RegisterVariable(&vid_mode);
    Cvar_RegisterVariable(&in_mouse);
    Cvar_RegisterVariable(&in_dgamouse);
    Cvar_RegisterVariable(&m_filter);
    Cvar_RegisterVariable(&gl_ztrick);

    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int*)vid.colormap + 2048));

    // interpret command-line params

    // set vid parameters
    if ((i = COM_CheckParm("-window")) != 0)
        fullscreen = false;

    if ((i = COM_CheckParm("-width")) != 0)
        width = atoi(com.argv[i + 1]);

    if ((i = COM_CheckParm("-height")) != 0)
        height = atoi(com.argv[i + 1]);

    if ((i = COM_CheckParm("-conwidth")) != 0)
        vid.conwidth = Q_atoi(com.argv[i + 1]);
    else
        vid.conwidth = 640;

    vid.conwidth &= 0xfff8; // make it a multiple of eight

    if (vid.conwidth < 320)     vid.conwidth = 320;

    // pick a conheight that matches with correct aspect
    vid.conheight = vid.conwidth * 3 / 4;

    if ((i = COM_CheckParm("-conheight")) != 0)
        vid.conheight = Q_atoi(com.argv[i + 1]);
    if (vid.conheight < 200)
        vid.conheight = 200;

    if (!(_dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Error couldn't open the X display\n");
        exit(1);
    }

    _scrNum = DefaultScreen(_dpy);
    root = RootWindow(_dpy, _scrNum);

    // Get video mode list
    MajorVersion = MinorVersion = 0;
    if (!XF86VidModeQueryVersion(_dpy, &MajorVersion, &MinorVersion)) {
        vidmode_ext = false;
    }
    else {
        Con_Printf("Using XFree86-VidModeExtension Version %d.%d\n", MajorVersion, MinorVersion);
        vidmode_ext = true;
    }

    visinfo = glXChooseVisual(_dpy, _scrNum, attrib);
    if (!visinfo) {
        fprintf(stderr, "qkHack: Error couldn't get an RGB, Double-buffered, Depth visual\n");
        exit(1);
    }

    if (vidmode_ext) {
        int best_fit, best_dist, dist, x, y;

        XF86VidModeGetAllModeLines(_dpy, _scrNum, &_numVidModes, &_vidModes);

        // Are we going fullscreen?  If so, let's change video mode
        if (fullscreen) {
            best_dist = 9999999;
            best_fit = -1;

            for (i = 0; i < _numVidModes; i++) {
                if (width > _vidModes[i]->hdisplay ||
                    height > _vidModes[i]->vdisplay)
                    continue;

                x = width - _vidModes[i]->hdisplay;
                y = height - _vidModes[i]->vdisplay;
                dist = (x * x) + (y * y);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_fit = i;
                }
            }

            if (best_fit != -1) {
                actualWidth = _vidModes[best_fit]->hdisplay;
                actualHeight = _vidModes[best_fit]->vdisplay;

                // change to the mode
                XF86VidModeSwitchToMode(_dpy, _scrNum, _vidModes[best_fit]);
                vidmode_active = true;

                // Move the viewport to top left
                XF86VidModeSetViewPort(_dpy, _scrNum, 0, 0);
            }
            else
                fullscreen = 0;
        }
    }

    /* window attributes */
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(_dpy, root, visinfo->visual, AllocNone);
    attr.event_mask = X_MASK;
    if (vidmode_active) {
        mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore |
            CWEventMask | CWOverrideRedirect;
        attr.override_redirect = True;
        attr.backing_store = NotUseful;
        attr.save_under = False;
    }
    else
        mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    _win = XCreateWindow(_dpy, root, 0, 0, width, height,
        0, visinfo->depth, InputOutput,
        visinfo->visual, mask, &attr);
    XMapWindow(_dpy, _win);

    if (vidmode_active) {
        XMoveWindow(_dpy, _win, 0, 0);
        XRaiseWindow(_dpy, _win);
        XWarpPointer(_dpy, None, _win, 0, 0, 0, 0, 0, 0);
        XFlush(_dpy);
        // Move the viewport to top left
        XF86VidModeSetViewPort(_dpy, _scrNum, 0, 0);
    }

    XFlush(_dpy);

    _ctx = glXCreateContext(_dpy, visinfo, NULL, True);

    glXMakeCurrent(_dpy, _win, _ctx);

    _scrWidth = width;
    _scrHeight = height;

    if (vid.conheight > height)     vid.conheight = height;
    if (vid.conwidth > width)       vid.conwidth = width;
    vid.width = vid.conwidth;
    vid.height = vid.conheight;

    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 2;

    InitSig(); // trap evil signals

    GL_Init();

    snprintf(gldir, sizeof(gldir), "%s/glquake", com.gamedir);
    Sys_mkdir(gldir);

    VID_SetPalette(palette);

    // Check for 3DFX Extensions and initialize them.
    VID_Init8bitPalette();

    Con_SafePrintf("Video mode %dx%d initialized.\n", width, height);

    vid.recalc_refdef = 1;				// force a surface cache flush
}

void Sys_SendKeyEvents(void) {
    HandleEvents();
}

void Force_CenterView_f(void) {
    cl.viewangles[PITCH] = 0;
}

void IN_Init(void) {}

void IN_Shutdown(void) {}

/*
===========
IN_Commands
===========
*/
void IN_Commands(void) {
    if (!_dpy || !_win)
        return;

    if (vidmode_active || (key.dest == key_game))   IN_ActivateMouse();
    else                                            IN_DeactivateMouse();
}

/*
===========
IN_Move
===========
*/
void IN_MouseMove(UserCmd_p cmd) {
    if (!_isMouseAvail)
        return;

    if (m_filter.value) {
        _mx = (_mx + _oldMouseX) * 0.5;
        _my = (_my + _oldMouseY) * 0.5;
    }
    _oldMouseX = _mx;
    _oldMouseY = _my;

    _mx *= sensitivity.value;
    _my *= sensitivity.value;

    // add mouse X/Y movement to cmd
    if ((in.strafe.state & 1) || (lookstrafe.value && (in.mlook.state & 1)))
        cmd->sidemove += m_side.value * _mx;
    else
        cl.viewangles[YAW] -= m_yaw.value * _mx;

    if (in.mlook.state & 1)
        V_StopPitchDrift();

    if ((in.mlook.state & 1) && !(in.strafe.state & 1)) {
        cl.viewangles[PITCH] += m_pitch.value * _my;
        if (cl.viewangles[PITCH] > 80)      cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)     cl.viewangles[PITCH] = -70;
    }
    else {
        if ((in.strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * _my;
        else
            cmd->forwardmove -= m_forward.value * _my;
    }
    _mx = my = 0;
}

void IN_Move(UserCmd_p cmd) {
    IN_MouseMove(cmd);
}


