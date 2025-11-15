#include "input.h"
#include "vid.h"
#include "x_prv.h"
#include "enginedefs.h"
#include "cvar_q1.h"
#include "common.h"
#include "client.h"
#include "angle.h"
#include "q_tools.h"
#include "host.h"

#include <X11/keysym.h>
#include <X11/Xutil.h>


static float   old_windowed_mouse;
static bool    mouse_avail;
static int     mouse_buttonstate;
static float   mouse_x, mouse_y;
static int     mouse_buttons = 3;
static int     mouse_oldbuttonstate;
static float   old_mouse_x, old_mouse_y;
static int     p_mouse_x;
static int     p_mouse_y;
struct {
    keycode_t   key;
    bool        down;
} keyq[64];

int keyq_head = 0;
int keyq_tail = 0;

void Sys_SendKeyEvents() {
    // get events from x server
    if (x_disp) {
        while (XPending(x_disp)) GetEvent();
        while (keyq_head != keyq_tail) {
            Key_Event(keyq[keyq_tail].key, keyq[keyq_tail].down);
            keyq_tail = (keyq_tail + 1) & 63;
        }
    }
}

#if 0
cString Sys_ConsoleInput() {

    static char text[256];
    int  len;
    fd_set  readfds;
    int  ready;
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    ready = select(1, &readfds, 0, 0, &timeout);

    if (ready > 0) {
        len = read(0, text, sizeof(text));
        if (len >= 1) {
            text[len - 1] = 0; // rip off the /n and terminate
            return text;
        }
    }

    return 0;

}
#endif

keycode_t XLateKey(XKeyEvent* ev) {
    keycode_t key;
    char buf[NAME_LENGTH];
    KeySym keysym;

    key = 0;

    XLookupString(ev, buf, sizeof buf, &keysym, 0);

    switch (keysym) {
    case XK_KP_Page_Up:
    case XK_Page_Up:        key = K_PGUP;       break;

    case XK_KP_Page_Down:
    case XK_Page_Down:      key = K_PGDN;       break;

    case XK_KP_Home:
    case XK_Home:           key = K_HOME;       break;

    case XK_KP_End:
    case XK_End:            key = K_END;        break;

    case XK_KP_Left:
    case XK_Left:           key = K_LEFTARROW;  break;

    case XK_KP_Right:
    case XK_Right:          key = K_RIGHTARROW; break;

    case XK_KP_Down:
    case XK_Down:           key = K_DOWNARROW;  break;

    case XK_KP_Up:
    case XK_Up:             key = K_UPARROW;    break;

    case XK_Escape:         key = K_ESCAPE;     break;

    case XK_KP_Enter:
    case XK_Return:         key = K_ENTER;      break;

    case XK_Tab:            key = K_TAB;        break;
    case XK_F1:             key = K_F1;         break;
    case XK_F2:             key = K_F2;         break;
    case XK_F3:             key = K_F3;         break;
    case XK_F4:             key = K_F4;         break;
    case XK_F5:             key = K_F5;         break;
    case XK_F6:             key = K_F6;         break;
    case XK_F7:             key = K_F7;         break;
    case XK_F8:             key = K_F8;         break;
    case XK_F9:             key = K_F9;         break;
    case XK_F10:            key = K_F10;        break;
    case XK_F11:            key = K_F11;        break;
    case XK_F12:            key = K_F12;        break;
    case XK_BackSpace:      key = K_BACKSPACE;  break;

    case XK_KP_Delete:
    case XK_Delete:         key = K_DEL;        break;

    case XK_Pause:          key = K_PAUSE;      break;

    case XK_Shift_L:
    case XK_Shift_R:        key = K_SHIFT;      break;

    case XK_Execute:
    case XK_Control_L:
    case XK_Control_R:      key = K_CTRL;       break;

    case XK_Alt_L:
    case XK_Meta_L:
    case XK_Alt_R:
    case XK_Meta_R:         key = K_ALT;        break;

    case XK_KP_Begin:       key = K_AUX30;      break;

    case XK_Insert:
    case XK_KP_Insert:      key = K_INS;        break;

    case XK_KP_Multiply:    key = '*';          break;
    case XK_KP_Add:         key = '+';          break;
    case XK_KP_Subtract:    key = '-';          break;
    case XK_KP_Divide:      key = '/';          break;

#if 0
    case 0x021: key = '1';break;/* [!] */
    case 0x040: key = '2';break;/* [@] */
    case 0x023: key = '3';break;/* [#] */
    case 0x024: key = '4';break;/* [$] */
    case 0x025: key = '5';break;/* [%] */
    case 0x05e: key = '6';break;/* [^] */
    case 0x026: key = '7';break;/* [&] */
    case 0x02a: key = '8';break;/* [*] */
    case 0x028: key = '9';;break;/* [(] */
    case 0x029: key = '0';break;/* [)] */
    case 0x05f: key = '-';break;/* [_] */
    case 0x02b: key = '=';break;/* [+] */
    case 0x07c: key = '\'';break;/* [|] */
    case 0x07d: key = '[';break;/* [}] */
    case 0x07b: key = ']';break;/* [{] */
    case 0x022: key = '\'';break;/* ["] */
    case 0x03a: key = ';';break;/* [:] */
    case 0x03f: key = '/';break;/* [?] */
    case 0x03e: key = '.';break;/* [>] */
    case 0x03c: key = ',';break;/* [<] */
#endif

    default:
        key = *(uint8_p)buf;
        if ((key >= 'A') &&
            (key <= 'Z'))
            key = key - 'A' + 'a';
        //   fprintf(stdout, "case 0x0%x: key = ___;break;/* [%c] */\n", keysym);
        break;
    }

    return key;
}

