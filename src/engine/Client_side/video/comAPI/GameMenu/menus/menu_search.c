#include "menu.h"
#include "menu_prv.h"
#include "net.h"
#include "host.h"


//=============================================================================
/* SEARCH MENU */

bool searchComplete = false;
LegTime_t searchCompleteTime;

void M_Menu_Search_f() {
    key.dest = key_menu;
    m_state = m_search;
    m_entersound = false;
    slistSilent = true;
    slistLocal = false;
    searchComplete = false;
    NET_Slist_f();
}


void M_Search_Draw() {
    M_DrawPicHC(4, Draw_CachePic("gfx/p_multi.lmp"));

    int x = HALF(320) - (HALF(12 * 8)) + 4;
    M_DrawTextBox(x - 8, 32, 12, 1); {
        M_Print(x, 40, "Searching...");
    }

    if (slistInProgress) { NET_Poll();      return; }

    if (!searchComplete) {
        searchComplete = true;
        searchCompleteTime = GetRealTime();
    }

    if (hostCacheCount) { M_Menu_ServerList_f();    return; }

    M_PrintWhite(HALF(320) - (HALF(22 * 8)), 64, "No Quake servers found");
    if ((GetRealTime() - searchCompleteTime) < 3.0)
        return;

    M_Menu_LanConfig_f();
}


void M_Search_Key(keycode_t key) {}
