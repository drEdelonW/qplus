#include "vid.h"
#include "x_prv.h"
#include "cvar.h"
#include "common.h"
#include "console.h"
#include "qOpenGL.h"
#ifdef GLQUAKE
#   undef GLQUAKE
#   include "d_iface.h"     // WARP_HEIGHT
#endif
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

// cvar_t	vid_mode = { "vid_mode","0",false };
// static cvar_t in_mouse = { "in_mouse", "1", false };
// static cvar_t in_dgamouse = { "in_dgamouse", "1", false };
// static cvar_t m_filter = { "m_filter", "0" };

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
cString gl_vendor;
cString gl_version;
cString gl_extensions;

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

    	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

static GLXContext _ctx = NULL;
static int _scrWidth, _scrHeight;

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )

typedef struct GlxState_s {
    int             err_base;
    int             ev_base;
    int             major;
    int             minor;

    XVisualInfo*    visual;
    GLXContext      context;
    int             doublebuffer;
    bool            is_direct;
} GlxState_t;

typedef struct WindowState_s {
    Window      window;
    Colormap    colormap;
    Atom        wm_delete;

    int         width;
    int         height;
} WindowState_t;

typedef struct App_s {
    Display*        display;
    int             screen;
    bool            running;

    GlxState_t      glx;
    WindowState_t   xwin;
} App_t;

typedef App_t* App_p;

App_t app;
int g_x_error_seen = 0;

int x_error_handler(Display* dpy, XErrorEvent* ev) {
    char text[256];

    XGetErrorText(dpy, ev->error_code, text, sizeof(text));

    Con_Printf(
        "[XERROR] error=%d (%s) request=%d minor=%d resource=0x%lx serial=%lu\n",
        ev->error_code,
        text,
        ev->request_code,
        ev->minor_code,
        ev->resourceid,
        ev->serial
    );

    g_x_error_seen = 1;
    return 0;
}

void app_zero(App_p app) {
    if (!app)
        return;

    *app = (App_t){
        .glx = {
            .major = 1,
            .minor = 0,
        },
        .xwin = {
            .width = 320,
            .height = 200,
        },
        .running = true,
    };
}

int open_display(App_p app) {
    Con_Printf("[START] X11 + GLX Quake video init\n");

    app->display = XOpenDisplay(NULL);
    if (!app->display) {
        Host_SysError("VID_Init: XOpenDisplay failed\n");
        return 0;
    }

    app->screen = DefaultScreen(app->display);

    x_disp = app->display;
    _scrNum = app->screen;

    Con_Printf("[OK] XOpenDisplay\n");
    Con_Printf("[X11] vendor=%s\n", ServerVendor(app->display));

    return 1;
}

int query_glx(App_p app) {
    if (!glXQueryExtension(app->display, &app->glx.err_base, &app->glx.ev_base)) {
        Host_SysError("VID_Init: GLX extension not available\n");
        return 0;
    }

    Con_Printf(
        "[OK] glXQueryExtension err_base=%d ev_base=%d\n",
        app->glx.err_base,
        app->glx.ev_base
    );

    if (!glXQueryVersion(app->display, &app->glx.major, &app->glx.minor)) {
        Host_SysError("VID_Init: glXQueryVersion failed\n");
        return 0;
    }

    Con_Printf("[GLX] version=%d.%d\n", app->glx.major, app->glx.minor);

    return 1;
}

XVisualInfo* try_choose_visual(
    Display* dpy,
    int screen,
    const char* name,
    int* attrs,
    int doublebuffer,
    int* out_doublebuffer
) {
    XVisualInfo* vi;

    Con_Printf("[TRY] glXChooseVisual: %s\n", name);

    vi = glXChooseVisual(dpy, screen, attrs);
    if (!vi) {
        Con_Printf("[FAIL] %s\n", name);
        return NULL;
    }

    *out_doublebuffer = doublebuffer;

    Con_Printf("[OK] visual: %s\n", name);
    Con_Printf(
        "[VISUAL] visualid=0x%lx depth=%d class=%d\n",
        vi->visualid,
        vi->depth,
        vi->class
    );

    return vi;
}

int choose_visual(App_p app) {
    int value;

    int attrs_rgba_db[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        None
    };

    int attrs_rgba_rgb_db[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_DOUBLEBUFFER,
        None
    };

    int attrs_rgba[] = {
        GLX_RGBA,
        None
    };

    int attrs_rgba_rgb[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        None
    };

    app->glx.visual = try_choose_visual(
        app->display,
        app->screen,
        "RGBA + doublebuffer",
        attrs_rgba_db,
        1,
        &app->glx.doublebuffer
    );

    if (!app->glx.visual) {
        app->glx.visual = try_choose_visual(
            app->display,
            app->screen,
            "RGBA + RGB 8:8:8 + doublebuffer",
            attrs_rgba_rgb_db,
            1,
            &app->glx.doublebuffer
        );
    }

    if (!app->glx.visual) {
        app->glx.visual = try_choose_visual(
            app->display,
            app->screen,
            "RGBA only",
            attrs_rgba,
            0,
            &app->glx.doublebuffer
        );
    }

    if (!app->glx.visual) {
        app->glx.visual = try_choose_visual(
            app->display,
            app->screen,
            "RGBA + RGB 8:8:8",
            attrs_rgba_rgb,
            0,
            &app->glx.doublebuffer
        );
    }

    if (!app->glx.visual) {
        Host_SysError("VID_Init: no GLX visual\n");
        return 0;
    }

    value = 0;
    if (glXGetConfig(app->display, app->glx.visual, GLX_DOUBLEBUFFER, &value) == 0)
        Con_Printf("[VISUAL] GLX_DOUBLEBUFFER=%d\n", value);

    value = 0;
    if (glXGetConfig(app->display, app->glx.visual, GLX_DEPTH_SIZE, &value) == 0)
        Con_Printf("[VISUAL] GLX_DEPTH_SIZE=%d\n", value);

    return 1;
}

