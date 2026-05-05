#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <GL/gl.h>
#include <GL/glx.h>

typedef struct GlxState_s {
    int err_base;
    int ev_base;
    int major;
    int minor;

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
    Display*    display;
    int         screen;
    bool        running;

    GlxState_t      glx;
    WindowState_t   xwin;
} App_t;
typedef App_t* App_p;

int g_x_error_seen = 0;

int x_error_handler(Display* dpy, XErrorEvent* ev) {
    char text[256];

    XGetErrorText(dpy, ev->error_code, text, sizeof(text));

    fprintf(stderr,
        "[XERROR] error=%d (%s) request=%d minor=%d resource=0x%lx serial=%lu\n",
        ev->error_code,
        text,
        ev->request_code,
        ev->minor_code,
        ev->resourceid,
        ev->serial);

    g_x_error_seen = 1;
    return 0;
}

void app_zero(App_p app) {
    if (!app) return;
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
    printf("[START] X11 + GLX triangle test\n");

    printf("[ENV] DISPLAY=%s\n",
        getenv("DISPLAY") ? getenv("DISPLAY") : "(null)");

    printf("[ENV] LIBGL_ALWAYS_INDIRECT=%s\n",
        getenv("LIBGL_ALWAYS_INDIRECT") ? getenv("LIBGL_ALWAYS_INDIRECT") : "(null)");

    app->display = XOpenDisplay(NULL);
    if (!app->display) {
        fprintf(stderr, "[FATAL] XOpenDisplay failed\n");
        return 0;
    }

    app->screen = DefaultScreen(app->display);

    printf("[OK] XOpenDisplay\n");
    printf("[X11] vendor=%s\n", ServerVendor(app->display));

    return 1;
}

int query_glx(App_p app) {
    if (!glXQueryExtension(app->display, &app->glx.err_base, &app->glx.ev_base)) {
        fprintf(stderr, "[FATAL] GLX extension not available\n");   return 0;
    }

    printf(
        "[OK] glXQueryExtension err_base=%d ev_base=%d\n",
        app->glx.err_base,
        app->glx.ev_base
    );

    if (!glXQueryVersion(app->display, &app->glx.major, &app->glx.minor)) {
        fprintf(stderr, "[FATAL] glXQueryVersion failed\n");        return 0;
    }

    printf("[GLX] version=%d.%d\n", app->glx.major, app->glx.minor);

    return 1;
}

XVisualInfo* try_choose_visual(
    Display* dpy,
    int screen,
    const char* name,
    int* attrs,
    int doublebuffer,
    int* out_doublebuffer) {
    XVisualInfo* vi;

    printf("[TRY] glXChooseVisual: %s\n", name);

    vi = glXChooseVisual(dpy, screen, attrs);
    if (!vi) {
        printf("[FAIL] %s\n", name);
        return NULL;
    }

    *out_doublebuffer = doublebuffer;

    printf("[OK] visual: %s\n", name);
    printf("[VISUAL] visualid=0x%lx depth=%d class=%d\n",
        vi->visualid,
        vi->depth,
        vi->class);

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

    app->glx.visual = try_choose_visual(app->display,
        app->screen,
        "RGBA + doublebuffer",
        attrs_rgba_db,
        1,
        &app->glx.doublebuffer);

    if (!app->glx.visual) {
        app->glx.visual = try_choose_visual(app->display,
            app->screen,
            "RGBA + RGB 8:8:8 + doublebuffer",
            attrs_rgba_rgb_db,
            1,
            &app->glx.doublebuffer);
    }

    if (!app->glx.visual) {
        app->glx.visual = try_choose_visual(app->display,
            app->screen,
            "RGBA only",
            attrs_rgba,
            0,
            &app->glx.doublebuffer);
    }

    if (!app->glx.visual) {
        app->glx.visual = try_choose_visual(app->display,
            app->screen,
            "RGBA + RGB 8:8:8",
            attrs_rgba_rgb,
            0,
            &app->glx.doublebuffer);
    }

    if (!app->glx.visual) {
        fprintf(stderr, "[FATAL] no GLX visual\n");
        return 0;
    }

    value = 0;
    if (glXGetConfig(app->display, app->glx.visual, GLX_DOUBLEBUFFER, &value) == 0)
        printf("[VISUAL] GLX_DOUBLEBUFFER=%d\n", value);

    value = 0;
    if (glXGetConfig(app->display, app->glx.visual, GLX_DEPTH_SIZE, &value) == 0)
        printf("[VISUAL] GLX_DEPTH_SIZE=%d\n", value);

    return 1;
}

int create_window(App_p app) {
    XSetWindowAttributes swa;

    app->xwin.colormap = XCreateColormap(app->display,
        RootWindow(app->display, app->glx.visual->screen),
        app->glx.visual->visual,
        AllocNone);

    memset(&swa, 0, sizeof(swa));

    swa.colormap = app->xwin.colormap;
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask |
        KeyPressMask |
        StructureNotifyMask;

    app->xwin.window = XCreateWindow(app->display,
        RootWindow(app->display, app->glx.visual->screen),
        0, 0,
        app->xwin.width,
        app->xwin.height,
        0,
        app->glx.visual->depth,
        InputOutput,
        app->glx.visual->visual,
        CWBorderPixel | CWColormap | CWEventMask,
        &swa);

    if (!app->xwin.window) {
        fprintf(stderr, "[FATAL] XCreateWindow failed\n");
        return 0;
    }

    app->xwin.wm_delete = XInternAtom(app->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app->display, app->xwin.window, &app->xwin.wm_delete, 1);

    XStoreName(app->display, app->xwin.window, "X11 GLX triangle test");
    XMapWindow(app->display, app->xwin.window);
    XFlush(app->display);

    printf("[OK] XCreateWindow win=0x%lx\n", app->xwin.window);
    printf("[OK] XMapWindow\n");

    return 1;
}

