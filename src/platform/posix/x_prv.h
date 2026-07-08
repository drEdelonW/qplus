#pragma once
#include <X11/Xlib.h>
#include "types.h"

extern bool     doShm;
extern Display* x_disp;
extern Window   x_win;
#if 0
extern bool config_notify;
extern int  config_notify_width;
extern int  config_notify_height;
#else
typedef struct {
    bool notify;
    int notify_width;
    int notify_height;
} CfgNotify_t;
extern CfgNotify_t xCfg;
#endif
extern int  x_shmeventtype;
extern bool oktodraw ;

void GetEvent();



