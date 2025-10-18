#include "menu.h"
#include "menu_prv.h"


//=============================================================================
/* SLIST MENU */

int  slist_cursor;
bool slist_sorted;

void M_Menu_ServerList_f() {
    key_dest = key_menu;
    m_state = m_slist;
    m_entersound = true;
    slist_cursor = 0;
    m_return_onerror = false;
    m_return_reason[0] = 0;
    slist_sorted = false;
}

void M_ServerList_Draw() {
    if (!slist_sorted) {
        if (hostCacheCount > 1) {
            hostcache_t temp;
            for (int i = 0; i < hostCacheCount; i++)
                for (int j = i + 1; j < hostCacheCount; j++)
                    if (strcmp(hostcache[j].name, hostcache[i].name) < 0) {
                        Q_memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
                        Q_memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
                        Q_memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
                    }
        }
        slist_sorted = true;
    }

    qPic_p p = Draw_CachePic("gfx/p_multi.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);
    for (int n = 0; n < hostCacheCount; n++) {
        char string[64];
        if (hostcache[n].maxusers)
            sprintf(string, "%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
        else
            sprintf(string, "%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
        M_Print(16, 32 + 8 * n, string);
    }
    M_DrawCharacter(0, 32 + slist_cursor * 8, 12 + ((int)(realtime * 4) & 1));

    if (*m_return_reason)
        M_PrintWhite(16, 148, m_return_reason);
}


void M_ServerList_Key(keycode_t k) {
    switch (k) {
    case K_ESCAPE:
        M_Menu_LanConfig_f();
        break;

    case K_SPACE:
        M_Menu_Search_f();
        break;

    case K_UPARROW:
    case K_LEFTARROW:
        S_LocalSound("misc/menu1.wav");
        slist_cursor--;
        if (slist_cursor < 0)
            slist_cursor = hostCacheCount - 1;
        break;

    case K_DOWNARROW:
    case K_RIGHTARROW:
        S_LocalSound("misc/menu1.wav");
        slist_cursor++;
        if (slist_cursor >= hostCacheCount)
            slist_cursor = 0;
        break;

    case K_ENTER:
        S_LocalSound("misc/menu2.wav");
        m_return_state = m_state;
        m_return_onerror = true;
        slist_sorted = false;
        key_dest = key_game;
        m_state = m_none;
        Cbuf_AddText(va("connect \"%s\"\n", hostcache[slist_cursor].cname));
        break;

    default:
        break;
    }

}
