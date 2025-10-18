#include "menu.h"
#include "menu_prv.h"

//=============================================================================
/* MULTIPLAYER MENU */

typedef enum {
    mp_force_signed = -1,
    mp_FIRST = 0,
    mp_Join = mp_FIRST,
    mp_Create,
    mp_Setup,

    mp_NUM     //should be last
} MultiPlayer_e;
static MultiPlayer_e _mp_cursor;

bool is_CreateGame() { return _mp_cursor == mp_Create; }
bool is_JoinGame() { return _mp_cursor == mp_Join; }
bool is_anyComAval() { return serialAvailable || ipxAvailable || tcpipAvailable; }

void M_Menu_MultiPlayer_f() {
    key_dest = key_menu;
    m_state = m_multiplayer;
    m_entersound = true;
}


void M_MultiPlayer_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    qPic_p p = Draw_CachePic("gfx/p_multi.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);
    M_DrawTransPic(72, 32, Draw_CachePic("gfx/mp_menu.lmp"));

    M_DrawTransPic(54, 32 + _mp_cursor * 20,
        Draw_CachePic(va("gfx/menudot%i.lmp", curAmimFrame()))
    );

    if (!(is_anyComAval()))
        M_PrintWhite((320 / 2) - ((27 * 8) / 2), 148,
            "No Communications Available"
        );
}


void M_MultiPlayer_Key(keycode_t key) {
    switch (key) {
    case K_ESCAPE:  M_Menu_Main_f();    break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        if (++_mp_cursor >= mp_NUM) _mp_cursor = mp_FIRST;
        break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        if (--_mp_cursor < mp_FIRST) _mp_cursor = mp_NUM - 1;
        break;

    case K_ENTER:
        m_entersound = true;
        switch (_mp_cursor) {
        case mp_Join:
        case mp_Create:
            if (is_anyComAval()) {M_Menu_Net_f();} break;

        case mp_Setup: M_Menu_Setup_f(); break;
        default: break;
        }
    default: break;
    }
}
