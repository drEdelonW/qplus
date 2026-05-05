#include "vid.h"
#include "x_prv.h"
#include "cvar.h"
#include "common.h"
#include "console.h"
#include "qOpenGL.h"
#include "d_iface.h"
#include "host.h"
#include "endian_tools.h"
#include "q_tools.h"
#include <stdlib.h>  // for atoi()
#include <string.h>
// #include <stdio.h>
// #include <sys/ioctl.h>
#include <dlfcn.h>
#include <GL/gl.h>
#include <GL/glx.h>

bool gl_mtexable;
const char* gl_renderer;
cvar_t gl_ztrick;

float gldepthmin;
float gldepthmax;

bool isPermedia;

int texture_extension_number;
int texture_mode;

int config_notify = 0;
int config_notify_width;
int config_notify_height;

uint8_t	d_15to8table[65536];
bool    doShm;
bool    oktodraw = false;
Display*    x_disp = NULL;
// static Display* _dpy = NULL;
static int _scrNum;
Window      x_win;
int         x_shmeventtype;
bool vidmode_ext = false;

cvar_t	vid_mode = { "vid_mode","0",false };
static cvar_t in_mouse = { "in_mouse", "1", false };
static cvar_t in_dgamouse = { "in_dgamouse", "1", false };
static cvar_t m_filter = { "m_filter", "0" };

// it live in d_surf.c
void D_InitCaches(TypeLess_ptr buffer, int size) {
    (void)buffer;
    (void)size;
}

void GL_BeginRendering(int *x, int *y, int *width, int *height) {
    *x = 0;
    *y = 0;
    *width = vid.width;
    *height = vid.height;
}

void GL_EndRendering(void) {
    glFlush();
    glXSwapBuffers(x_disp, x_win);
}

bool is8bit = false;

bool VID_Is8bit() {
    return is8bit;
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
cStringRO gl_vendor;
cStringRO gl_version;
cStringRO gl_extensions;

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

#if 0
void VID_Init(uint8_p palette) {
    int i;
    GLint attribs[32];
    char	gldir[MAX_OSPATH];
    int width = 640, height = 480;

    // Init_KBD();

    // Cvar_RegisterVariable(&vid_mode);
    // Cvar_RegisterVariable(&vid_redrawfull);
    // Cvar_RegisterVariable(&vid_waitforrefresh);
    // Cvar_RegisterVariable(&gl_ztrick);

    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int*)vid.colormap + 2048));

    // interpret command-line params

    // set vid parameters
    attribs[0] = FXMESA_DOUBLEBUFFER;
    attribs[1] = FXMESA_ALPHA_SIZE;
    attribs[2] = 1;
    attribs[3] = FXMESA_DEPTH_SIZE;
    attribs[4] = 1;
    attribs[5] = FXMESA_NONE;

    if ((i = COM_CheckParm("-width")) != 0)     width = atoi(com.argv[i + 1]);
    if ((i = COM_CheckParm("-height")) != 0)    height = atoi(com.argv[i + 1]);

    if ((i = COM_CheckParm("-conwidth")) != 0)  vid.conwidth = Q_atoi(com.argv[i + 1]);
    else                                        vid.conwidth = 640;

    vid.conwidth &= 0xfff8; // make it a multiple of eight

    if (vid.conwidth < 320)        vid.conwidth = 320;

    // pick a conheight that matches with correct aspect
    vid.conheight = vid.conwidth * 3 / 4;

    if ((i = COM_CheckParm("-conheight")) != 0)     vid.conheight = Q_atoi(com.argv[i + 1]);
    if (vid.conheight < 200)                        vid.conheight = 200;

    _fc = fxMesaCreateContext(0, findres(&width, &height), GR_REFRESH_75Hz,
        attribs);
    if (!_fc)
        Host_SysError("Unable to create 3DFX context.\n");

    InitSig(); // trap evil signals

    scr_width = width;
    scr_height = height;

    fxMesaMakeCurrent(_fc);

    if (vid.conheight > height)     vid.conheight = height;
    if (vid.conwidth > width)       vid.conwidth = width;
    vid.width = vid.conwidth;
    vid.height = vid.conheight;

    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 2;

    GL_Init();

    snprintf(gldir, sizeof(gldir), "%s/glquake", com.gamedir);
    Sys_mkdir(gldir);

    Check_Gamma(palette);
    VID_SetPalette(palette);

    // Check for 3DFX Extensions and initialize them.
    VID_Init8bitPalette();

    Con_SafePrintf("Video mode %dx%d initialized.\n", width, height);

    vid.recalc_refdef = 1;				// force a surface cache flush
}
#else
static GLXContext _ctx = NULL;
static int _scrWidth, _scrHeight;

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )


