#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <GL/gl.h>
#include <GL/glx.h>

static int x_error_seen = 0;

static int x_error_handler(Display *dpy, XErrorEvent *ev) {
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

    x_error_seen = 1;
    return 0;
}

static int wait_for_map(Display *dpy, Window win) {
    for (;;) {
        XEvent ev;
        XNextEvent(dpy, &ev);

        if (ev.type == MapNotify && ev.xmap.window == win)
            return 1;

        if (ev.type == DestroyNotify)
            return 0;
    }
}

static XVisualInfo *choose_glx_visual(Display *dpy, int screen, int *doublebuffer) {
    struct {
        const char *name;
        int attrs[16];
        int db;
    } tries[] = {
        {
            "RGBA + doublebuffer",
            { GLX_RGBA, GLX_DOUBLEBUFFER, None },
            1
        },
        {
            "RGBA + RGB 8:8:8 + doublebuffer",
            { GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DOUBLEBUFFER, None },
            1
        },
        {
            "RGBA only",
            { GLX_RGBA, None },
            0
        },
        {
            "RGBA + RGB 8:8:8",
            { GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None },
            0
        }
    };

    int n = (int)(sizeof(tries) / sizeof(tries[0]));

    for (int i = 0; i < n; i++) {
        printf("[TRY] glXChooseVisual: %s\n", tries[i].name);

        XVisualInfo *vi = glXChooseVisual(dpy, screen, tries[i].attrs);
        if (vi) {
            int value = 0;

            *doublebuffer = tries[i].db;

            printf("[OK] visual: %s\n", tries[i].name);
            printf("[VISUAL] visualid=0x%lx depth=%d class=%d\n",
                   vi->visualid, vi->depth, vi->class);

            if (glXGetConfig(dpy, vi, GLX_DOUBLEBUFFER, &value) == 0)
                printf("[VISUAL] GLX_DOUBLEBUFFER=%d\n", value);

            if (glXGetConfig(dpy, vi, GLX_DEPTH_SIZE, &value) == 0)
                printf("[VISUAL] GLX_DEPTH_SIZE=%d\n", value);

            return vi;
        }

        printf("[FAIL] %s\n", tries[i].name);
    }

    return NULL;
}

int main(void) {
    printf("[START] X11 + GLX context test\n");

    printf("[ENV] DISPLAY=%s\n", getenv("DISPLAY") ? getenv("DISPLAY") : "(null)");
    printf("[ENV] LIBGL_ALWAYS_INDIRECT=%s\n",
           getenv("LIBGL_ALWAYS_INDIRECT") ? getenv("LIBGL_ALWAYS_INDIRECT") : "(null)");

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "[FATAL] XOpenDisplay failed\n");
        return 1;
    }

    int screen = DefaultScreen(dpy);

    printf("[OK] XOpenDisplay\n");
    printf("[X11] vendor=%s\n", ServerVendor(dpy));

    int err_base = 0;
    int ev_base = 0;

    if (!glXQueryExtension(dpy, &err_base, &ev_base)) {
        fprintf(stderr, "[FATAL] GLX extension not available\n");
        XCloseDisplay(dpy);
        return 1;
    }

    printf("[OK] glXQueryExtension err_base=%d ev_base=%d\n", err_base, ev_base);

    int glx_major = 1;
    int glx_minor = 0;

    if (!glXQueryVersion(dpy, &glx_major, &glx_minor)) {
        fprintf(stderr, "[FATAL] glXQueryVersion failed\n");
        XCloseDisplay(dpy);
        return 1;
    }

    printf("[GLX] version=%d.%d\n", glx_major, glx_minor);

    int doublebuffer = 0;
    XVisualInfo *vi = choose_glx_visual(dpy, screen, &doublebuffer);

    if (!vi) {
        fprintf(stderr, "[FATAL] no GLX visual\n");
        XCloseDisplay(dpy);
        return 1;
    }

    Colormap cmap = XCreateColormap(
        dpy,
        RootWindow(dpy, vi->screen),
        vi->visual,
        AllocNone
    );

    XSetWindowAttributes swa;
    memset(&swa, 0, sizeof(swa));

    swa.colormap = cmap;
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

    Window win = XCreateWindow(
        dpy,
        RootWindow(dpy, vi->screen),
        100, 100,
        640, 480,
        0,
        vi->depth,
        InputOutput,
        vi->visual,
        CWBorderPixel | CWColormap | CWEventMask,
        &swa
    );

    if (!win) {
        fprintf(stderr, "[FATAL] XCreateWindow failed\n");
        XFreeColormap(dpy, cmap);
        XFree(vi);
        XCloseDisplay(dpy);
        return 1;
    }

    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wm_delete, 1);

    XStoreName(dpy, win, "X11 GLX context test");
    XMapWindow(dpy, win);
    XFlush(dpy);

    printf("[OK] XCreateWindow win=0x%lx\n", win);
    printf("[OK] XMapWindow\n");

    if (!wait_for_map(dpy, win)) {
        fprintf(stderr, "[FATAL] MapNotify failed\n");
        XDestroyWindow(dpy, win);
        XFreeColormap(dpy, cmap);
        XFree(vi);
        XCloseDisplay(dpy);
        return 1;
    }

    printf("[OK] MapNotify\n");

    printf("[TRY] glXCreateContext direct=False\n");

    GLXContext ctx = glXCreateContext(dpy, vi, NULL, False);
    if (!ctx) {
        fprintf(stderr, "[FATAL] glXCreateContext failed\n");
        XDestroyWindow(dpy, win);
        XFreeColormap(dpy, cmap);
        XFree(vi);
        XCloseDisplay(dpy);
        return 1;
    }

    printf("[OK] glXCreateContext ctx=%p\n", (void *)ctx);
    printf("[GLX] glXIsDirect=%d\n", glXIsDirect(dpy, ctx));

    printf("[TRY] glXMakeCurrent\n");

    x_error_seen = 0;
    XSetErrorHandler(x_error_handler);

    Bool ok = glXMakeCurrent(dpy, win, ctx);

    XSync(dpy, False);
    XSetErrorHandler(NULL);

    if (x_error_seen || !ok) {
        fprintf(stderr, "[FAIL] glXMakeCurrent failed, x_error_seen=%d return=%d\n",
                x_error_seen, ok);

        glXDestroyContext(dpy, ctx);
        XDestroyWindow(dpy, win);
        XFreeColormap(dpy, cmap);
        XFree(vi);
        XCloseDisplay(dpy);

        return 1;
    }

    printf("[OK] glXMakeCurrent\n");

    printf("[GL] vendor=%s\n", glGetString(GL_VENDOR));
    printf("[GL] renderer=%s\n", glGetString(GL_RENDERER));
    printf("[GL] version=%s\n", glGetString(GL_VERSION));

    printf("[INFO] Context is alive. Press q/Esc or close window.\n");

    int running = 1;

    while (running) {
        XEvent ev;
        XNextEvent(dpy, &ev);

        if (ev.type == Expose) {
            glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            if (doublebuffer)
                glXSwapBuffers(dpy, win);
            else
                glFlush();
        } else if (ev.type == KeyPress) {
            KeySym ks = XLookupKeysym(&ev.xkey, 0);
            if (ks == XK_Escape || ks == XK_q || ks == XK_Q)
                running = 0;
        } else if (ev.type == ClientMessage) {
            if ((Atom)ev.xclient.data.l[0] == wm_delete)
                running = 0;
        }
    }

    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, ctx);

    XDestroyWindow(dpy, win);
    XFreeColormap(dpy, cmap);
    XFree(vi);
    XCloseDisplay(dpy);

    return 0;
}
