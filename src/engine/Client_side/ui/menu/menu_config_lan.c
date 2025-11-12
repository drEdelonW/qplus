#include "menu.h"
#include "menu_prv.h"
#include "menu_net.h"
#include "net.h"
#include "q_tools.h"
#include <string.h>
#include "cbuf.h"

//=============================================================================
/* LAN CONFIG MENU */

// typedef enum {
//     MNET_IPX = 1,
//     MNET_TCP = 2
// } mnet_type_t;
// mnet_type_t m_activenet;

typedef enum {
    lc_UNINITED = -1,
    lc_FIRST    = 0,
    lc_Port     = lc_FIRST,
    lc_Search,
    lc_Ok       = lc_Search,
    lc_JoinName,

    lc_LAST     //should be last
} LanConfig_e;
static LanConfig_e _cursor = lc_UNINITED;
LanConfig_t lanConfig;

static int _y[] = {
    72,     // lc_Port
    92,     // lc_Search
    124     // lc_Ok/lc_Search,
};


void M_Menu_LanConfig_f() {
    key.dest = key_menu;
    m_state = m_lanconfig;
    m_entersound = true;
    if (_cursor == lc_UNINITED) {
        if (is_JoinGame() && TCPIPConfig)   _cursor = lc_JoinName;
        else                                _cursor = 1;
    }
    if (is_CreateGame() &&
        (_cursor == lc_JoinName))
        _cursor = 1;
    lanConfig.port = DEFAULTnet_hostport;
    sprintf(lanConfig.portname, "%u", lanConfig.port);

    m_return_onerror = false;
    m_return_reason[0] = 0x00;
}


void M_LanConfig_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));

    int basex = M_DrawPicHC(4, Draw_CachePic("gfx/p_multi.lmp"));

    cString startJoin = (is_CreateGame()) ? "New Game" : "Join Game";
    cString protocol = (IPXConfig) ? "IPX" : "TCP/IP";
    M_Print(basex, 32, va("%s - %s", startJoin, protocol));

    basex += 8;
    M_Print(basex, 52, "Address:");
    M_Print(basex + 9 * 8, 52, (IPXConfig) ? my_ipx_address : my_tcpip_address);

    M_Print(basex, _y[lc_Port], "Port");
    M_DrawTextBox(basex + 8 * 8, _y[lc_Port] - 8, 6, 1); {
        M_Print(basex + 9 * 8, _y[lc_Port], lanConfig.portname);
    }

    if (is_JoinGame()) {
        M_Print(basex, _y[lc_Search], "Search for local games...");
        M_Print(basex, 108, "Join game at:");
        M_DrawTextBox(basex + 8, _y[2] - 8, 22, 1); {
            M_Print(basex + 16, _y[2], lanConfig.joinname);
        }
    }
    else {
        M_DrawTextBox(basex, _y[lc_Search] - 8, 2, 1); {
            M_Print(basex + 8, _y[lc_Search], "OK");
        }
    }

    M_DrawCharacter(basex - 8, _y[_cursor], curSymb());

    if (_cursor == lc_Port)
        M_DrawCharacter(basex + 9 * 8 + 8 * strlen(lanConfig.portname), _y[lc_Port], inpSymb());

    if (_cursor == lc_JoinName)
        M_DrawCharacter(basex + 16 + 8 * strlen(lanConfig.joinname), _y[lc_JoinName], inpSymb());

    if (*m_return_reason)
        M_PrintWhite(basex, 148, m_return_reason);
}


void M_LanConfig_Key(keycode_t Key) {
    switch (Key) {
    case K_ESCAPE:  M_Menu_Net_f(); break;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        if (--_cursor < lc_FIRST) { _cursor = lc_LAST - 1; } break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        if (++_cursor >= lc_LAST) { _cursor = lc_FIRST; } break;

    case K_ENTER:
        if (_cursor == lc_Port) break;

        m_entersound = true;

        M_ConfigureNetSubsystem();

        if (_cursor == 1) {
            if (is_CreateGame()) {
                M_Menu_GameOptions_f();
                break;
            }
            M_Menu_Search_f();
            break;
        }

        if (_cursor == lc_JoinName) {
            m_return_state = m_state;
            m_return_onerror = true;
            key.dest = key_game;
            m_state = m_none;
            Cbuf_AddText(va("connect \"%s\"\n", lanConfig.joinname));
            break;
        }

        break;

    case K_BACKSPACE:
        if (_cursor == lc_Port) {
            if (strlen(lanConfig.portname))
                lanConfig.portname[strlen(lanConfig.portname) - 1] = 0x00;
        }

        if (_cursor == lc_JoinName) {
            if (strlen(lanConfig.joinname))
                lanConfig.joinname[strlen(lanConfig.joinname) - 1] = 0x00;
        }
        break;

    default:
        if (!is_printable(Key)) break;

        if (_cursor == lc_JoinName) {
            int l = strlen(lanConfig.joinname);
            if (l < 21) {
                lanConfig.joinname[l] = Key;
                lanConfig.joinname[l + 1] = 0x00;
            }
        }

        if (!is_digits(Key))    break;
        if (_cursor == lc_Port) {
            int l = strlen(lanConfig.portname);
            if (l < 5) {
                lanConfig.portname[l] = Key;
                lanConfig.portname[l + 1] = 0x00;
            }
        }
    }

    if ((is_CreateGame()) &&
        (_cursor == lc_JoinName)
        ) {
        if (Key == K_UPARROW)   _cursor = 1;
        else                    _cursor = lc_Port;
    }

    int l = Q_atoi(lanConfig.portname);
    if (l > 0xFFFF)     l = lanConfig.port;
    else                lanConfig.port = l;

    sprintf(lanConfig.portname, "%u", lanConfig.port);
}
