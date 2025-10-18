#include "menu.h"
#include "menu_prv.h"
//=============================================================================
/* SINGLE PLAYER MENU */

int m_singleplayer_cursor;
#define SINGLEPLAYER_ITEMS 3


void M_Menu_SinglePlayer_f() {
    key_dest = key_menu;
    m_state = m_singleplayer;
    m_entersound = true;
}


void M_SinglePlayer_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));

    qPic_p p = Draw_CachePic("gfx/ttl_sgl.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_DrawTransPic(72, 32, Draw_CachePic("gfx/sp_menu.lmp"));

    M_DrawTransPic(54, 32 + m_singleplayer_cursor * 20, Draw_CachePic(va("gfx/menudot%i.lmp", curAmimFrame())));
}

void M_SinglePlayer_Key(keycode_t key) {
    switch (key) {
    case K_ESCAPE:
        M_Menu_Main_f();
        break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
            m_singleplayer_cursor = 0;
        break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        if (--m_singleplayer_cursor < 0)
            m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
        break;

    case K_ENTER:
        m_entersound = true;

        switch (m_singleplayer_cursor) {
        case 0:
            if (sv.active)
                if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
                    break;
            key_dest = key_game;
            if (sv.active)
                Cbuf_AddText("disconnect\n");
            Cbuf_AddText("maxplayers 1\n");
            Cbuf_AddText("map start\n");
            break;

        case 1:
            M_Menu_Load_f();
            break;

        case 2:
            M_Menu_Save_f();
            break;
        }
    default: break;
    }
}
