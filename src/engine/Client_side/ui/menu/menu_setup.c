#include "menu.h"
#include "menu_prv.h"
#include "client.h"
#include "cvar_q1.h"
#include "q_tools.h"
#include <string.h>
#include "cbuf.h"


//=============================================================================
/* SETUP MENU */
typedef enum {
    s_force_signed = -1,
    s_FIRST = 0,
    s_HostName = s_FIRST,
    s_PlayerName,
    s_ShirtCol,
    s_PantsCol,
    s_Accept,

    s_NUM     //should be last
} Setup_e;
static Setup_e _cursor = 4;
#define NUM_SETUP_CMDS 5

static int _y[5] = { 40, 56, 80, 104, 140 };

#define HOSTNAME_LEN    (16)
#define PLAYERNAME_LEN    (16)
typedef struct {
    char hostname[HOSTNAME_LEN];
    char myname[PLAYERNAME_LEN];
    int  oldtop;
    int  oldbottom;
    int  top;
    int  bottom;
} Setup_t;
Setup_t _s;

void M_Menu_Setup_f() {
    key.dest = key_menu;
    m_state = m_setup;
    m_entersound = true;
    Q_strcpy(_s.myname, cl_name.string);
    Q_strcpy(_s.hostname, hostname.string);
    _s.top = _s.oldtop = ((int)cl_color.value) >> 4;
    _s.bottom = _s.oldbottom = ((int)cl_color.value) & 15;
}


void M_Setup_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));

    M_DrawPicHC(4, Draw_CachePic("gfx/p_multi.lmp"));

    M_Print(64, _y[s_HostName], "Hostname");
    M_DrawTextBox(160, _y[s_HostName] - 8, HOSTNAME_LEN, 1); {
        M_Print(168, _y[s_HostName], _s.hostname);
    }

    M_Print(64, _y[s_PlayerName], "Your name");
    M_DrawTextBox(160, _y[s_PlayerName] - 8, PLAYERNAME_LEN, 1); {
        M_Print(168, _y[s_PlayerName], _s.myname);
    }
    M_Print(64, _y[s_ShirtCol], "Shirt color");

    M_Print(64, _y[s_PantsCol], "Pants color");

    M_DrawTextBox(64, _y[s_Accept] - 8, 14, 1); {
        M_Print(72, _y[s_Accept], "Accept Changes");
    }

    M_DrawTransPic(160, 64, Draw_CachePic("gfx/bigbox.lmp"));
    M_BuildTranslationTable(_s.top * 16, _s.bottom * 16); {
        M_DrawTransPicTranslate(172, 72, Draw_CachePic("gfx/menuplyr.lmp"));
    }

    M_DrawCharacter(56, _y[_cursor], curSymb());

    if (_cursor == s_HostName)
        M_DrawCharacter(168 + 8 * strlen(_s.hostname), _y[_cursor], inpSymb());

    if (_cursor == s_PlayerName)
        M_DrawCharacter(168 + 8 * strlen(_s.myname), _y[_cursor], inpSymb());
}


void M_Setup_Key(keycode_t k) {
    switch (k) {
    case K_ESCAPE:  M_Menu_MultiPlayer_f(); break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        if (--_cursor < s_FIRST) { _cursor = s_NUM - 1; } break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        if (++_cursor >= s_NUM) { _cursor = s_FIRST; } break;

    case K_LEFTARROW:
        if (_cursor < s_ShirtCol)   return;
        S_LocalSound("misc/menu3.wav");
        if (_cursor == s_ShirtCol)  _s.top--;
        if (_cursor == s_PantsCol)  _s.bottom--;
        break;

    case K_RIGHTARROW:
        if (_cursor < s_ShirtCol)   return;
    forward:
        S_LocalSound("misc/menu3.wav");
        if (_cursor == s_ShirtCol)  _s.top++;
        if (_cursor == s_PantsCol)  _s.bottom++;
        break;

    case K_ENTER:
        if ((_cursor == s_HostName) ||
            (_cursor == s_PlayerName))
            return;

        if ((_cursor == s_ShirtCol) ||
            (_cursor == s_PantsCol))
            goto forward;

        // _cursor == 4 (OK)
        if (Q_strcmp(cl_name.string, _s.myname) != 0) { Cbuf_AddText(va("name \"%s\"\n", _s.myname)); }
        if (Q_strcmp(hostname.string, _s.hostname) != 0) { Cvar_Set("hostname", _s.hostname); }
        if ((_s.top != _s.oldtop) ||
            (_s.bottom != _s.oldbottom))
            Cbuf_AddText(va("color %i %i\n", _s.top, _s.bottom));
        m_entersound = true;
        M_Menu_MultiPlayer_f();
        break;

    case K_BACKSPACE:
        if ((_cursor == s_HostName) &&
            (strlen(_s.hostname)))
            _s.hostname[strlen(_s.hostname) - 1] = 0x00;

        if ((_cursor == s_PlayerName) &&
            (strlen(_s.myname)))
            _s.myname[strlen(_s.myname) - 1] = 0x00;
        break;

    default:
        if (!is_printable(k)) break;
        if (_cursor == s_HostName) {
            int l = strlen(_s.hostname);
            if (l < (HOSTNAME_LEN - 1)) {
                _s.hostname[l] = k;
                _s.hostname[l + 1] = 0x00;
            }
        }
        if (_cursor == s_PlayerName) {
            int l = strlen(_s.myname);
            if (l < (PLAYERNAME_LEN - 1)) {
                _s.myname[l] = k;
                _s.myname[l + 1] = 0x00;
            }
        }
    }

    if (_s.top > 13)
        _s.top = 0;
    else if (_s.top < 0)
        _s.top = 13;

    if (_s.bottom > 13)
        _s.bottom = 0;
    else if (_s.bottom < 0)
        _s.bottom = 13;
}
