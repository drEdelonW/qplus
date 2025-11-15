#pragma once
#include <X11/Xlib.h>

extern bool     doShm;
extern Display* x_disp;
extern Window   x_win;
extern int  config_notify;
extern int  config_notify_width;
extern int  config_notify_height;
extern int  x_shmeventtype;
extern bool oktodraw ;

void GetEvent();



