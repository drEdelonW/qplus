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
// cl.input.c  -- builds an intended movement command to send to the server

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "client.h"
#include "host.h"
#include "msg.h"
#include "cvar_q1.h"
#include <stdlib.h>
#include "console.h"
#include "cmd.h"
#include "protocol.h"
#include "q_tools.h"
#include "angle.h"

ClInput_t in;


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

    state bit 0 is the current state of the key
    state bit 1 is edge triggered on the up to down transition
    state bit 2 is edge triggered on the down to up transition

    ===============================================================================
*/


void KeyDown(kbutton_p btn) {
    cString c = Cmd_Argv(1);
    int k = (c[0]) ? atoi(c) : -1;  // typed manually at the console for continuous down

    if ((k == btn->down[0]) ||
        (k == btn->down[1]))
        return;  // repeating key

    if (!btn->down[0])      btn->down[0] = k;
    else if (!btn->down[1]) btn->down[1] = k;
    else {
        Con_Printf("Three keys down for a button!\n");
        return;
    }

    if (btn->state & 1) return;  // still down
    btn->state |= 1 + 2; // down + impulse down
}

void KeyUp(kbutton_p btn) {
    int k;

    cString c = Cmd_Argv(1);
    if (c[0]) { k = atoi(c); }
    else { // typed manually at the console, assume for unsticking, so clear all
        btn->down[0] = btn->down[1] = 0;
        btn->state = 4; // impulse up
        return;
    }

    if (btn->down[0] == k)          btn->down[0] = 0;
    else if (btn->down[1] == k)     btn->down[1] = 0;
    else    return;  // key up without coresponding down (menu pass through)

    if ((btn->down[0] ||
        btn->down[1]) ||   // some other key is still holding it down
        (!(btn->state & 1)) // still up (this should not happen)
        ) {
        return;
    }
    btn->state &= ~1;  // now up
    btn->state |= 4;   // impulse up
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
float CL_KeyState(kbutton_p key) {
    bool down = key->state & 1;
    bool impulsedown = key->state & 2;
    bool impulseup = key->state & 4;
    float val = 0;

    if (impulsedown && !impulseup) {
        if (down)   val = 0.5; // pressed and held this frame
        else        val = 0; // I_Error ();
    }
    if (impulseup && !impulsedown) {
        if (down)   val = 0; // I_Error ();
        else        val = 0; // released this frame
    }
    if (!impulsedown && !impulseup) {
        if (down)   val = 1.0; // held the entire frame
        else        val = 0; // up the entire frame
    }
    if (impulsedown && impulseup) {
        if (down)   val = 0.75; // released and re-pressed this frame
        else        val = 0.25; // pressed and released this frame
    }

    key->state &= 1;  // clear impulses

    return val;
}


//==========================================================================



/*
    ================
    CL_AdjustAngles

    Moves the local angle positions
    ================
*/
void CL_AdjustAngles() {
    float speed;

    if (in.speed.state & 1) speed = host_frametime * cl_anglespeedkey.value;
    else                    speed = host_frametime;

    if (!(in.strafe.state & 1)) {
        cl.viewangles[YAW] -= speed * cl_yawspeed.value * CL_KeyState(&in.right);
        cl.viewangles[YAW] += speed * cl_yawspeed.value * CL_KeyState(&in.left);
        cl.viewangles[YAW] = anglemod(cl.viewangles[YAW]);
    }
    if (in.klook.state & 1) {
        V_StopPitchDrift();
        cl.viewangles[PITCH] -= speed * cl_pitchspeed.value * CL_KeyState(&in.forward);
        cl.viewangles[PITCH] += speed * cl_pitchspeed.value * CL_KeyState(&in.back);
    }

    float up = CL_KeyState(&in.lookup);
    float down = CL_KeyState(&in.lookdown);

    cl.viewangles[PITCH] -= speed * cl_pitchspeed.value * up;
    cl.viewangles[PITCH] += speed * cl_pitchspeed.value * down;

    if (up || down)
        V_StopPitchDrift();

    CLAMP(-70, cl.viewangles[PITCH], 80);    // down look
    CLAMP(-50, cl.viewangles[ROLL], 50);

}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove(UserCmd_p cmd) {
    if (cls.signon != SIGNONS)
        return;

    CL_AdjustAngles();

    Q_memset(cmd, 0, sizeof(*cmd));

    if (in.strafe.state & 1) {
        cmd->sidemove += cl_sidespeed.value * CL_KeyState(&in.right);
        cmd->sidemove -= cl_sidespeed.value * CL_KeyState(&in.left);
    }

    cmd->sidemove += cl_sidespeed.value * CL_KeyState(&in.moveright);
    cmd->sidemove -= cl_sidespeed.value * CL_KeyState(&in.moveleft);

    cmd->upmove += cl_upspeed.value * CL_KeyState(&in.up);
    cmd->upmove -= cl_upspeed.value * CL_KeyState(&in.down);

    if (!(in.klook.state & 1)) {
        cmd->forwardmove += cl_forwardspeed.value * CL_KeyState(&in.forward);
        cmd->forwardmove -= cl_backspeed.value * CL_KeyState(&in.back);
    }

    //
    // adjust for speed key
    //
    if (in.speed.state & 1) {
        cmd->forwardmove *= cl_movespeedkey.value;
        cmd->sidemove *= cl_movespeedkey.value;
        cmd->upmove *= cl_movespeedkey.value;
    }

#ifdef QUAKE2
    cmd->lightlevel = cl.light_level;
#endif
}



/*
==============
CL_SendMove
==============
*/
void CL_SendMove(UserCmd_p cmd) {
    uint8_t data[128];
    sizebuf_t buf = {
        .maxsize = 128,
        .cursize = 0,
        .data = data,
    };

    cl.cmd = *cmd;

    //
    // send the movement message
    //
    MSG_WriteByte(&buf, clc_move); MSG_WriteFloat(&buf, cl.mtime[0]); // so server can get ping times

    for (int i = 0; i < VECT_DIM; i++)
        MSG_WriteAngle(&buf, cl.viewangles[i]);

    MSG_WriteShort(&buf, cmd->forwardmove);
    MSG_WriteShort(&buf, cmd->sidemove);
    MSG_WriteShort(&buf, cmd->upmove);

    //
    // send button bits
    //
    int bits = 0;
    if (in.attack.state & 3)    bits |= 1;
    in.attack.state &= ~2;

    if (in.jump.state & 3)      bits |= 2;
    in.jump.state &= ~2;

    MSG_WriteByte(&buf, bits);

    MSG_WriteByte(&buf, in.impulse);
    in.impulse = 0;

#ifdef QUAKE2
    //
    // light level
    //
    MSG_WriteByte(&buf, cmd->lightlevel);
#endif

    //
    // deliver the message
    //
    if (cls.demoplayback)   return;

    //
    // allways dump the first two message, because it may contain leftover inputs
    // from the last level
    //
    if (++cl.movemessages <= 2) return;

    if (NET_SendUnreliableMessage(cls.netcon, &buf) == -1) {
        Con_Printf("CL_SendMove: lost server connection\n");
        CL_Disconnect();
    }
}


void IN_KLookDown() { KeyDown(&in.klook); }         void IN_KLookUp() { KeyUp(&in.klook); }
void IN_MLookDown() { KeyDown(&in.mlook); }         void IN_MLookUp() { KeyUp(&in.mlook); if (!(in.mlook.state & 1) && lookspring.value) V_StartPitchDrift(); }
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