int wait_for_map(App_p app) {
    for (;;) {
        XEvent ev;

        XNextEvent(app->display, &ev);

        if (ev.type == MapNotify && ev.xmap.window == app->xwin.window) {
            printf("[OK] MapNotify\n");
            return 1;
        }

        if (ev.type == DestroyNotify) {
            fprintf(stderr, "[FATAL] DestroyNotify before MapNotify\n");
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

    printf("[TRY] glXCreateContext direct=%s\n", direct ? "True" : "False");

    g_x_error_seen = 0;
    XSetErrorHandler(x_error_handler);

    ctx = glXCreateContext(app->display, app->glx.visual, NULL, direct);

    XSync(app->display, False);
    XSetErrorHandler(NULL);

    if (g_x_error_seen || !ctx) {
        printf("[FAIL] glXCreateContext direct=%s ctx=%p x_error_seen=%d\n",
            direct ? "True" : "False",
            (void*)ctx,
            g_x_error_seen);

        if (ctx)
            glXDestroyContext(app->display, ctx);

        return NULL;
    }

    printf("[OK] glXCreateContext ctx=%p\n", (void*)ctx);
    return ctx;
}

int create_gl_context(App_p app) {
    app->glx.context = try_create_context(app, True);

    if (!app->glx.context)
        app->glx.context = try_create_context(app, False);

    if (!app->glx.context) {
        fprintf(stderr, "[FATAL] glXCreateContext failed\n");
        return 0;
    }

    app->glx.is_direct = glXIsDirect(app->display, app->glx.context);
    printf("[GLX] glXIsDirect=%d\n", app->glx.is_direct);

    printf("[TRY] glXMakeCurrent\n");

    g_x_error_seen = 0;
    XSetErrorHandler(x_error_handler);

    if (!glXMakeCurrent(app->display, app->xwin.window, app->glx.context)) {
        XSync(app->display, False);
        XSetErrorHandler(NULL);

        fprintf(stderr, "[FATAL] glXMakeCurrent returned False\n");
        return 0;
    }

    XSync(app->display, False);
    XSetErrorHandler(NULL);

    if (g_x_error_seen) {
        fprintf(stderr, "[FATAL] glXMakeCurrent produced X error\n");
        return 0;
    }

    printf("[OK] glXMakeCurrent\n");

    printf("[GL] vendor=%s\n", glGetString(GL_VENDOR));
    printf("[GL] renderer=%s\n", glGetString(GL_RENDERER));
    printf("[GL] version=%s\n", glGetString(GL_VERSION));

    return 1;
}

void draw_triangle(App_p app) {
    int w = app->xwin.width;
    int h = app->xwin.height;

    if (w <= 0)
        w = 1;

    if (h <= 0)
        h = 1;

    glViewport(0, 0, w, h);

    glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_TRIANGLES);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.6f, -0.4f);

    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex2f(0.6f, -0.4f);

    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2f(0.0f, 0.6f);

    glEnd();

    if (app->glx.doublebuffer)
        glXSwapBuffers(app->display, app->xwin.window);
    else
        glFlush();
}

void handle_key(App_p app, XKeyEvent* ev) {
    KeySym ks = XLookupKeysym(ev, 0);

    switch (ks) {
        case XK_Escape:
        case XK_q:
        case XK_Q: {
            app->running = false;
        } break;

        default:    break;
    }
}

void handle_event(App_p app, XEvent* ev) {
    if (ev->type == ConfigureNotify) {
        app->xwin.width = ev->xconfigure.width;
        app->xwin.height = ev->xconfigure.height;
    }
    else if (ev->type == KeyPress) {
        handle_key(app, &ev->xkey);
    }
    else if (ev->type == ClientMessage) {
        if ((Atom)ev->xclient.data.l[0] == app->xwin.wm_delete)
            app->running = 0;
    }
    else if (ev->type == DestroyNotify) {
        app->running = 0;
    }
}

void main_loop(App_p app) {
    printf("[INFO] Context is alive. Press q/Esc or close window.\n");

    while (app->running) {
        while (XPending(app->display)) {
            XEvent ev;

            XNextEvent(app->display, &ev);
            handle_event(app, &ev);
        }

        draw_triangle(app);
        usleep(16000);
    }
}

void shutdown(App_p app) {
    if (!app)
        return;

    if (app->display && app->glx.context) {
        glXMakeCurrent(app->display, None, NULL);
        glXDestroyContext(app->display, app->glx.context);
        app->glx.context = NULL;
    }

    if (app->display && app->xwin.window) {
        XDestroyWindow(app->display, app->xwin.window);
        app->xwin.window = 0;
    }

    if (app->display && app->xwin.colormap) {
        XFreeColormap(app->display, app->xwin.colormap);
        app->xwin.colormap = 0;
    }

    if (app->glx.visual) {
        XFree(app->glx.visual);
        app->glx.visual = NULL;
    }

    if (app->display) {
        XCloseDisplay(app->display);
        app->display = NULL;
    }
}

int init(App_p app) {
    app_zero(app);

    if (!open_display(app))         return 0;
    if (!query_glx(app))            return 0;
    if (!choose_visual(app))        return 0;
    if (!create_window(app))        return 0;
    if (!wait_for_map(app))         return 0;
    if (!create_gl_context(app))    return 0;

    return 1;
}

int main(void) {
    App_t app;

    int ok = init(&app);

    if (ok)
        main_loop(&app);

    shutdown(&app);

    return ok ? 0 : 1;
}