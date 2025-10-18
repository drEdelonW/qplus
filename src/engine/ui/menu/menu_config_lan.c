#include "menu.h"
#include "menu_prv.h"
#include "menu_net.h"

//=============================================================================
/* LAN CONFIG MENU */

// typedef enum {
//     MNET_IPX = 1,
//     MNET_TCP = 2
// } mnet_type_t;
// mnet_type_t m_activenet;



int  lanConfig_cursor = -1;
int  lanConfig_cursor_table[] = { 72, 92, 124 };
#define NUM_LANCONFIG_CMDS 3

int  lanConfig_port;
char lanConfig_portname[6];
char lanConfig_joinname[22];

void M_Menu_LanConfig_f() {
    key_dest = key_menu;
    m_state = m_lanconfig;
    m_entersound = true;
    if (lanConfig_cursor == -1) {
        if (is_JoinGame() &&
            TCPIPConfig)
            lanConfig_cursor = 2;
        else
            lanConfig_cursor = 1;
    }
    if (is_CreateGame() &&
        (lanConfig_cursor == 2))
        lanConfig_cursor = 1;
    lanConfig_port = DEFAULTnet_hostport;
    sprintf(lanConfig_portname, "%u", lanConfig_port);

    m_return_onerror = false;
    m_return_reason[0] = 0;
}


void M_LanConfig_Draw() {

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    qPic_p p = Draw_CachePic("gfx/p_multi.lmp");
    int basex = (320 - p->width) / 2;
    M_DrawPic(basex, 4, p);

    cString startJoin = (is_CreateGame()) ? "New Game" : "Join Game";
    cString protocol = (IPXConfig) ? "IPX" : "TCP/IP";
    M_Print(basex, 32, va("%s - %s", startJoin, protocol));

    basex += 8;
    M_Print(basex, 52, "Address:");
    M_Print(basex + 9 * 8, 52, (IPXConfig) ? my_ipx_address : my_tcpip_address);

    M_Print(basex, lanConfig_cursor_table[0], "Port");
    M_DrawTextBox(basex + 8 * 8, lanConfig_cursor_table[0] - 8, 6, 1);
    M_Print(basex + 9 * 8, lanConfig_cursor_table[0], lanConfig_portname);

    if (is_JoinGame()) {
        M_Print(basex, lanConfig_cursor_table[1], "Search for local games...");
        M_Print(basex, 108, "Join game at:");
        M_DrawTextBox(basex + 8, lanConfig_cursor_table[2] - 8, 22, 1);
        M_Print(basex + 16, lanConfig_cursor_table[2], lanConfig_joinname);
    }
    else {
        M_DrawTextBox(basex, lanConfig_cursor_table[1] - 8, 2, 1);
        M_Print(basex + 8, lanConfig_cursor_table[1], "OK");
    }

    M_DrawCharacter(basex - 8, lanConfig_cursor_table[lanConfig_cursor], 12 + ((int)(realtime * 4) & 1));

    if (lanConfig_cursor == 0)
        M_DrawCharacter(basex + 9 * 8 + 8 * strlen(lanConfig_portname), lanConfig_cursor_table[0], 10 + ((int)(realtime * 4) & 1));

    if (lanConfig_cursor == 2)
        M_DrawCharacter(basex + 16 + 8 * strlen(lanConfig_joinname), lanConfig_cursor_table[2], 10 + ((int)(realtime * 4) & 1));

    if (*m_return_reason)
        M_PrintWhite(basex, 148, m_return_reason);
}


void M_LanConfig_Key(keycode_t key) {
    switch (key) {
    case K_ESCAPE:
        M_Menu_Net_f();
        break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        lanConfig_cursor--;
        if (lanConfig_cursor < 0)
            lanConfig_cursor = NUM_LANCONFIG_CMDS - 1;
        break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        lanConfig_cursor++;
        if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
            lanConfig_cursor = 0;
        break;

    case K_ENTER:
        if (lanConfig_cursor == 0)
            break;

        m_entersound = true;

        M_ConfigureNetSubsystem();

        if (lanConfig_cursor == 1) {
            if (is_CreateGame()) {
                M_Menu_GameOptions_f();
                break;
            }
            M_Menu_Search_f();
            break;
        }

        if (lanConfig_cursor == 2) {
            m_return_state = m_state;
            m_return_onerror = true;
            key_dest = key_game;
            m_state = m_none;
            Cbuf_AddText(va("connect \"%s\"\n", lanConfig_joinname));
            break;
        }

        break;

    case K_BACKSPACE:
        if (lanConfig_cursor == 0) {
            if (strlen(lanConfig_portname))
                lanConfig_portname[strlen(lanConfig_portname) - 1] = 0;
        }

        if (lanConfig_cursor == 2) {
            if (strlen(lanConfig_joinname))
                lanConfig_joinname[strlen(lanConfig_joinname) - 1] = 0;
        }
        break;

    default:
        if ((key < 32) ||
            (key > 127))
            break;

        if (lanConfig_cursor == 2) {
            int l = strlen(lanConfig_joinname);
            if (l < 21) {
                lanConfig_joinname[l + 1] = 0;
                lanConfig_joinname[l] = key;
            }
        }

        if ((key < '0') ||
            (key > '9'))
            break;
        if (lanConfig_cursor == 0) {
            int l = strlen(lanConfig_portname);
            if (l < 5) {
                lanConfig_portname[l + 1] = 0;
                lanConfig_portname[l] = key;
            }
        }
    }

    if ((is_CreateGame()) &&
        (lanConfig_cursor == 2)
        ) {
        if (key == K_UPARROW) {
            lanConfig_cursor = 1;
        }
        else {
            lanConfig_cursor = 0;
        }
    }

    int l = Q_atoi(lanConfig_portname);
    if (l > 0xFFFF)
        l = lanConfig_port;
    else
        lanConfig_port = l;
    sprintf(lanConfig_portname, "%u", lanConfig_port);
}
