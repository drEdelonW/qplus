/* src/platform/Windows/mgl_stubs.c: грубые заглушки MGL, чтобы собрать без библиотеки */
void MGL_exit(void) {}
int  MGL_result(void) { return 0; }
const char* MGL_errorMsg(int) { return "MGL stub"; }
void MGL_fatalError(const char*, ...) {}

void MGL_registerDriver(void*) {}
void MGL_unregisterAllDrivers(void) {}
int  MGL_detectGraph(void) { return 0; }
void* MGL_availableModes(void) { return 0; }
void  MGL_modeResolution(int, int*, int*) {}

int  MGL_init(void) { return 0; }
void MGL_setSuspendAppCallback(void (*)(int)) {}

int  MGL_changeDisplayMode(int,int,int,int) { return 0; }
int  MGL_availablePages(void*) { return 1; }
void* MGL_createDisplayDC(int,int,int,int) { return 0; }
int  MGL_surfaceAccessType(void*) { return 0; }
int  MGL_makeCurrentDC(void*) { return 1; }
int  MGL_sizey(void*) { return 0; }
int  MGL_sizex(void*) { return 0; }
void* MGL_createMemoryDC(int,int,int,int) { return 0; }
void MGL_setActivePage(void*, int) {}
void MGL_setVisualPage(void*, int, int) {}
int  MGL_initWindowed(void) { return 1; }
const char* MGL_modeDriverName(int) { return "stub"; }
void MGL_destroyDC(void*) {}
void MGL_registerFullScreenWindow(void*) {}

int  MGL_beginDirectAccess(void*) { return 1; }
void MGL_endDirectAccess(void*) {}

void MGL_setPalette(void*, int,int,void*) {}
void MGL_realizePalette(void*) {}

void MGL_stretchBltCoord(void*,int,int,int,int,void*,int,int,int,int,int) {}
void MGL_bitBltCoord(void*,int,int,int,int,void*,int,int,int) {}
void MGL_setWinDC(void*, void*) {}
void MGL_appActivate(int) {}

/* драйверные символы, к которым идёт ссылка в vid_win.c */
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
) { (void)hInst; }

MGLDC MGL_createWindowedDC(int a,int b,int c,int d, void* e, void* f, int g) {
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;
    return NULL; // код выше проверяет на NULL → работоспособно как “no-MGL”
}

void MGL_activatePalette(void* dc) { (void)dc; }