void GetEvent() {
    XEvent x_event;    XNextEvent(x_disp, &x_event);
    switch (x_event.type) {
    case KeyPress:
        keyq[keyq_head].key = XLateKey(&x_event.xkey);
        keyq[keyq_head].down = true;
        keyq_head = (keyq_head + 1) & 63;
        break;
    case KeyRelease:
        keyq[keyq_head].key = XLateKey(&x_event.xkey);
        keyq[keyq_head].down = false;
        keyq_head = (keyq_head + 1) & 63;
        break;

    case MotionNotify:
        if (_windowed_mouse.value) {
            mouse_x = (float)((int)x_event.xmotion.x - (int)(vid.width / 2));
            mouse_y = (float)((int)x_event.xmotion.y - (int)(vid.height / 2));
            //printf("m: x=%d,y=%d, mx=%3.2f,my=%3.2f\n",
            // x_event.xmotion.x, x_event.xmotion.y, mouse_x, mouse_y);

                        /* move the mouse to the window center again */
            XSelectInput(x_disp, x_win, StructureNotifyMask | KeyPressMask
                | KeyReleaseMask | ExposureMask
                | ButtonPressMask
                | ButtonReleaseMask);
            XWarpPointer(x_disp, None, x_win, 0, 0, 0, 0,
                (vid.width / 2), (vid.height / 2));
            XSelectInput(x_disp, x_win, StructureNotifyMask | KeyPressMask
                | KeyReleaseMask | ExposureMask
                | PointerMotionMask | ButtonPressMask
                | ButtonReleaseMask);
        }
        else {
            mouse_x = (float)(x_event.xmotion.x - p_mouse_x);
            mouse_y = (float)(x_event.xmotion.y - p_mouse_y);
            p_mouse_x = x_event.xmotion.x;
            p_mouse_y = x_event.xmotion.y;
        }
        break;

    case ButtonPress: {
        int b = -1;
        if (x_event.xbutton.button == 1)        b = 0;
        else if (x_event.xbutton.button == 2)   b = 2;
        else if (x_event.xbutton.button == 3)   b = 1;
        if (b >= 0)         mouse_buttonstate |= 1 << b;
    }break;

    case ButtonRelease: {
        int b = -1;
        if (x_event.xbutton.button == 1)        b = 0;
        else if (x_event.xbutton.button == 2)   b = 2;
        else if (x_event.xbutton.button == 3)   b = 1;
        if (b >= 0)         mouse_buttonstate &= ~(1 << b);
    } break;

    case ConfigureNotify:
        //printf("config notify\n");
        config_notify_width = x_event.xconfigure.width;
        config_notify_height = x_event.xconfigure.height;
        config_notify = 1;
        break;

    default:
        if (doShm && x_event.type == x_shmeventtype)
            oktodraw = true;
    }

    if (old_windowed_mouse != _windowed_mouse.value) {
        old_windowed_mouse = _windowed_mouse.value;

        if (!_windowed_mouse.value) {
            /* ungrab the pointer */
            XUngrabPointer(x_disp, CurrentTime);
        }
        else {
            /* grab the pointer */
            XGrabPointer(x_disp, x_win, True, 0, GrabModeAsync,
                GrabModeAsync, x_win, None, CurrentTime);
        }
    }
}


void IN_Init() {
    Cvar_RegisterVariable(&_windowed_mouse);
    Cvar_RegisterVariable(&m_filter);
    if (COM_CheckParm("-nomouse"))
        return;
    mouse_x = mouse_y = 0.0;
    mouse_avail = 1;
}

void IN_Shutdown() {
    mouse_avail = 0;
}

void IN_Commands() {
    if (!mouse_avail) return;

    for (int i = 0; i < mouse_buttons; i++) {
        if ((mouse_buttonstate & (1 << i)) &&
            !(mouse_oldbuttonstate & (1 << i))
            )
            Key_Event(K_MOUSE1 + i, true);

        if (!(mouse_buttonstate & (1 << i)) &&
            (mouse_oldbuttonstate & (1 << i))
            )
            Key_Event(K_MOUSE1 + i, false);
    }
    mouse_oldbuttonstate = mouse_buttonstate;
}

void IN_Move(UserCmd_p cmd) {
    if (!mouse_avail)
        return;

    if (m_filter.value) {
        mouse_x = (mouse_x + old_mouse_x) * 0.5;
        mouse_y = (mouse_y + old_mouse_y) * 0.5;
    }

    old_mouse_x = mouse_x;
    old_mouse_y = mouse_y;

    mouse_x *= sensitivity.value;
    mouse_y *= sensitivity.value;

    if ((in.strafe.state & 1) || (lookstrafe.value && (in.mlook.state & 1)))
        cmd->sidemove += m_side.value * mouse_x;
    else
        cl.viewangles[YAW] -= m_yaw.value * mouse_x;    // mouseLook
    if (in.mlook.state & 1)
        V_StopPitchDrift();

    if ((in.mlook.state & 1) && !(in.strafe.state & 1)) {  // mouseLook
        cl.viewangles[PITCH] += m_pitch.value * mouse_y;
        CLAMP_MAX(cl.viewangles[PITCH], 80);    // down look
        // if (cl.viewangles[PITCH] > 80)
        //  cl.viewangles[PITCH] = 80;
        CLAMP_MIN(cl.viewangles[PITCH], -70);  // up look
        // if (cl.viewangles[PITCH] < -70)
        //  cl.viewangles[PITCH] = -70;
    }
    else {
        if ((in.strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * mouse_y;
        else
            cmd->forwardmove -= m_forward.value * mouse_y;
    }
    mouse_x = mouse_y = 0.0;
}
