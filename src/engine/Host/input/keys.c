/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "keys.h"
#include <string.h>
#include "cmd.h"
#include "cbuf.h"
#include "console.h"
#include "client.h"
#include "cvar.h"
#include "q_tools.h"
#include "zone.h"
#include "menu.h"
#include "sys.h"
#include "screen.h"
#include <stdbool.h>
/*
key up events are sent even if in console mode
*/

#define MAXCMD      (1024)

Key_t key;
cString keyBindings[MAX_KEYS];

static int  _key_repeats[MAX_KEYS]; // if > 1, it is autorepeating
static int  _history_line = 0;
static bool _shift_down = false;
static bool _isConKeys[MAX_KEYS]; // if true, can't be rebound while in console
static bool _isMenuBound[MAX_KEYS]; // if true, can't be rebound while in menu
static bool _isKeyDown[MAX_KEYS];
static keycode_t _keyshift[MAX_KEYS];  // key to map to if shift held down in console

/*
==============================================================================

            LINE TYPING INTO THE CONSOLE

==============================================================================
*/

bool is_printable(keycode_t symb) {
    return
        (symb >= K_SPACE) &&
        (symb < K_BACKSPACE);
}

bool is_digits(keycode_t symb) {
    return
        (symb >= '0') &&
        (symb < '9');
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console(keycode_t key) {
    if (key == K_ENTER) {
        Cbuf_AddText(con.lines[edit_line] + 1);    // skip the >
        Cbuf_AddText("\n");
        Con_Printf("%s\n", con.lines[edit_line]);
        edit_line = (edit_line + 1) & 31;
        _history_line = edit_line;
        con.lines[edit_line][0] = ']';
        con.linepos = 1;
        if (cls.state == ca_disconnected)
            SCR_UpdateScreen(); // force an update, because the command
        // may take some time
        return;
    }

    if (key == K_TAB) { // command completion
        cString cmd = Cmd_CompleteCommand(con.lines[edit_line] + 1);
        if (!cmd)
            cmd = Cvar_CompleteVariable(con.lines[edit_line] + 1);

        if (cmd) {
            Q_strcpy(con.lines[edit_line] + 1, cmd);
            con.linepos = Q_strlen(cmd) + 1;
            con.lines[edit_line][con.linepos] = ' ';
            con.linepos++;
            con.lines[edit_line][con.linepos] = 0;
            return;
        }
    }

    if ((key == K_BACKSPACE) ||
        (key == K_LEFTARROW)) {
        if (con.linepos > 1)
            con.linepos--;
        return;
    }

    if (key == K_UPARROW) {
        do {
            _history_line = (_history_line - 1) & 31;
        } while ((_history_line != edit_line) &&
            (!con.lines[_history_line][1])
            );
        if (_history_line == edit_line)
            _history_line = (edit_line + 1) & 31;
        Q_strcpy(con.lines[edit_line], con.lines[_history_line]);
        con.linepos = Q_strlen(con.lines[edit_line]);
        return;
    }

    if (key == K_DOWNARROW) {
        if (_history_line == edit_line) return;
        do {
            _history_line = (_history_line + 1) & 31;
        } while ((_history_line != edit_line) &&
            (!con.lines[_history_line][1]));

        if (_history_line == edit_line) {
            con.lines[edit_line][0] = ']';
            con.linepos = 1;
        }
        else {
            Q_strcpy(con.lines[edit_line], con.lines[_history_line]);
            con.linepos = Q_strlen(con.lines[edit_line]);
        }
        return;
    }

    if ((key == K_PGUP) ||
        (key == K_MWHEELUP)
        ) {
        con.backscroll += 2;
        if (con.backscroll > (con.totallines - (vid.height >> 3) - 1))
            con.backscroll = con.totallines - (vid.height >> 3) - 1;
        return;
    }

    if ((key == K_PGDN) ||
        (key == K_MWHEELDOWN)
        ) {
        con.backscroll -= 2;
        if (con.backscroll < 0)
            con.backscroll = 0;
        return;
    }

    if (key == K_HOME) {
        con.backscroll = con.totallines - (vid.height >> 3) - 1;
        return;
    }

    if (key == K_END) {
        con.backscroll = 0;
        return;
    }

    if (!is_printable(key))
        return; // non printable

    if (con.linepos < (MAXCMDLINE - 1)) {
        con.lines[edit_line][con.linepos] = key;
        con.linepos++;
        con.lines[edit_line][con.linepos] = 0;
    }
}

//============================================================================
char chatBuffer[MAXCHATLEN];
bool team_message = false;

void Key_Message(keycode_t Key) {
    static int _chatBuffLen = 0;

    if (Key == K_ENTER) {
        if (team_message)   Cbuf_AddText("say_team \"");
        else                Cbuf_AddText("say \"");

        Cbuf_AddText(chatBuffer);
        Cbuf_AddText("\"\n");

        key.dest = key_game;
        _chatBuffLen = 0;
        chatBuffer[0] = 0;
        return;
    }

    if (Key == K_ESCAPE) {
        key.dest = key_game;
        _chatBuffLen = 0;
        chatBuffer[0] = 0;
        return;
    }

    if (!is_printable(Key))
        return; // non printable

    if (Key == K_BACKSPACE) {
        if (_chatBuffLen) {
            _chatBuffLen--;
            chatBuffer[_chatBuffLen] = 0;
        }
        return;
    }

    if (_chatBuffLen == 31)
        return; // all full

    chatBuffer[_chatBuffLen++] = Key;
    chatBuffer[_chatBuffLen] = 0;
}

//============================================================================



typedef struct {
    cString     name;
    keycode_t   keynum;
} keyname_t;
typedef keyname_t* keyname_p;

static keyname_t _keyNames[] = {
    {"TAB",         K_TAB},
    {"ENTER",       K_ENTER},
    {"ESCAPE",      K_ESCAPE},
    {"SPACE",       K_SPACE},
    {"BACKSPACE",   K_BACKSPACE},
    {"UPARROW",     K_UPARROW},
    {"DOWNARROW",   K_DOWNARROW},
    {"LEFTARROW",   K_LEFTARROW},
    {"RIGHTARROW",  K_RIGHTARROW},

    {"ALT",     K_ALT},
    {"CTRL",    K_CTRL},
    {"SHIFT",   K_SHIFT},

    {"F1",  K_F1},
    {"F2",  K_F2},
    {"F3",  K_F3},
    {"F4",  K_F4},
    {"F5",  K_F5},
    {"F6",  K_F6},
    {"F7",  K_F7},
    {"F8",  K_F8},
    {"F9",  K_F9},
    {"F10", K_F10},
    {"F11", K_F11},
    {"F12", K_F12},

    {"INS",     K_INS},
    {"DEL",     K_DEL},
    {"PGDN",    K_PGDN},
    {"PGUP",    K_PGUP},
    {"HOME",    K_HOME},
    {"END",     K_END},

    {"MOUSE1",  K_MOUSE1},
    {"MOUSE2",  K_MOUSE2},
    {"MOUSE3",  K_MOUSE3},

    {"JOY1",    K_JOY1},
    {"JOY2",    K_JOY2},
    {"JOY3",    K_JOY3},
    {"JOY4",    K_JOY4},

    {"AUX1",    K_AUX1},
    {"AUX2",    K_AUX2},
    {"AUX3",    K_AUX3},
    {"AUX4",    K_AUX4},
    {"AUX5",    K_AUX5},
    {"AUX6",    K_AUX6},
    {"AUX7",    K_AUX7},
    {"AUX8",    K_AUX8},
    {"AUX9",    K_AUX9},
    {"AUX10",   K_AUX10},
    {"AUX11",   K_AUX11},
    {"AUX12",   K_AUX12},
    {"AUX13",   K_AUX13},
    {"AUX14",   K_AUX14},
    {"AUX15",   K_AUX15},
    {"AUX16",   K_AUX16},
    {"AUX17",   K_AUX17},
    {"AUX18",   K_AUX18},
    {"AUX19",   K_AUX19},
    {"AUX20",   K_AUX20},
    {"AUX21",   K_AUX21},
    {"AUX22",   K_AUX22},
    {"AUX23",   K_AUX23},
    {"AUX24",   K_AUX24},
    {"AUX25",   K_AUX25},
    {"AUX26",   K_AUX26},
    {"AUX27",   K_AUX27},
    {"AUX28",   K_AUX28},
    {"AUX29",   K_AUX29},
    {"AUX30",   K_AUX30},
    {"AUX31",   K_AUX31},
    {"AUX32",   K_AUX32},

    {"PAUSE",   K_PAUSE},

    {"MWHEELUP",    K_MWHEELUP},
    {"MWHEELDOWN",  K_MWHEELDOWN},

    {"SEMICOLON", ';'}, // because a raw semicolon seperates commands

    {NULL, 0}
};

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keyBindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/

keycode_t Key_StringToKeynum(cStringRO str) {
    if ((!str) || (!str[0]))
        return -1;

    if (!str[1])
        return str[0];

    for (keyname_p kn = _keyNames; kn->name; kn++) {
        if (!Q_strcasecmp(str, kn->name))
            return kn->keynum;
    }
    return K_UNKNOWN;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
cStringRO Key_KeynumToString(keycode_t keynum) {
    static char tinystr[2];

    if (keynum == -1)
        return "<KEY NOT FOUND>";

    if (is_printable(keynum)) { // printable ascii
        tinystr[0] = keynum;
        tinystr[1] = 0;
        return tinystr;
    }

    for (keyname_p kn = _keyNames; kn->name; kn++) {
        if (keynum == kn->keynum)
            return kn->name;
    }

    return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding(keycode_t keynum, cString binding) {
    if (keynum == -1)
        return;

    // free old bindings
    if (keyBindings[keynum]) {
        Z_Free(keyBindings[keynum]);
        keyBindings[keynum] = NULL;
    }

    // allocate memory for new binding
    int len = Q_strlen(binding);
    cString new = Z_Malloc(len + 1);
    Q_strcpy(new, binding);
    new[len] = 0;
    keyBindings[keynum] = new;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f() {
    if (Cmd_Argc() != 2) {
        Con_Printf("unbind <key> : remove commands from a key\n");
        return;
    }

    keycode_t b = Key_StringToKeynum(Cmd_Argv(1));
    if (b == K_UNKNOWN) {
        Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
        return;
    }

    Key_SetBinding(b, "");
}

void Key_Unbindall_f() {
    for (keycode_t i = 0; i < MAX_KEYS; i++)
        if (keyBindings[i])
            Key_SetBinding(i, "");
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f() {
    int argcnt = Cmd_Argc();
    if ((argcnt != 2) &&
        (argcnt != 3)
        ) {
        Con_Printf("bind <key> [command] : attach a command to a key\n");
        return;
    }

    keycode_t btn = Key_StringToKeynum(Cmd_Argv(1));
    if (btn == K_UNKNOWN) {
        Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
        return;
    }

    if (argcnt == 2) {
        if (keyBindings[btn]) {
            Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), keyBindings[btn]);
        }
        else {
            Con_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
        }
        return;
    }

    // copy the rest of the command line
    char cmd[MAXCMD] = { 0 }; // start out with a null string
    for (int i = 2; i < argcnt; i++) {
        if (i > 2)
            strcat(cmd, " ");
        strcat(cmd, Cmd_Argv(i));
    }

    Key_SetBinding(btn, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings(FILE* f) {
    for (keycode_t i = 0; i < MAX_KEYS; i++) {
        if ((keyBindings[i]) &&
            (*keyBindings[i])
            ) {
            fprintf(f,
                "bind \"%s\" \"%s\"\n",
                Key_KeynumToString(i),
                keyBindings[i]
            );
        }
    }
}


/*
===================
Key_Init
===================
*/
void Key_Init() {
    for (int i = 0; i < 32; i++) {
        con.lines[i][0] = ']';
        con.lines[i][1] = 0;
    }
    con.linepos = 1;

    //
    // init ascii characters in console mode
    //
    for (int i = 32; i < 128; i++) {
        _isConKeys[i] = true;
    }

    _isConKeys[K_ENTER] = true;
    _isConKeys[K_TAB] = true;
    _isConKeys[K_LEFTARROW] = true;
    _isConKeys[K_RIGHTARROW] = true;
    _isConKeys[K_UPARROW] = true;
    _isConKeys[K_DOWNARROW] = true;
    _isConKeys[K_BACKSPACE] = true;
    _isConKeys[K_PGUP] = true;
    _isConKeys[K_PGDN] = true;
    _isConKeys[K_SHIFT] = true;
    _isConKeys[K_MWHEELUP] = true;
    _isConKeys[K_MWHEELDOWN] = true;
    _isConKeys['`'] = false;
    _isConKeys['~'] = false;

    for (keycode_t i = 0; i < MAX_KEYS; i++) {
        _keyshift[i] = i;
    }

    for (keycode_t i = 'a'; i <= 'z'; i++) {
        _keyshift[i] = i - 'a' + 'A';
    }

    _keyshift['1'] = '!';
    _keyshift['2'] = '@';
    _keyshift['3'] = '#';
    _keyshift['4'] = '$';
    _keyshift['5'] = '%';
    _keyshift['6'] = '^';
    _keyshift['7'] = '&';
    _keyshift['8'] = '*';
    _keyshift['9'] = '(';
    _keyshift['0'] = ')';
    _keyshift['-'] = '_';
    _keyshift['='] = '+';
    _keyshift[','] = '<';
    _keyshift['.'] = '>';
    _keyshift['/'] = '?';
    _keyshift[';'] = ':';
    _keyshift['\''] = '"';
    _keyshift['['] = '{';
    _keyshift[']'] = '}';
    _keyshift['`'] = '~';
    _keyshift['\\'] = '|';

    _isMenuBound[K_ESCAPE] = true;
    for (int i = 0; i < 12; i++) {
        _isMenuBound[K_F1 + i] = true;
    }

    //
    // register our functions
    //
    Cmd_AddCommand("bind", Key_Bind_f);
    Cmd_AddCommand("unbind", Key_Unbind_f);
    Cmd_AddCommand("unbindall", Key_Unbindall_f);

}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event(keycode_t Key, bool down) {
    _isKeyDown[Key] = down;

    if (!down)
        _key_repeats[Key] = 0;

    key.lastpress = Key;
    key.count++;
    if (key.count <= 0)
        return;  // just catching keys for Con_NotifyBox

    // update auto-repeat status
    if (down) {
        _key_repeats[Key]++;
        if ((Key != K_BACKSPACE) &&
            (Key != K_PAUSE) &&
            (_key_repeats[Key] > 1)
            ) {
            return; // ignore most autorepeats
        }

        if ((Key >= 200) &&
            !keyBindings[Key]
            ) {
            Con_Printf(
                "%s is unbound, hit F4 to set.\n",
                Key_KeynumToString(Key)
            );
        }
    }

    if (Key == K_SHIFT)
        _shift_down = down;


    //
    // handle escape specialy, so the user can never unbind it
    //
    if (Key == K_ESCAPE) {
        if (!down)  return;

        switch (key.dest) {
        case key_message:   Key_Message(Key);   break;
        case key_menu:      M_Keydown(Key);     break;
        case key_game:
        case key_console:   M_ToggleMenu_f();   break;
        default:            Sys_Error("Bad key.dest");
        }
        return;
    }

    //
    // key up events only generate commands if the game key binding is
    // a button command (leading + sign).  These will occur even in console mode,
    // to keep the character from continuing an action started before a console
    // switch.  Button commands include the kenum as a parameter, so multiple
    // downs can be matched with ups
    //
    if (!down) {
        cString kb = keyBindings[Key];
        if (kb && (kb[0] == '+')
            ) {
            char cmd[MAXCMD];   sprintf(cmd, "-%s %i\n", kb + 1, Key);
            Cbuf_AddText(cmd);
        }

        if (_keyshift[Key] != Key) {
            if ((unsigned)Key < MAX_KEYS
                ) {
                keycode_t sh = _keyshift[Key];
                if ((unsigned)sh < MAX_KEYS) {
                    kb = keyBindings[(unsigned)sh];
                }
                else { kb = NULL; }
            }
            else { kb = NULL; }

            if (kb && (kb[0] == '+')
                ) {
                char cmd[MAXCMD];   sprintf(cmd, "-%s %i\n", kb + 1, Key);
                Cbuf_AddText(cmd);
            }
        }
        return;
    }

    //
    // during demo playback, most keys bring up the main menu
    //
    if (cls.demoplayback && down &&
        _isConKeys[Key] && (key.dest == key_game)
        ) {
        M_ToggleMenu_f();
        return;
    }

    //
    // if not a consolekey, send to the interpreter no matter what mode is
    //
    if (
        ((key.dest == key_menu) && _isMenuBound[Key]) ||
        ((key.dest == key_console) && !_isConKeys[Key]) ||
        ((key.dest == key_game) && (!con.forcedup ||
            !_isConKeys[Key]))
        ) {
        cString kb = keyBindings[Key];
        if (kb) {
            if (kb[0] == '+') { // button commands add keynum as a parm
                char cmd[MAXCMD];   sprintf(cmd, "%s %i\n", kb, Key);
                Cbuf_AddText(cmd);
            }
            else {
                Cbuf_AddText(kb);
                Cbuf_AddText("\n");
            }
        }
        return;
    }

    if (!down)
        return;  // other systems only care about key down events

    if (_shift_down)
        Key = _keyshift[Key];


    switch (key.dest) {
    case key_message:   Key_Message(Key);  break;
    case key_menu:      M_Keydown(Key);    break;
    case key_game:
    case key_console:   Key_Console(Key);  break;
    default:            Sys_Error("Bad key.dest");
    }
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates() {
    for (keycode_t i = 0; i < MAX_KEYS; i++) {
        _isKeyDown[i] = false;
        _key_repeats[i] = 0;
    }
}

