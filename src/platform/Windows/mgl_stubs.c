/* src/platform/Windows/mgl_stubs.c: Rough MGL stubs to build without the library */
void MGL_exit() {}
int  MGL_result() { return 0; }
cStringRO MGL_errorMsg(int) { return "MGL stub"; }
void MGL_fatalError(cStringRO, ...) {}

void MGL_registerDriver(void*) {}
void MGL_unregisterAllDrivers() {}
int  MGL_detectGraph() { return 0; }
void* MGL_availableModes() { return 0; }
void  MGL_modeResolution(int, int*, int*) {}

int  MGL_init() { return 0; }
void MGL_setSuspendAppCallback(void (*)(int)) {}

int  MGL_changeDisplayMode(int, int, int, int) { return 0; }
int  MGL_availablePages(void*) { return 1; }
void* MGL_createDisplayDC(int, int, int, int) { return 0; }
int  MGL_surfaceAccessType(void*) { return 0; }
int  MGL_makeCurrentDC(void*) { return 1; }
int  MGL_sizey(void*) { return 0; }
int  MGL_sizex(void*) { return 0; }
void* MGL_createMemoryDC(int, int, int, int) { return 0; }
void MGL_setActivePage(void*, int) {}
void MGL_setVisualPage(void*, int, int) {}
int  MGL_initWindowed() { return 1; }
cStringRO MGL_modeDriverName(int) { return "stub"; }
void MGL_destroyDC(void*) {}
void MGL_registerFullScreenWindow(void*) {}

int  MGL_beginDirectAccess(void*) { return 1; }
void MGL_endDirectAccess(void*) {}

void MGL_setPalette(void*, int, int, void*) {}
void MGL_realizePalette(void*) {}

void MGL_stretchBltCoord(void*, int, int, int, int, void*, int, int, int, int, int) {}
void MGL_bitBltCoord(void*, int, int, int, int, void*, int, int, int) {}
void MGL_setWinDC(void*, void*) {}
void MGL_appActivate(int) {}

/* driver symbols referenced in vid_win.c */
int PACKED8_driver = 0;
int DDRAW8_driver  = 0;
int ACCEL8_driver  = 0;
int LINEAR8_driver = 0;
int VGA8_driver    = 0;


// src/platform/Windows/mgl_stubs.c

#ifdef _WIN32
#include <windows.h>
#endif
#include <stddef.h>

typedef void* MGLDC;

void MGL_setAppInstance(
#ifdef _WIN32
    HINSTANCE hInst
#else
    void* hInst
#endif
) {
    (void)hInst;
}

MGLDC MGL_createWindowedDC(int a, int b, int c, int d, void* e, void* f, int g) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    (void)f;
    (void)g;
    return NULL;
}

void MGL_activatePalette(void* dc) { (void)dc; }
