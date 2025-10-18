#include "menu.h"
#include "menu_prv.h"
#include "sound.h"
#include "cvar_q1.h"
#include "host.h"
#include "console.h"
#include "cmd.h"
#include "vid.h"
#ifdef _WIN32
#   include "winquake.h"
#endif

//=============================================================================
/* OPTIONS MENU */

static int _options_cursor;
// void (*vid_menudrawfn)();

void M_Menu_Options_f() {
    key_dest = key_menu;
    m_state = m_options;
    m_entersound = true;

#ifdef _WIN32
    if ((_options_cursor == 13) &&
        (modestate != MS_WINDOWED)) {
        _options_cursor = 0;
    }
#endif
}


void M_AdjustSliders(int dir) {
    S_LocalSound("misc/menu3.wav");

    switch (_options_cursor) {
    case 3: // screen size
        scr_viewsize.value += dir * 10;
        CLAMP(30.0f, scr_viewsize.value, 120.0f);
        Cvar_SetValue("viewsize", scr_viewsize.value);
        break;
    case 4: // gamma
        v_gamma.value -= dir * 0.05;
        CLAMP(0.5f, v_gamma.value, 1.0f);
        Cvar_SetValue("gamma", v_gamma.value);
        break;
    case 5: // mouse speed
        sensitivity.value += dir * 0.5;
        CLAMP(1.0f, sensitivity.value, 11.0f);
        Cvar_SetValue("sensitivity", sensitivity.value);
        break;
    case 6: // music volume
#ifdef _WIN32
        bgmvolume.value += dir * 1.0f;
#else
        bgmvolume.value += dir * 0.1f;
#endif
        CLAMP(0.0f, bgmvolume.value, 1.0f);
        Cvar_SetValue("bgmvolume", bgmvolume.value);
        break;
    case 7: // sfx volume
        volume.value += dir * 0.1f;
        CLAMP(0.0f, volume.value, 1.0f);
        Cvar_SetValue("volume", volume.value);
        break;

    case 8: // allways run
        if (cl_forwardspeed.value > 200) {
            Cvar_SetValue("cl_forwardspeed", 200);
            Cvar_SetValue("cl_backspeed", 200);
        }
        else {
            Cvar_SetValue("cl_forwardspeed", 400);
            Cvar_SetValue("cl_backspeed", 400);
        }
        break;

    case 9: // invert mouse
        Cvar_SetValue("m_pitch", -m_pitch.value);
        break;

    case 10: // lookspring
        Cvar_SetValue("lookspring", !lookspring.value);
        break;

    case 11: // lookstrafe
        Cvar_SetValue("lookstrafe", !lookstrafe.value);
        break;

#ifdef _WIN32
    case 13: // _windowed_mouse
        Cvar_SetValue("_windowed_mouse", !_windowed_mouse.value);
        break;
#endif
    }
}

void M_Options_Draw() {
    const int col0 = 16;
    const int col1 = 220;

    M_DrawTransPic(col0, 4, Draw_CachePic("gfx/qplaque.lmp"));
    qPic_p p = Draw_CachePic("gfx/p_option.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_Print(col0, 32, "    Customize controls");
    M_Print(col0, 40, "         Go to console");
    M_Print(col0, 48, "     Reset to defaults");
    M_Print(col0, 56, "           Screen size");    M_DrawSlider(col1, 56, (scr_viewsize.value - 30) / (120 - 30));
    M_Print(col0, 64, "            Brightness");    M_DrawSlider(col1, 64, (1.0 - v_gamma.value) / 0.5);
    M_Print(col0, 72, "           Mouse Speed");    M_DrawSlider(col1, 72, (sensitivity.value - 1) / 10);
    M_Print(col0, 80, "       CD Music Volume");    M_DrawSlider(col1, 80, bgmvolume.value);
    M_Print(col0, 88, "          Sound Volume");    M_DrawSlider(col1, 88, volume.value);
    M_Print(col0, 96, "            Always Run");    M_DrawCheckbox(col1, 96, cl_forwardspeed.value > 200);
    M_Print(col0, 104, "          Invert Mouse");   M_DrawCheckbox(col1, 104, m_pitch.value < 0);
    M_Print(col0, 112, "            Lookspring");   M_DrawCheckbox(col1, 112, lookspring.value);
    M_Print(col0, 120, "            Lookstrafe");   M_DrawCheckbox(col1, 120, lookstrafe.value);
    if (vid_menudrawfn)
        M_Print(col0, 128, "         Video Options");

#ifdef _WIN32
    if (modestate == MS_WINDOWED) {
        M_Print(col0, 136, "             Use Mouse");   M_DrawCheckbox(col1, 136, _windowed_mouse.value);
    }
#endif
    // cursor
    M_DrawCharacter(200, 32 + _options_cursor * CHAR_HEIGHT, 12 + ((int)(realtime * 4) & 1));
}

#ifdef _WIN32
#   define OPTIONS_ITEMS 14
#else
#   define OPTIONS_ITEMS 13
#endif

void M_Options_Key(keycode_t k) {
    switch (k) {
    case K_ESCAPE:
        M_Menu_Main_f();
        break;

    case K_ENTER:
        m_entersound = true;
        switch (_options_cursor) {
        case 0:
            M_Menu_Keys_f();
            break;
        case 1:
            m_state = m_none;
            Con_ToggleConsole_f();
            break;
        case 2:
            Cbuf_AddText("exec default.cfg\n");
            break;
        case 12:
            M_Menu_Video_f();
            break;
        default:
            M_AdjustSliders(1);
            break;
        }
        return;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        _options_cursor--;
        if (_options_cursor < 0)
            _options_cursor = OPTIONS_ITEMS - 1;
        break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        _options_cursor++;
        if (_options_cursor >= OPTIONS_ITEMS)
            _options_cursor = 0;
        break;

    case K_LEFTARROW:
        M_AdjustSliders(-1);
        break;

    case K_RIGHTARROW:
        M_AdjustSliders(1);
        break;
    default: break;
    }

    if ((_options_cursor == 12) &&
        (vid_menudrawfn == NULL)) {
        if (k == K_UPARROW)
            _options_cursor = 11;
        else
            _options_cursor = 0;
    }

#ifdef _WIN32
    if ((_options_cursor == 13) &&
        (modestate != MS_WINDOWED)) {
        if (k == K_UPARROW)
            _options_cursor = 12;
        else
            _options_cursor = 0;
    }
#endif
}

