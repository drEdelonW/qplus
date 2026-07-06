#pragma once
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
// input.h -- external (non-keyboard) input devices
// #include "client.h" // UserCmd_p
#include "UserCmd.h"
#include "keys.h"

/*
    ===============================================================================

    KEY BUTTONS

    Continuous button event tracking is complicated by the fact that two different
    input sources (say, mouse button 1 and the control key) can both press the
    same button, but the button should only be released when both of the
    pressing key have been released.

    When a key event issues a button command (+forward, +attack, etc), it appends
    its key number as a parameter to the command so it can be matched up with
    the release.

    kState bit 0 is the current kState of the key
    kState bit 1 is edge triggered on the up to down transition
    kState bit 2 is edge triggered on the down to up transition

    ===============================================================================
*/

typedef enum {
    KbsDown        = 1 << 0,   // current physical state (bit0)
    KbsImpulseDown = 1 << 1,   // edge: up->down transition this frame (bit1)
    KbsImpulseUp   = 1 << 2,   // edge: down->up transition this frame (bit2)
    KbsMask        = KbsDown | KbsImpulseDown | KbsImpulseUp
} KeyBtnState_t;

// cl_input
typedef struct {
    keycode_t       down[2];    // key nums holding it down
    KeyBtnState_t   kState;     // low bit is down state // TODO: make bit state helpers
} kbutton_t;
typedef kbutton_t* kbutton_p;

static inline bool kbIsDown(kbutton_t b)        { return b.kState & KbsDown; }
static inline bool kbWentDown(kbutton_t b)      { return b.kState & KbsImpulseDown; }
static inline bool kbWentUp(kbutton_t b)        { return b.kState & KbsImpulseUp; }
static inline bool kbWasPressed(kbutton_t b)    { return kbIsDown(b) || kbWentDown(b); }
static inline void kbClearImpulses(kbutton_p b)     { b->kState = (KeyBtnState_t)(b->kState & KbsDown); }
static inline void kbClearImpulseDown(kbutton_p b)  { b->kState = (KeyBtnState_t)(b->kState & ~KbsImpulseDown); }

typedef struct {
    kbutton_t   up;     // Aim Up
    kbutton_t   down;   // Aim Down
    kbutton_t   left;   // Aim Left
    kbutton_t   right;  // Aim Right

    kbutton_t   lookup; 
    kbutton_t   lookdown;

    kbutton_t   forward;// Move Forward
    // kbutton_t   forward2;// not used
    kbutton_t   moveleft;
    kbutton_t   moveright;
    kbutton_t   back;   // Move Back

    kbutton_t   strafe;
    kbutton_t   speed;
    kbutton_t   attack;
    kbutton_t   use;
    kbutton_t   jump;
    kbutton_t   mlook;  // Mouse look
    kbutton_t   klook;  // Keyboard Look
    uint8_t     impulse;
} ClInput_t;
extern ClInput_t in;


extern void (*vid_menukeyfn)(keycode_t key);

#ifdef __cplusplus
extern "C" {
#endif

    void IN_Init();
    void IN_Shutdown();
    void IN_Commands();             // opportunity for devices to stick commands on the script buffer
    void IN_Move(UserCmd_p cmd);    // add additional movement on top of the keyboard move cmd
    void IN_ClearStates();          // restores all button and position states to defaults

#ifdef __cplusplus
}
#endif
