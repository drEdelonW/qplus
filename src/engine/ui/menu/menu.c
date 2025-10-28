/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "menu.h"
#include "menu_prv.h"
#include "console.h"
#include "cmd.h"
#include "vid.h"
#include "screen.h"


#ifdef _WIN32
#   include "winquake.h"
#endif

m_state_t m_state;
m_state_t m_return_state;
bool m_entersound;  // play after drawing a frame, so caching
// won't disrupt the sound

void (*vid_menudrawfn)();
void (*vid_menukeyfn)(keycode_t key);

bool m_recursiveDraw;
bool m_return_onerror;
char m_return_reason[32];

//=============================================================================


/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f() {
    m_entersound = true;

    if (key_dest == key_menu) {
        if (m_state != m_main) { M_Menu_Main_f(); return; }
        key_dest = key_game;
        m_state = m_none;
        return;
    }
    if (key_dest == key_console)    Con_ToggleConsole_f();
    else                            M_Menu_Main_f();

}



//=============================================================================
/* Menu Subsystem */


void M_Init() {
    Cmd_AddCommand("togglemenu", M_ToggleMenu_f);

    Cmd_AddCommand("menu_main", M_Menu_Main_f);
    Cmd_AddCommand("menu_singleplayer", M_Menu_SinglePlayer_f);
    Cmd_AddCommand("menu_load", M_Menu_Load_f);
    Cmd_AddCommand("menu_save", M_Menu_Save_f);
    Cmd_AddCommand("menu_multiplayer", M_Menu_MultiPlayer_f);
    Cmd_AddCommand("menu_setup", M_Menu_Setup_f);
    Cmd_AddCommand("menu_options", M_Menu_Options_f);
    Cmd_AddCommand("menu_keys", M_Menu_Keys_f);
    Cmd_AddCommand("menu_video", M_Menu_Video_f);
    Cmd_AddCommand("help", M_Menu_Help_f);
    Cmd_AddCommand("menu_quit", M_Menu_Quit_f);
}


void M_Draw() {
    if ((m_state == m_none) ||
        (key_dest != key_menu))
        return;

    if (!m_recursiveDraw) {
        scr_copyeverything = 1;

        if (scr_con_current) {
            Draw_ConsoleBackground(vid.height);
            VID_UnlockBuffer();
            S_ExtraUpdate();
            VID_LockBuffer();
        }
        else    Draw_FadeScreen();

        scr_fullupdate = 0;
    }
    else { m_recursiveDraw = false; }

    switch (m_state) {
    case m_none:            break;
    case m_main:            M_Main_Draw();          break;
    case m_singleplayer:    M_SinglePlayer_Draw();  break;
    case m_load:            M_Load_Draw();          break;
    case m_save:            M_Save_Draw();          break;
    case m_multiplayer:     M_MultiPlayer_Draw();   break;
    case m_setup:           M_Setup_Draw();         break;
    case m_net:             M_Net_Draw();           break;
    case m_options:         M_Options_Draw();       break;
    case m_keys:            M_Keys_Draw();          break;
    case m_video:           M_Video_Draw();         break;
    case m_help:            M_Help_Draw();          break;
    case m_quit:            M_Quit_Draw();          break;
    case m_serialconfig:    M_SerialConfig_Draw();  break;
    case m_modemconfig:     M_ModemConfig_Draw();   break;
    case m_lanconfig:       M_LanConfig_Draw();     break;
    case m_gameoptions:     M_GameOptions_Draw();   break;
    case m_search:          M_Search_Draw();        break;
    case m_slist:           M_ServerList_Draw();    break;
    }

    if (m_entersound) {
        S_LocalSound("misc/menu2.wav");
        m_entersound = false;
    }

    VID_UnlockBuffer();
    S_ExtraUpdate();
    VID_LockBuffer();
}


void M_Keydown(keycode_t key) {
    switch (m_state) {
    case m_none:        return;
    case m_main:            M_Main_Key(key);            return;
    case m_singleplayer:    M_SinglePlayer_Key(key);    return;
    case m_load:            M_Load_Key(key);            return;
    case m_save:            M_Save_Key(key);            return;
    case m_multiplayer:     M_MultiPlayer_Key(key);     return;
    case m_setup:           M_Setup_Key(key);           return;
    case m_net:             M_Net_Key(key);             return;
    case m_options:         M_Options_Key(key);         return;
    case m_keys:            M_Keys_Key(key);            return;
    case m_video:           M_Video_Key(key);           return;
    case m_help:            M_Help_Key(key);            return;
    case m_quit:            M_Quit_Key(key);            return;
    case m_serialconfig:    M_SerialConfig_Key(key);    return;
    case m_modemconfig:     M_ModemConfig_Key(key);     return;
    case m_lanconfig:       M_LanConfig_Key(key);       return;
    case m_gameoptions:     M_GameOptions_Key(key);     return;
    case m_search:          M_Search_Key(key);          break;
    case m_slist:           M_ServerList_Key(key);      return;
    }
}


