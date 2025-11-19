#include "menu.h"
#include "menu_prv.h"
#include "vid.h"
#include "client.h"
#include <string.h>
#include "sound.h"
#include "cmd.h"
#include "cbuf.h"
#include "host.h"

//=============================================================================
/* KEYS MENU */
enum bind_st {
    command     = 0u,
    description,
    bs_num
};

static int _keys_cursor;
static bool _bind_grab;
static cString _bindnames[][bs_num] = {
    {"+attack",     "attack"},
    {"impulse 10",  "change weapon"},
    {"+jump",       "jump / swim up"},
    {"+forward",    "walk forward"},
    {"+back",       "backpedal"},
    {"+left",       "turn left"},
    {"+right",      "turn right"},
    {"+speed",      "run"},
    {"+moveleft",   "step left"},
    {"+moveright",  "step right"},
    {"+strafe",     "sidestep"},
    {"+lookup",     "look up"},
    {"+lookdown",   "look down"},
    {"centerview",  "center view"},
    {"+mlook",      "mouse look"},
    {"+klook",      "keyboard look"},
    {"+moveup",     "swim up"},
    {"+movedown",   "swim down"}
};

#define NUMCOMMANDS (sizeof(_bindnames)/sizeof(_bindnames[0]))


void M_Keys_Draw() {
    qPic_p p = Draw_CachePic("gfx/ttl_cstm.lmp");
    M_DrawPic((vid.width - p->width) / 2, 4, p);

    M_Print(12, 32, (_bind_grab) ?
        "Press a key or button for this action" :
        "Enter to change, backspace to clear"
    );

    // search for known bindings
    for (int i = 0; i < NUMCOMMANDS; i++) {
        int y = 48 + (8 * i);
        M_Print(16, y, _bindnames[i][description]);
        // int len = strlen (_bindnames[i][command]);

        int keys[2];
        M_FindKeysForCommand(_bindnames[i][command], keys);

        if (keys[0] == -1) {
            M_Print(140, y, "???");
        }
        else {
            cStringRO name = Key_KeynumToString(keys[0]);
            M_Print(140, y, name);
            int x = strlen(name) * 8;
            if (keys[1] != -1) {
                M_Print(140 + x + 8, y, "or");
                M_Print(140 + x + 32, y, Key_KeynumToString(keys[1]));
            }
        }
    }

    M_DrawCharacter(
        130, 48 + (_keys_cursor * 8),
        (_bind_grab) ?
        '=' : curSymb()
    );
}

void M_UnbindCommand(cString command) {
    int len = strlen(command);

    for (keycode_t j = 0; j < MAX_KEYS; j++) {
        if (!keyBindings[j])
            continue;
        if (!strncmp(keyBindings[j], command, len))
            Key_SetBinding(j, "");
    }
}

void M_Keys_Key(keycode_t k) {
    char cmd[80];
    int  keys[2];

    if (_bind_grab) { // defining a key
        S_LocalSound("misc/menu1.wav");
        if (k == K_ESCAPE) {
            _bind_grab = false;
        }
        else if (k != '`') {
            snprintf(
                cmd, sizeof(cmd),
                "bind \"%s\" \"%s\"\n",
                Key_KeynumToString(k),
                _bindnames[_keys_cursor][command]
            );
            Cbuf_InsertText(cmd);
        }

        _bind_grab = false;
        return;
    }

    switch (k) {
    case K_ESCAPE:  M_Menu_Options_f(); break;

    case K_LEFTARROW:
    case K_UPARROW:
        S_LocalSound("misc/menu1.wav");
        if (--_keys_cursor < 0)
            _keys_cursor = NUMCOMMANDS - 1;
        break;

    case K_DOWNARROW:
    case K_RIGHTARROW:
        S_LocalSound("misc/menu1.wav");
        if (++_keys_cursor >= NUMCOMMANDS)
            _keys_cursor = 0;
        break;

    case K_ENTER:  // go into bind mode
        M_FindKeysForCommand(_bindnames[_keys_cursor][command], keys);
        S_LocalSound("misc/menu2.wav");
        if (keys[1] != -1)
            M_UnbindCommand(_bindnames[_keys_cursor][command]);
        _bind_grab = true;
        break;

    case K_BACKSPACE:  // delete bindings
    case K_DEL:    // delete bindings
        S_LocalSound("misc/menu2.wav");
        M_UnbindCommand(_bindnames[_keys_cursor][command]);
        break;
    default: break;
    }
}

void M_Menu_Keys_f() {
    key.dest = key_menu;
    m_state = m_keys;
    m_entersound = true;
}

void M_FindKeysForCommand(cString command, int* twokeys) {
    twokeys[0] = twokeys[1] = -1;
    int len = strlen(command);
    int count = 0;

    for (int j = 0; j < MAX_KEYS; j++) {
        if (!keyBindings[j])
            continue;
        if (!strncmp(keyBindings[j], command, len)) {
            twokeys[count] = j;
            count++;
            if (count == 2)
                break;
        }
    }
}


