#include "input.h"
#include "cmd.h"
#include <stdlib.h>
#include "console.h"
#include "cvar_q1.h"
#include "q_tools.h"
#include "client.h" // V_StartPitchDrift()

ClInput_t in;

void KeyDown(kbutton_p btn) {
    cString c = Cmd_Argv(1);
    keycode_t k = (c[0]) ? atoi(c) : -1;  // typed manually at the console for continuous down

    if ((k == btn->down[0]) ||
        (k == btn->down[1])
        )   return;  // repeating key

    /* */if (!btn->down[0]) btn->down[0] = k;
    else if (!btn->down[1]) btn->down[1] = k;
    else {
        Con_Printf("Three keys down for a button!\n");
        return;
    }

    if (kbIsDown(*btn)) return;
    btn->kState |= KbsDown | KbsImpulseDown;
}

void KeyUp(kbutton_p btn) {
    keycode_t k;

    cString c = Cmd_Argv(1);
    if (c[0]) { k = atoi(c); }
    else { // typed manually at the console, assume for unsticking, so clear all
        btn->down[0] = btn->down[1] = 0;
        btn->kState = KbsImpulseUp;
        return;
    }

    /* */if (btn->down[0] == k)     btn->down[0] = 0;
    else if (btn->down[1] == k)     btn->down[1] = 0;
    else    return;  // key up without coresponding down (menu pass through)

    if ((
        btn->down[0] ||
        btn->down[1]
        ) ||   // some other key is still holding it down
        (!(kbIsDown(*btn))) // still up (this should not happen)
        ) {
        return;
    }
    btn->kState &= ~KbsDown;  // now up
    btn->kState |= KbsImpulseUp;   // impulse up
}

/*
    ===============
    CL_KeyState

    Returns 0.25 if a key was pressed and released during the frame,
    0.5 if it was pressed and held
    0 if held then released, and
    1.0 if held for the entire time
    ===============
*/
#if 0
float CL_KeyState(kbutton_p key) {
    bool down = key->kState & 1;
    bool impulsedown = key->kState & 2;
    bool impulseup = key->kState & 4;
    float val = 0.f;
    // TODO: remake as array table
    if (impulsedown && !impulseup) {
        if (down)   val = 0.5f; // pressed and held this frame
        else        val = 0.0f; // I_Error();
    }
    if (impulseup && !impulsedown) {
        if (down)   val = 0.0f; // I_Error();
        else        val = 0.0f; // released this frame
    }
    if (!impulsedown && !impulseup) {
        if (down)   val = 1.0f; // held the entire frame
        else        val = 0.0f; // up the entire frame
    }
    if (impulsedown && impulseup) {
        if (down)   val = 0.75f; // released and re-pressed this frame
        else        val = 0.25f; // pressed and released this frame
    }

    key->kState &= 1;  // clear impulses

    return val;
}
#else
static const float kKeyStateTable[8] = {
    /* kState & 7 = down | impulsedown<<1 | impulseup<<2 */
    [0] = 0.00f,  // up,   no impulse         -> up entire frame
    [1] = 1.00f,  // down, no impulse         -> held entire frame
    [2] = 0.00f,  // up,   impulsedown only   -> I_Error() (should not happen)
    [3] = 0.50f,  // down, impulsedown only   -> pressed and held this frame
    [4] = 0.00f,  // up,   impulseup only     -> released this frame
    [5] = 0.00f,  // down, impulseup only     -> I_Error() (should not happen)
    [6] = 0.25f,  // up,   both impulses      -> pressed and released this frame
    [7] = 0.75f,  // down, both impulses      -> released and re-pressed this frame
};

float CL_KeyState(kbutton_p key) {
    float val = kKeyStateTable[key->kState & KbsMask];
    kbClearImpulses(key);
    return val;
}
#endif


//==========================================================================



