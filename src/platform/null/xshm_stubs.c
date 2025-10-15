#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "types.h"

// Заглушки для MIT-SHM под macOS/STM32

Bool XShmQueryExtension(Display* dpy) {
    return False;
}

int XShmGetEventBase(Display* dpy) {
    return 0;
}

Bool XShmAttach(Display* dpy, XShmSegmentInfo* shminfo) {
    return False;
}

Bool XShmDetach(Display* dpy, XShmSegmentInfo* shminfo) {
    return False;
}

XImage* XShmCreateImage(Display* dpy, Visual* visual, uint32_t depth,
    int format, cString data, XShmSegmentInfo* shminfo,
    uint32_t width, uint32_t height) {
    return NULL;
}

Bool XShmPutImage(Display* dpy, Drawable d, GC gc, XImage* image,
    int src_x, int src_y, int dst_x, int dst_y,
    uint32_t width, uint32_t height, Bool send_event) {
    return False;
}