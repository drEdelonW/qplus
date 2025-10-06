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
    float r;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    qpic_p p = Draw_CachePic("gfx/p_option.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_Print(16, 32, "    Customize controls");
    M_Print(16, 40, "         Go to console");
    M_Print(16, 48, "     Reset to defaults");

    M_Print(16, 56, "           Screen size");
    r = (scr_viewsize.value - 30) / (120 - 30);
    M_DrawSlider(220, 56, r);

    M_Print(16, 64, "            Brightness");
    r = (1.0 - v_gamma.value) / 0.5;
    M_DrawSlider(220, 64, r);

    M_Print(16, 72, "           Mouse Speed");
    r = (sensitivity.value - 1) / 10;
    M_DrawSlider(220, 72, r);

    M_Print(16, 80, "       CD Music Volume");
    r = bgmvolume.value;
    M_DrawSlider(220, 80, r);

    M_Print(16, 88, "          Sound Volume");
    r = volume.value;
    M_DrawSlider(220, 88, r);

    M_Print(16, 96, "            Always Run");
    M_DrawCheckbox(220, 96, cl_forwardspeed.value > 200);

    M_Print(16, 104, "          Invert Mouse");
    M_DrawCheckbox(220, 104, m_pitch.value < 0);

    M_Print(16, 112, "            Lookspring");
    M_DrawCheckbox(220, 112, lookspring.value);

    M_Print(16, 120, "            Lookstrafe");
    M_DrawCheckbox(220, 120, lookstrafe.value);

    if (vid_menudrawfn)
        M_Print(16, 128, "         Video Options");

#ifdef _WIN32
    if (modestate == MS_WINDOWED) {
        M_Print(16, 136, "             Use Mouse");
        M_DrawCheckbox(220, 136, _windowed_mouse.value);
    }
#endif

    // cursor
    M_DrawCharacter(200, 32 + _options_cursor * 8, 12 + ((int)(realtime * 4) & 1));
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