void IN_KLookDown() { KeyDown(&in.klook); }         void IN_KLookUp() { KeyUp(&in.klook); }
void IN_MLookDown() { KeyDown(&in.mlook); }         void IN_MLookUp() { KeyUp(&in.mlook); if (!(kbIsDown(in.mlook)) && lookspring.value) V_StartPitchDrift(); }
void IN_UpDown() { KeyDown(&in.up); }               void IN_UpUp() { KeyUp(&in.up); }
void IN_DownDown() { KeyDown(&in.down); }           void IN_DownUp() { KeyUp(&in.down); }
void IN_LeftDown() { KeyDown(&in.left); }           void IN_LeftUp() { KeyUp(&in.left); }
void IN_RightDown() { KeyDown(&in.right); }         void IN_RightUp() { KeyUp(&in.right); }
void IN_ForwardDown() { KeyDown(&in.forward); }     void IN_ForwardUp() { KeyUp(&in.forward); }

void IN_BackDown() { KeyDown(&in.back); }           void IN_BackUp() { KeyUp(&in.back); }
void IN_LookupDown() { KeyDown(&in.lookup); }       void IN_LookupUp() { KeyUp(&in.lookup); }
void IN_LookdownDown() { KeyDown(&in.lookdown); }   void IN_LookdownUp() { KeyUp(&in.lookdown); }
void IN_MoveleftDown() { KeyDown(&in.moveleft); }   void IN_MoveleftUp() { KeyUp(&in.moveleft); }
void IN_MoverightDown() { KeyDown(&in.moveright); } void IN_MoverightUp() { KeyUp(&in.moveright); }
void IN_SpeedDown() { KeyDown(&in.speed); }         void IN_SpeedUp() { KeyUp(&in.speed); }
void IN_StrafeDown() { KeyDown(&in.strafe); }       void IN_StrafeUp() { KeyUp(&in.strafe); }
void IN_AttackDown() { KeyDown(&in.attack); }       void IN_AttackUp() { KeyUp(&in.attack); }
void IN_UseDown() { KeyDown(&in.use); }             void IN_UseUp() { KeyUp(&in.use); }
void IN_JumpDown() { KeyDown(&in.jump); }           void IN_JumpUp() { KeyUp(&in.jump); }
void IN_Impulse() { in.impulse = Q_atoi(Cmd_Argv(1)); }


/*
============
CL_InitInput
============
*/
void CL_InitInput() {
    Cmd_AddCommand("+moveup", IN_UpDown);               Cmd_AddCommand("-moveup", IN_UpUp);
    Cmd_AddCommand("+movedown", IN_DownDown);           Cmd_AddCommand("-movedown", IN_DownUp);
    Cmd_AddCommand("+left", IN_LeftDown);               Cmd_AddCommand("-left", IN_LeftUp);
    Cmd_AddCommand("+right", IN_RightDown);             Cmd_AddCommand("-right", IN_RightUp);
    Cmd_AddCommand("+forward", IN_ForwardDown);         Cmd_AddCommand("-forward", IN_ForwardUp);
    Cmd_AddCommand("+back", IN_BackDown);               Cmd_AddCommand("-back", IN_BackUp);
    Cmd_AddCommand("+lookup", IN_LookupDown);           Cmd_AddCommand("-lookup", IN_LookupUp);
    Cmd_AddCommand("+lookdown", IN_LookdownDown);       Cmd_AddCommand("-lookdown", IN_LookdownUp);
    Cmd_AddCommand("+strafe", IN_StrafeDown);           Cmd_AddCommand("-strafe", IN_StrafeUp);
    Cmd_AddCommand("+moveleft", IN_MoveleftDown);       Cmd_AddCommand("-moveleft", IN_MoveleftUp);
    Cmd_AddCommand("+moveright", IN_MoverightDown);     Cmd_AddCommand("-moveright", IN_MoverightUp);
    Cmd_AddCommand("+speed", IN_SpeedDown);             Cmd_AddCommand("-speed", IN_SpeedUp);
    Cmd_AddCommand("+attack", IN_AttackDown);           Cmd_AddCommand("-attack", IN_AttackUp);
    Cmd_AddCommand("+use", IN_UseDown);                 Cmd_AddCommand("-use", IN_UseUp);
    Cmd_AddCommand("+jump", IN_JumpDown);               Cmd_AddCommand("-jump", IN_JumpUp);
    Cmd_AddCommand("+klook", IN_KLookDown);             Cmd_AddCommand("-klook", IN_KLookUp);
    Cmd_AddCommand("+mlook", IN_MLookDown);             Cmd_AddCommand("-mlook", IN_MLookUp);
    Cmd_AddCommand("impulse", IN_Impulse);
}

