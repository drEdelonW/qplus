#include "menu.h"
#include "menu_prv.h"
#include "menu_net.h"


//=============================================================================
/* NET MENU */

int m_net_cursor;
int m_net_items;
int m_net_saveHeight;

cString net_helpMessage[][4]/*[25]*/ = {
    /* .........1.........2.... */{
        "                        ",
        " Two computers connected",
        "   through two modems.  ",
        "                        "
    },
    {
        "                        ",
        " Two computers connected",
        " by a null-modem cable. ",
        "                        "
    },
    {
        " Novell network LANs    ",
        " or Windows 95 DOS-box. ",
        "                        ",
        "(LAN=Local Area Network)"
    },
    {
        " Commonly used to play  ",
        " over the Internet, but ",
        " also used on a Local   ",
        " Area Network.          "
    }
};

void M_Menu_Net_f() {
    key_dest = key_menu;
    m_state = m_net;
    m_entersound = true;
    m_net_items = 4;

    if (m_net_cursor >= m_net_items)
        m_net_cursor = 0;
    m_net_cursor--;
    M_Net_Key(K_DOWNARROW);
}

void M_Net_Draw() {
    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));

    qPic_p p = Draw_CachePic("gfx/p_multi.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    if (serialAvailable) {
        p = Draw_CachePic("gfx/netmen1.lmp");
    }
    else {
#ifdef _WIN32
        p = NULL;
#else
        p = Draw_CachePic("gfx/dim_modm.lmp");
#endif
    }

    int f = 32;
    if (p)
        M_DrawTransPic(72, f, p);

    M_DrawTransPic(72, f += 19,
        (serialAvailable) ?
        Draw_CachePic("gfx/netmen2.lmp") :
#ifdef _WIN32
        NULL
#else
        Draw_CachePic("gfx/dim_drct.lmp")
#endif
    );

    M_DrawTransPic(72, f += 19, Draw_CachePic((ipxAvailable) ? "gfx/netmen3.lmp" : "gfx/dim_ipx.lmp"));
    M_DrawTransPic(72, f += 19, Draw_CachePic((tcpipAvailable) ? "gfx/netmen4.lmp" : "gfx/dim_tcp.lmp"));

    if (m_net_items == 5) { // JDC, could just be removed
        M_DrawTransPic(72, f += 19, Draw_CachePic("gfx/netmen5.lmp"));
    }

    f = (320 - 26 * 8) / 2;
    M_DrawTextBox(f, 134, 24, 4);
    f += 8;
    for (int i = 0; i < 4; i++)
        M_Print(f, 142 + (i * 8), net_helpMessage[m_net_cursor][i]);


    M_DrawTransPic(
        54, 32 + m_net_cursor * 20,
        Draw_CachePic(va("gfx/menudot%i.lmp", curAmimFrame())
        )
    );
}


void M_Net_Key(keycode_t k) {
again:
    switch (k) {
    case K_ESCAPE:      M_Menu_MultiPlayer_f(); break;

    case K_DOWNARROW:   S_LocalSound("misc/menu1.wav");
        if (++m_net_cursor >= m_net_items)  m_net_cursor = 0;
        break;

    case K_UPARROW:     S_LocalSound("misc/menu1.wav");
        if (--m_net_cursor < 0) m_net_cursor = m_net_items - 1;
        break;

    case K_ENTER:
        m_entersound = true;

        switch (m_net_cursor) {
        case 0: M_Menu_SerialConfig_f();    break;
        case 1: M_Menu_SerialConfig_f();    break;
        case 2: M_Menu_LanConfig_f();       break;
        case 3: M_Menu_LanConfig_f();       break;
        case 4: /* multiprotocol */         break;
        }
    default: break;
    }

    if (((m_net_cursor == 0) && (!serialAvailable)) ||
        ((m_net_cursor == 1) && (!serialAvailable)) ||
        ((m_net_cursor == 2) && (!ipxAvailable)) ||
        ((m_net_cursor == 3) && (!tcpipAvailable)))
        goto again;
}


//=============================================================================
/* VIDEO MENU */

void M_Menu_Video_f() {
    key_dest = key_menu;
    m_state = m_video;
    m_entersound = true;
}


void M_Video_Draw() { (*vid_menudrawfn) (); }
void M_Video_Key(keycode_t key) { (*vid_menukeyfn) (key); }
void M_ConfigureNetSubsystem() {
    // enable/disable net systems to match desired config

    Cbuf_AddText("stopdemo\n");
    if (SerialConfig ||
        DirectConfig) {
        Cbuf_AddText("com1 enable\n");
    }

    if (IPXConfig ||
        TCPIPConfig)
        net_hostport = lanConfig_port;
}
