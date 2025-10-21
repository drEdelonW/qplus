#include "menu.h"
#include "menu_prv.h"
#include "enginedefs.h"
#include <string.h>
#include "common.h"
#include "server.h"
#include "cmd.h"


//=============================================================================
/* LOAD/SAVE MENU */

int  load_cursor;  // 0 < load_cursor < MAX_SAVEGAMES

#define MAX_SAVEGAMES  12
char m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH + 1];
int  loadable[MAX_SAVEGAMES];

void M_ScanSaves() {

    for (int i = 0; i < MAX_SAVEGAMES; i++) {
        strcpy(m_filenames[i], "--- UNUSED SLOT ---");
        loadable[i] = false;

        char name[MAX_OSPATH];
        snprintf(name, sizeof(name), "%s/s%i.sav", com_gamedir, i);
        FILE* f = fopen(name, "r");
        if (!f)
            continue;

        int  version;
        fscanf(f, "%i\n", &version);
        fscanf(f, "%79s\n", name);
        strncpy(m_filenames[i], name, sizeof(m_filenames[i]) - 1);

        // change _ back to space
        for (int j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
            if (m_filenames[i][j] == '_')
                m_filenames[i][j] = ' ';
        loadable[i] = true;
        fclose(f);
    }
}

void M_Menu_Load_f() {
    m_entersound = true;
    m_state = m_load;
    key_dest = key_menu;
    M_ScanSaves();
}


void M_Menu_Save_f() {
    if ((!sv.active) ||
        (cl.intermission) ||
        (svs.maxclients != 1))
        return;
    m_entersound = true;
    m_state = m_save;
    key_dest = key_menu;
    M_ScanSaves();
}

void M_Load_Draw() {
    M_DrawPicHC(4, Draw_CachePic("gfx/p_load.lmp"));

    for (int i = 0; i < MAX_SAVEGAMES; i++)
        M_Print(16, 32 + 8 * i, m_filenames[i]);

    // line cursor
    M_DrawCharacter(8, 32 + load_cursor * 8, curSymb());
}


void M_Save_Draw() {
    M_DrawPicHC(4, Draw_CachePic("gfx/p_save.lmp"));

    for (int i = 0; i < MAX_SAVEGAMES; i++)
        M_Print(16, 32 + 8 * i, m_filenames[i]);

    // line cursor
    M_DrawCharacter(8, 32 + load_cursor * 8, curSymb());
}


void M_Load_Key(keycode_t k) {
    switch (k) {
    case K_ESCAPE:
        M_Menu_SinglePlayer_f();
        break;

    case K_ENTER:
        S_LocalSound("misc/menu2.wav");
        if (!loadable[load_cursor])
            return;
        m_state = m_none;
        key_dest = key_game;

        // Host_Loadgame_f can't bring up the loading plaque because too much
        // stack space has been used, so do it now
        SCR_BeginLoadingPlaque();

        // issue the load command
        Cbuf_AddText(va("load s%i\n", load_cursor));
        return;

    case K_UPARROW:
    case K_LEFTARROW:
        S_LocalSound("misc/menu1.wav");
        if (--load_cursor < 0)
            load_cursor = MAX_SAVEGAMES - 1;
        break;

    case K_DOWNARROW:
    case K_RIGHTARROW:
        S_LocalSound("misc/menu1.wav");
        if (++load_cursor >= MAX_SAVEGAMES)
            load_cursor = 0;
        break;
    default: break;
    }
}


void M_Save_Key(keycode_t k) {
    switch (k) {
    case K_ESCAPE:
        M_Menu_SinglePlayer_f();
        break;

    case K_ENTER:
        m_state = m_none;
        key_dest = key_game;
        Cbuf_AddText(va("save s%i\n", load_cursor));
        return;

    case K_UPARROW:
    case K_LEFTARROW:
        S_LocalSound("misc/menu1.wav");
        if (--load_cursor < 0)
            load_cursor = MAX_SAVEGAMES - 1;
        break;

    case K_DOWNARROW:
    case K_RIGHTARROW:
        S_LocalSound("misc/menu1.wav");
        if (++load_cursor >= MAX_SAVEGAMES)
            load_cursor = 0;
        break;
    default: break;
    }
}
