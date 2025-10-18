#include "menu.h"
#include "menu_prv.h"


//=============================================================================
/* MODEM CONFIG MENU */

typedef enum{
    mc_force_signed = -1,
    mc_FIRST = 0,
    mc_DialMode = mc_FIRST,
    mc_Clear,
    mc_Init,
    mc_Hangup,
    mc_OK,

    mc_LAST     //should be last
} modemConfig_e;
int piontY[mc_LAST] = { 40, 56, 88, 120, 156 };

#define CLEAR_LEN   (16)
#define INIT_LEN    (30)
#define HANGUP_LEN  (16)

typedef struct {
    modemConfig_e  cursor;
    char dialing;
    char sClear[CLEAR_LEN];
    char sInit[INIT_LEN];
    char sHangup[HANGUP_LEN];
} modemConfig_t;
static modemConfig_t _mc;

void M_Menu_ModemConfig_f() {
    key_dest = key_menu;
    m_state = m_modemconfig;
    m_entersound = true;
    (*GetModemConfig) (0, &_mc.dialing, _mc.sClear, _mc.sInit, _mc.sHangup);
}


void M_ModemConfig_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));

    qPic_p p = Draw_CachePic("gfx/p_multi.lmp");
    int x = (320 - p->width) / 2;
    M_DrawPic(x, 4, p);

    x += 8;
    M_Print(x, piontY[mc_DialMode], (_mc.dialing == 'P') ? "Pulse Dialing" : "Touch Tone Dialing");

    M_Print(x, piontY[mc_Clear], "Clear");
    M_DrawTextBox(x, piontY[mc_Clear] + 4, CLEAR_LEN, 1); {
        M_Print(x + 8, piontY[mc_Clear] + 12, _mc.sClear);
    }
    if (_mc.cursor == mc_Clear)
        M_DrawCharacter(x + 8 + 8 * strlen(_mc.sClear), piontY[mc_Clear] + 12, inpSymb());


    M_Print(x, piontY[mc_Init], "Init");
    M_DrawTextBox(x, piontY[mc_Init] + 4, INIT_LEN, 1); {
        M_Print(x + 8, piontY[mc_Init] + 12, _mc.sInit);
    }
    if (_mc.cursor == mc_Init)
        M_DrawCharacter(x + 8 + 8 * strlen(_mc.sInit), piontY[mc_Init] + 12, inpSymb());


    M_Print(x, piontY[mc_Hangup], "Hangup");
    M_DrawTextBox(x, piontY[mc_Hangup] + 4, HANGUP_LEN, 1); {
        M_Print(x + 8, piontY[mc_Hangup] + 12, _mc.sHangup);
    }
    if (_mc.cursor == mc_Hangup)
        M_DrawCharacter(x + 8 + 8 * strlen(_mc.sHangup), piontY[mc_Hangup] + 12, inpSymb());


    M_DrawTextBox(x, piontY[mc_OK] - 8, 2, 1); {
        M_Print(x + 8, piontY[mc_OK], "OK");
    }

    M_DrawCharacter(x - 8, piontY[_mc.cursor], curSymb());
}


void M_ModemConfig_Key(keycode_t key) {
    switch (key) {
    case K_ESCAPE:  M_Menu_SerialConfig_f();    break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        _mc.cursor--;
        if (_mc.cursor < mc_FIRST)
            _mc.cursor = mc_LAST - 1;
        break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        _mc.cursor++;
        if (_mc.cursor >= mc_LAST)
            _mc.cursor = mc_FIRST;
        break;

    case K_LEFTARROW:
    case K_RIGHTARROW:
        if (_mc.cursor == mc_DialMode) {
            _mc.dialing = (_mc.dialing == 'P') ? 'T' : 'P';
            S_LocalSound("misc/menu1.wav");
        }
        break;

    case K_ENTER:
        if (_mc.cursor == mc_DialMode) {
            _mc.dialing = (_mc.dialing == 'P') ? 'T' : 'P';
            m_entersound = true;
        }

        if (_mc.cursor == mc_OK) {
            (*SetModemConfig) (0, va("%c", _mc.dialing), _mc.sClear, _mc.sInit, _mc.sHangup);
            m_entersound = true;
            M_Menu_SerialConfig_f();
        }
        break;

    case K_BACKSPACE:
        if (_mc.cursor == mc_Clear) {
            if (strlen(_mc.sClear))  _mc.sClear[strlen(_mc.sClear) - 1] = 0x00;
        }

        if (_mc.cursor == mc_Init) {
            if (strlen(_mc.sInit))   _mc.sInit[strlen(_mc.sInit) - 1] = 0x00;
        }

        if (_mc.cursor == mc_Hangup) {
            if (strlen(_mc.sHangup)) _mc.sHangup[strlen(_mc.sHangup) - 1] = 0x00;
        }
        break;

    default:
        if ((key < 32) ||
            (key > 127))
            break; // is_printable

        if (_mc.cursor == mc_Clear) {
            int l = strlen(_mc.sClear);
            if (l < CLEAR_LEN - 1) {
                _mc.sClear[l] = key; _mc.sClear[l + 1] = 0x00;
            }
        }

        if (_mc.cursor == mc_Init) {
            int l = strlen(_mc.sInit);
            if (l < INIT_LEN - 1) {
                _mc.sInit[l] = key;  _mc.sInit[l + 1] = 0x00;
            }
        }

        if (_mc.cursor == mc_Hangup) {
            int l = strlen(_mc.sHangup);
            if (l < HANGUP_LEN - 1) {
                _mc.sHangup[l] = key;    _mc.sHangup[l + 1] = 0x00;
            }
        }
    }
}
