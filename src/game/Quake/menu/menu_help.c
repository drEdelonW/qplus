#include "menu.h"
#include "menu_prv.h"


//=============================================================================
/* HELP MENU */

int help_page;
#define NUM_HELP_PAGES 6


void M_Menu_Help_f() {
    key.dest = key_menu;
    m_state = m_help;
    m_entersound = true;
    help_page = 0;
}



void M_Help_Draw() {
    M_DrawPic(0, 0,
        Draw_CachePic(
            va("gfx/help%i.lmp", help_page)
        )
    );
}


void M_Help_Key(keycode_t key) {
    switch (key) {
    case K_ESCAPE:  M_Menu_Main_f();    break;

    case K_UPARROW:
    case K_RIGHTARROW: {
        m_entersound = true;
        if (++help_page >= NUM_HELP_PAGES)
            help_page = 0;
    } break;

    case K_DOWNARROW:
    case K_LEFTARROW: {
        m_entersound = true;
        if (--help_page < 0)
            help_page = NUM_HELP_PAGES - 1;
    } break;
    default: break;
    }

}