int create_window(App_p app) {
    XSetWindowAttributes swa;

    app->xwin.colormap = XCreateColormap(
        app->display,
        RootWindow(app->display, app->glx.visual->screen),
        app->glx.visual->visual,
        AllocNone
    );

    memset(&swa, 0, sizeof(swa));

    swa.colormap = app->xwin.colormap;
    swa.border_pixel = 0;
    swa.event_mask = X_MASK;

    app->xwin.window = XCreateWindow(
        app->display,
        RootWindow(app->display, app->glx.visual->screen),
        0,
        0,
        app->xwin.width,
        app->xwin.height,
        0,
        app->glx.visual->depth,
        InputOutput,
        app->glx.visual->visual,
        CWBorderPixel | CWColormap | CWEventMask,
        &swa
    );

    if (!app->xwin.window) {
        Host_SysError("VID_Init: XCreateWindow failed\n");
        return 0;
    }

    app->xwin.wm_delete = XInternAtom(app->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app->display, app->xwin.window, &app->xwin.wm_delete, 1);

    XStoreName(app->display, app->xwin.window, "Quake GLX");
    XMapWindow(app->display, app->xwin.window);
    XFlush(app->display);

    x_win = app->xwin.window;

    Con_Printf("[OK] XCreateWindow win=0x%lx\n", app->xwin.window);
    Con_Printf("[OK] XMapWindow\n");

    return 1;
}

int wait_for_map(App_p app) {
    for (;;) {
        XEvent ev;

        XNextEvent(app->display, &ev);

        if (ev.type == MapNotify && ev.xmap.window == app->xwin.window) {
            Con_Printf("[OK] MapNotify\n");
            return 1;
        }

        if (ev.type == DestroyNotify) {
            Host_SysError("VID_Init: DestroyNotify before MapNotify\n");
            return 0;
        }

        if (ev.type == ConfigureNotify) {
            app->xwin.width = ev.xconfigure.width;
            app->xwin.height = ev.xconfigure.height;
        }
    }
}

GLXContext try_create_context(App_p app, Bool direct) {
    GLXContext ctx;

    Con_Printf("[TRY] glXCreateContext direct=%s\n", direct ? "True" : "False");

    g_x_error_seen = 0;
    XSetErrorHandler(x_error_handler);

    ctx = glXCreateContext(app->display, app->glx.visual, NULL, direct);

    XSync(app->display, False);
    XSetErrorHandler(NULL);

    if (g_x_error_seen || !ctx) {
        Con_Printf(
            "[FAIL] glXCreateContext direct=%s ctx=%p x_error_seen=%d\n",
            direct ? "True" : "False",
            (void*)ctx,
            g_x_error_seen
        );

        if (ctx)
            glXDestroyContext(app->display, ctx);

        return NULL;
    }

    Con_Printf("[OK] glXCreateContext ctx=%p\n", (void*)ctx);
    return ctx;
}

int create_gl_context(App_p app) {
    app->glx.context = try_create_context(app, True);

    if (!app->glx.context)
        app->glx.context = try_create_context(app, False);

    if (!app->glx.context) {
        Host_SysError("VID_Init: glXCreateContext failed\n");
        return 0;
    }

    _ctx = app->glx.context;

    app->glx.is_direct = glXIsDirect(app->display, app->glx.context) ? true : false;
    Con_Printf("[GLX] glXIsDirect=%d\n", app->glx.is_direct);

    Con_Printf("[TRY] glXMakeCurrent\n");

    g_x_error_seen = 0;
    XSetErrorHandler(x_error_handler);

    if (!glXMakeCurrent(app->display, app->xwin.window, app->glx.context)) {
        XSync(app->display, False);
        XSetErrorHandler(NULL);

        Host_SysError("VID_Init: glXMakeCurrent returned False\n");
        return 0;
    }

    XSync(app->display, False);
    XSetErrorHandler(NULL);

    if (g_x_error_seen) {
        Host_SysError("VID_Init: glXMakeCurrent produced X error\n");
        return 0;
    }

    Con_Printf("[OK] glXMakeCurrent\n");

    Con_Printf("[GL] vendor=%s\n", glGetString(GL_VENDOR));
    Con_Printf("[GL] renderer=%s\n", glGetString(GL_RENDERER));
    Con_Printf("[GL] version=%s\n", glGetString(GL_VERSION));

    return 1;
}

void apply_vid_state(App_p app) {
    _scrWidth = app->xwin.width;
    _scrHeight = app->xwin.height;

    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int*)vid.colormap + 2048));

    vid.width = app->xwin.width;
    vid.height = app->xwin.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0f / 240.0f);
    vid.numpages = 2;
    vid.recalc_refdef = 1;

    glViewport(0, 0, app->xwin.width, app->xwin.height);
}

int init(App_p app) {
    app_zero(app);

    if (!open_display(app))         return 0;
    if (!query_glx(app))            return 0;
    if (!choose_visual(app))        return 0;
    if (!create_window(app))        return 0;
    if (!wait_for_map(app))         return 0;
    if (!create_gl_context(app))    return 0;

    apply_vid_state(app);

    return 1;
}

void VID_Init(uint8_p palette) {
    (void)palette;

    if (!init(&app))
        Host_SysError("VID_Init: init failed\n");

    GL_Init();

    Con_SafePrintf(
        "Video mode %dx%d initialized.\n",
        vid.width,
        vid.height
    );

    oktodraw = true;
}