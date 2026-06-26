#include "menu.h"
#include "menu_prv.h"
#include "host.h"


//=============================================================================
/* QUIT MENU */

static int  _msgNumber;
static int  _m_quitPrevState;
static bool _wasInMenus;

#ifndef _WIN32
cString quitMessage[][4] = {
    /* .........1.........2.... */
    {
        "  Are you gonna quit    ",
        "  this game just like   ",
        "   everything else?     ",
        "                        ",
    },
    {
        " Milord, methinks that  ",
        "   thou art a lowly     ",
        " quitter. Is this true? ",
        "                        ",
    },
    {
        " Do I need to bust your ",
        "  face open for trying  ",
        "        to quit?        ",
        "                        ",
    },
    {
        " Man, I oughta smack you",
        "   for trying to quit!  ",
        "     Press Y to get     ",
        "      smacked out.      ",
    },
    {
        " Press Y to quit like a ",
        "   big loser in life.   ",
        "  Press N to stay proud ",
        "    and successful!     ",
    },
    {
        "   If you press Y to    ",
        "  quit, I will summon   ",
        "  Satan all over your   ",
        "      hard drive!       ",
    },
    {
        "  Um, Asmodeus dislikes ",
        " his children trying to ",
        " quit. Press Y to return",
        "   to your Tinkertoys.  ",
    },
    {
        "  If you quit now, I'll ",
        "  throw a blanket-party ",
        "   for you next time!   ",
        "                        "
    }
};
#endif

#include <stdlib.h>
void M_Menu_Quit_f() {
    if (m_state == m_quit)
        return;

    _wasInMenus = (key.dest == key_menu);
    key.dest = key_menu;
    _m_quitPrevState = m_state;
    m_state = m_quit;
    m_entersound = true;
    _msgNumber = rand() & 7;
}


void M_Quit_Key(keycode_t Key) {
    switch ((char)Key) {
    case K_ESCAPE:
    case 'n':
    case 'N': {
        if (_wasInMenus) {
            m_state = _m_quitPrevState;
            m_entersound = true;
        }
        else {
            key.dest = key_game;
            m_state = m_none;
        }
    } break;

    case 'Y':
    case 'y': {
        key.dest = key_console;
        Host_Quit_f();
    } break;

    default:    break;
    }

}


void M_Quit_Draw() {
    if (_wasInMenus) {
        m_state = _m_quitPrevState;
        m_recursiveDraw = true;
        M_Draw();
        m_state = m_quit;
    }

#ifdef _WIN32
    M_DrawTextBox(0, 0, 38, 23); {
        int y = -4;
        M_PrintWhite(16, y += 16, "  Quake version 1.09 by id Software\n\n");
        M_PrintWhite(16, y += 16, "Programming        Art \n");
        M_Print(16, y += 16, " John Carmack       Adrian Carmack\n");
        M_Print(16, y += 16, " Michael Abrash     Kevin Cloud\n");
        M_Print(16, y += 16, " John Cash          Paul Steed\n");
        M_Print(16, y += 16, " Dave 'Zoid' Kirsch\n");
        M_PrintWhite(16, y += 16, "Design             Biz\n");
        M_Print(16, y += 16, " John Romero        Jay Wilbur\n");
        M_Print(16, y += 16, " Sandy Petersen     Mike Wilson\n");
        M_Print(16, y += 16, " American McGee     Donna Jackson\n");
        M_Print(16, y += 16, " Tim Willits        Todd Hollenshead\n");
        M_PrintWhite(16, y += 16, "Support            Projects\n");
        M_Print(16, y += 16, " Barrett Alexander  Shawn Green\n");
        M_PrintWhite(16, y += 16, "Sound Effects\n");
        M_Print(16, y += 16, " Trent Reznor and Nine Inch Nails\n\n");
        M_PrintWhite(16, y += 16, "Quake is a trademark of Id Software,\n");
        M_PrintWhite(16, y += 16, "inc., (c)1996 Id Software, inc. All\n");
        M_PrintWhite(16, y += 16, "rights reserved. NIN logo is a\n");
        M_PrintWhite(16, y += 16, "registered trademark licensed to\n");
        M_PrintWhite(16, y += 16, "Nothing Interactive, Inc. All rights\n");
        M_PrintWhite(16, y += 16, "reserved. Press y to exit\n");
}
#else
    M_DrawTextBox(56, 76, 24, 4); {
        for (int i = 0; i < 4; i++)
            M_Print(64, 84 + i * 8, quitMessage[_msgNumber][i]);
    }
#endif
}
