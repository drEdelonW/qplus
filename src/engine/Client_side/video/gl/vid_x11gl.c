#include "vid.h"
#include "x_prv.h"
#include "cvar.h"
// #include "types.h"

bool gl_mtexable;
char *gl_renderer;
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
Display*    x_disp;
Window      x_win;
int         x_shmeventtype;

void D_InitCaches(void *buffer, int size) {
    (void)buffer;
    (void)size;
}

void GL_BeginRendering(int *x, int *y, int *width, int *height) {
    *x = 0;
    *y = 0;
    *width = vid.width;
    *height = vid.height;
}

void GL_EndRendering(void) {}

bool VID_Is8bit(void) {
    return false;
}