void VID_Init(uint8_p palette) {
    (void)palette;

    int i;
    int width = 640/2;
    int height = 480/2;

    int attrib[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_RED_SIZE,   8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE,  8,
        GLX_DEPTH_SIZE, 24,
        None
    };

    Window root;
    XVisualInfo* visinfo;
    XSetWindowAttributes attr;
    uint32_t mask;

    if ((i = COM_CheckParm("-width")) != 0)
        width = atoi(com.argv[i + 1]);

    if ((i = COM_CheckParm("-height")) != 0)
        height = atoi(com.argv[i + 1]);

    if ((i = COM_CheckParm("-conwidth")) != 0)
        vid.conwidth = Q_atoi(com.argv[i + 1]);
    else
        vid.conwidth = width;

    vid.conwidth &= 0xfff8;

    if (vid.conwidth < 320)
        vid.conwidth = 320;

    if ((i = COM_CheckParm("-conheight")) != 0)
        vid.conheight = Q_atoi(com.argv[i + 1]);
    else
        vid.conheight = vid.conwidth * height / width;

    if (vid.conheight < 200)
        vid.conheight = 200;

    if (vid.conwidth > width)
        vid.conwidth = width;

    if (vid.conheight > height)
        vid.conheight = height;

    x_disp = XOpenDisplay(NULL);
    if (!x_disp)
        Host_SysError("VID_Init: XOpenDisplay failed\n");

    _scrNum = DefaultScreen(x_disp);
    root = RootWindow(x_disp, _scrNum);

    visinfo = glXChooseVisual(x_disp, _scrNum, attrib);
    if (!visinfo)
        Host_SysError("VID_Init: glXChooseVisual failed\n");

    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(x_disp, root, visinfo->visual, AllocNone);
    attr.event_mask = X_MASK;

    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    x_win = XCreateWindow(
        x_disp,
        root,
        0, 0,
        width, height,
        0,
        visinfo->depth,
        InputOutput,
        visinfo->visual,
        mask,
        &attr
    );

    XStoreName(x_disp, x_win, "Quake GLX");
    XMapWindow(x_disp, x_win);
    XFlush(x_disp);

    _ctx = glXCreateContext(x_disp, visinfo, NULL, True);
    if (!_ctx)
        Host_SysError("VID_Init: glXCreateContext failed\n");

    XSync(x_disp, False);

    Con_Printf("x_win = 0x%lx\n", x_win);
    Con_Printf("ctx direct = %d\n", glXIsDirect(x_disp, _ctx));
    if (!glXMakeCurrent(x_disp, x_win, _ctx))
        Host_SysError("VID_Init: glXMakeCurrent failed\n");

    XFree(visinfo);

    _scrWidth = width;
    _scrHeight = height;

    vid.width = vid.conwidth;
    vid.height = vid.conheight;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0f / 240.0f);
    vid.numpages = 2;
    vid.recalc_refdef = 1;

    glViewport(0, 0, width, height);

    GL_Init();

    Con_SafePrintf("Video mode %dx%d initialized.\n", width, height);

    oktodraw = true;
}
#endif