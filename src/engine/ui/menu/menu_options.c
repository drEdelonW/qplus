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
typedef enum {
    o_force_signed = -1,
    o_FIRST = 0,
    o_CostumizeCtrl = o_FIRST,
    o_GoConsole,
    o_ResetDefault,
    o_ScreenSize,
    o_Brightness,
    o_MouseSpeed,
    o_CDVolume,
    o_SndVolume,
    o_AlwaysRun,
    o_InvertMouse,
    o_LookSpring,
    o_LookStrafe,
    o_VideoOptions,
    o_UseMouse,

    o_LAST     //should be last
} options_e;
static options_e _cursor;
// void (*vid_menudrawfn)();

void M_Menu_Options_f() {
    key_dest = key_menu;
    m_state = m_options;
    m_entersound = true;

#ifdef _WIN32
    if ((_cursor == o_UseMouse) &&
        (modestate != MS_WINDOWED)) {
        _cursor = o_FIRST;
    }
#endif
}

typedef enum {
    md_left = -1,
    md_right = 1
} menuDirection_e;

void M_AdjustSliders(menuDirection_e dir) {
    S_LocalSound("misc/menu3.wav");

    switch (_cursor) {
    case o_ScreenSize: // screen size
        scr_viewsize.value += dir * 10; CLAMP(30.0f, scr_viewsize.value, 120.0f);
        Cvar_SetValue("viewsize", scr_viewsize.value);
        break;
    case o_Brightness: // gamma
        v_gamma.value -= dir * 0.05;    CLAMP(0.5f, v_gamma.value, 1.0f);
        Cvar_SetValue("gamma", v_gamma.value);
        break;
    case o_MouseSpeed: // mouse speed
        sensitivity.value += dir * 0.5; CLAMP(1.0f, sensitivity.value, 11.0f);
        Cvar_SetValue("sensitivity", sensitivity.value);
        break;
    case o_CDVolume: // music volume
#ifdef _WIN32
        bgmvolume.value += dir * 1.0f;
#else
        bgmvolume.value += dir * 0.1f;
#endif
        CLAMP(0.0f, bgmvolume.value, 1.0f);
        Cvar_SetValue("bgmvolume", bgmvolume.value);
        break;
    case o_SndVolume: // sfx volume
        volume.value += dir * 0.1f; CLAMP(0.0f, volume.value, 1.0f);
        Cvar_SetValue("volume", volume.value);
        break;

    case o_AlwaysRun: /* allways run */
        if (cl_forwardspeed.value > 200) {
            Cvar_SetValue("cl_forwardspeed", 200);
            Cvar_SetValue("cl_backspeed", 200);
        }
        else {
            Cvar_SetValue("cl_forwardspeed", 400);
            Cvar_SetValue("cl_backspeed", 400);
        }
        break;

    case o_InvertMouse: Cvar_SetValue("m_pitch", -m_pitch.value);       break;
    case o_LookSpring:  Cvar_SetValue("lookspring", !lookspring.value); break;
    case o_LookStrafe:  Cvar_SetValue("lookstrafe", !lookstrafe.value); break;

        // #ifdef _WIN32
    case o_VideoOptions: // _windowed_mouse
        Cvar_SetValue("_windowed_mouse", !_windowed_mouse.value);
        break;
        // #endif
    default: break;
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
    M_DrawCharacter(200, 32 + _cursor * CHAR_HEIGHT, curSymb());
}

void M_Options_Key(keycode_t k) {
    switch (k) {
    case K_ESCAPE:  M_Menu_Main_f();    break;

    case K_ENTER:
        m_entersound = true;
        switch (_cursor) {
        case o_CostumizeCtrl:                   M_Menu_Keys_f();                    break;
        case o_GoConsole:   m_state = m_none;   Con_ToggleConsole_f();              break;
        case o_ResetDefault:                    Cbuf_AddText("exec default.cfg\n"); break;
        case o_VideoOptions:                    M_Menu_Video_f();                   break;
        default:                                M_AdjustSliders(md_right);          break;
        }
        return;

    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        _cursor--;
        if (_cursor < o_FIRST)  _cursor = o_LAST - 1;
        break;

    case K_DOWNARROW:
        S_LocalSound("misc/menu1.wav");
        _cursor++;
        if (_cursor >= o_LAST)  _cursor = o_FIRST;
        break;

    case K_LEFTARROW:   M_AdjustSliders(md_left);   break;
    case K_RIGHTARROW:  M_AdjustSliders(md_right);  break;
    default: break;
    }

    if ((_cursor == o_VideoOptions) &&
        (vid_menudrawfn == NULL)) {
        _cursor = (k == K_UPARROW) ? o_LookStrafe : o_FIRST;
    }

#ifdef _WIN32
    if ((_cursor == o_UseMouse) &&
        (modestate != MS_WINDOWED)) {
        _cursor = (k == K_UPARROW) ? o_VideoOptions : o_FIRST;
}
#endif
}

