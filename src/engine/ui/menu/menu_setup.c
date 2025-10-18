#include "menu.h"
#include "menu_prv.h"


//=============================================================================
/* SETUP MENU */

int  setup_cursor = 4;
int  setup_cursor_table[] = { 40, 56, 80, 104, 140 };

char setup_hostname[16];
char setup_myname[16];
int  setup_oldtop;
int  setup_oldbottom;
int  setup_top;
int  setup_bottom;

#define NUM_SETUP_CMDS 5

void M_Menu_Setup_f() {
    key_dest = key_menu;
    m_state = m_setup;
    m_entersound = true;
    Q_strcpy(setup_myname, cl_name.string);
    Q_strcpy(setup_hostname, hostname.string);
    setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
    setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
}


void M_Setup_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    qPic_p p = Draw_CachePic("gfx/p_multi.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_Print(64, 40, "Hostname");    M_DrawTextBox(160, 32, 16, 1);  M_Print(168, 40, setup_hostname);
    M_Print(64, 56, "Your name");   M_DrawTextBox(160, 48, 16, 1);  M_Print(168, 56, setup_myname);
    M_Print(64, 80, "Shirt color");
    M_Print(64, 104, "Pants color");
    M_DrawTextBox(64, 140 - 8, 14, 1);    M_Print(72, 140, "Accept Changes");

    M_DrawTransPic(160, 64, Draw_CachePic("gfx/bigbox.lmp"));
    M_BuildTranslationTable(setup_top * 16, setup_bottom * 16);
    M_DrawTransPicTranslate(172, 72, Draw_CachePic("gfx/menuplyr.lmp"));

    M_DrawCharacter(56, setup_cursor_table[setup_cursor], 12 + ((int)(realtime * 4) & 1));

    if (setup_cursor == 0)
        M_DrawCharacter(168 + 8 * strlen(setup_hostname), setup_cursor_table[setup_cursor], 10 + ((int)(realtime * 4) & 1));

    if (setup_cursor == 1)
        M_DrawCharacter(168 + 8 * strlen(setup_myname), setup_cursor_table[setup_cursor], 10 + ((int)(realtime * 4) & 1));
}


void M_Setup_Key(keycode_t k) {
    switch (k) {
    case K_ESCAPE:
        M_Menu_MultiPlayer_f();
        break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        setup_cursor--;
        if (setup_cursor < 0)
            setup_cursor = NUM_SETUP_CMDS - 1;
        break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        setup_cursor++;
        if (setup_cursor >= NUM_SETUP_CMDS)
            setup_cursor = 0;
        break;

    case K_LEFTARROW:
        if (setup_cursor < 2)
            return;
        S_LocalSound("misc/menu3.wav");
        if (setup_cursor == 2)
            setup_top = setup_top - 1;
        if (setup_cursor == 3)
            setup_bottom = setup_bottom - 1;
        break;
    case K_RIGHTARROW:
        if (setup_cursor < 2)
            return;
    forward:
        S_LocalSound("misc/menu3.wav");
        if (setup_cursor == 2)
            setup_top = setup_top + 1;
        if (setup_cursor == 3)
            setup_bottom = setup_bottom + 1;
        break;

    case K_ENTER:
        if ((setup_cursor == 0) ||
            (setup_cursor == 1))
            return;

        if ((setup_cursor == 2) ||
            (setup_cursor == 3))
            goto forward;

        // setup_cursor == 4 (OK)
        if (Q_strcmp(cl_name.string, setup_myname) != 0)
            Cbuf_AddText(va("name \"%s\"\n", setup_myname));
        if (Q_strcmp(hostname.string, setup_hostname) != 0)
            Cvar_Set("hostname", setup_hostname);
        if ((setup_top != setup_oldtop) ||
            (setup_bottom != setup_oldbottom))
            Cbuf_AddText(va("color %i %i\n", setup_top, setup_bottom));
        m_entersound = true;
        M_Menu_MultiPlayer_f();
        break;

    case K_BACKSPACE:
        if ((setup_cursor == 0) &&
            (strlen(setup_hostname)))
            setup_hostname[strlen(setup_hostname) - 1] = 0;

        if ((setup_cursor == 1) &&
            (strlen(setup_myname)))
            setup_myname[strlen(setup_myname) - 1] = 0;
        break;

    default:
        if ((k < 32) ||
            (k > 127))
            break;
        if (setup_cursor == 0) {
            int l = strlen(setup_hostname);
            if (l < 15) {
                setup_hostname[l + 1] = 0;
                setup_hostname[l] = k;
            }
        }
        if (setup_cursor == 1) {
            int l = strlen(setup_myname);
            if (l < 15) {
                setup_myname[l + 1] = 0;
                setup_myname[l] = k;
            }
        }
    }

    if (setup_top > 13)
        setup_top = 0;
    if (setup_top < 0)
        setup_top = 13;
    if (setup_bottom > 13)
        setup_bottom = 0;
    if (setup_bottom < 0)
        setup_bottom = 13;
}
