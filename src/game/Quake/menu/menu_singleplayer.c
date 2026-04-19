#include "menu.h"
#include "menu_prv.h"
#include "host.h"
#include "screen.h"
#include "cbuf.h"

//=============================================================================
/* SINGLE PLAYER MENU */

typedef enum {
    sp_force_signed = -1,
    sp_FIRST = 0,
    sp_NewGame = sp_FIRST,
    sp_Load,
    sp_Save,

    sp_NUM     //should be last
} SinglePlayer_e;
static SinglePlayer_e _cursor;

void M_Menu_SinglePlayer_f() {
    key.dest = key_menu;
    m_state = m_singleplayer;
    m_entersound = true;
}


void M_SinglePlayer_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    M_DrawPicHC(4, Draw_CachePic("gfx/ttl_sgl.lmp"));
    M_DrawTransPic(72, 32, Draw_CachePic("gfx/sp_menu.lmp"));
    M_DrawTransPic(54, 32 + _cursor * 20, Draw_CachePic(va("gfx/menudot%i.lmp", curAnimFrame())));
}

void M_SinglePlayer_Key(keycode_t Key) {
    switch (Key) {
    case K_ESCAPE:  M_Menu_Main_f();    break;

    case K_DOWNARROW: {
        S_LocalSound("misc/menu1.wav");
        if (++_cursor >= sp_NUM)    _cursor = sp_FIRST;
    } break;

    case K_UPARROW: {
        S_LocalSound("misc/menu1.wav");
        if (--_cursor < sp_FIRST)   _cursor = sp_NUM - 1;
    } break;

    case K_ENTER: {
        m_entersound = true;

        switch (_cursor) {
        case sp_NewGame: {
            if ((Host_IsServerActive()) &&
                (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n")))
                break;
            key.dest = key_game;
            if (Host_IsServerActive())
                Cbuf_AddText("disconnect\n");
            Cbuf_AddText("maxplayers 1\n");
            Cbuf_AddText("map start\n");
        } break;

        case sp_Load:   M_Menu_Load_f();    break;
        case sp_Save:   M_Menu_Save_f();    break;
        default: break;
        }
    } break;
    default: break;
    }
}
