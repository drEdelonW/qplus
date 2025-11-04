#include "menu.h"
#include "menu_prv.h"
#include "client.h"
//=============================================================================
/* MAIN MENU */

typedef enum {
    m_force_signed = -1,
    m_FIRST = 0,
    m_SinglePlayer = m_FIRST,
    m_MultiPlayer,
    m_Options,
    m_Help,
    m_Quit,

    m_LAST     //should be last
} Main_e;
static Main_e _cursor;
#define MAIN_ITEMS 5
int m_save_demonum;

void M_Menu_Main_f() {
    if (key.dest != key_menu) {
        m_save_demonum = cls.demonum;
        cls.demonum = -1;
    }
    key.dest = key_menu;
    m_state = m_main;
    m_entersound = true;
}

void M_Main_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));

    M_DrawPicHC(4, Draw_CachePic("gfx/ttl_main.lmp"));

    M_DrawTransPic(72, 32, Draw_CachePic("gfx/mainmenu.lmp"));

    M_DrawTransPic(54, 32 + _cursor * 20, Draw_CachePic(va("gfx/menudot%i.lmp", curAnimFrame())));
}

void M_Main_Key(keycode_t Key) {
    switch (Key) {
    case K_ESCAPE:
        key.dest = key_game;
        m_state = m_none;
        cls.demonum = m_save_demonum;
        if ((cls.demonum != -1) &&
            (!cls.demoplayback) &&
            (cls.state != ca_connected))
            CL_NextDemo();
        break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        if (++_cursor >= m_LAST)    _cursor = m_FIRST;
        break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        if (--_cursor < (int)m_FIRST)    _cursor = m_LAST - 1;
        break;

    case K_ENTER:
        m_entersound = true;

        switch (_cursor) {
        case m_SinglePlayer:    M_Menu_SinglePlayer_f();    break;
        case m_MultiPlayer:     M_Menu_MultiPlayer_f();     break;
        case m_Options:         M_Menu_Options_f();         break;
        case m_Help:            M_Menu_Help_f();            break;
        case m_Quit:            M_Menu_Quit_f();            break;
        default: break;
        }
    default: break;
    }
}
