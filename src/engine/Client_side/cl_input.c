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
// #include <stdlib.h>
#include "console.h"
// #include "cmd.h"
#include "protocol.h"
#include "q_tools.h"
#include "angle.h"
#include "vector_tools.h"


/*
    ================
    CL_AdjustAngles

    Moves the local angle positions
    ================
*/
void CL_AdjustAngles() {
    LegDt_t speed = (kbIsDown(in.speed)) ?
        host_frametime * cl_anglespeedkey.value : host_frametime;

    if (!(kbIsDown(in.strafe))) {
        cl.viewangles.yaw = anglemod(
            cl.viewangles.yaw +
            speed * cl_yawspeed.value * (CL_KeyState(&in.left) - CL_KeyState(&in.right))
        );
    }

    float lUp = CL_KeyState(&in.lookup);
    float lDown = CL_KeyState(&in.lookdown);

    cl.viewangles.pitch += speed * cl_pitchspeed.value * (lDown - lUp);

    if (kbIsDown(in.klook)) {
        V_StopPitchDrift();
        cl.viewangles.pitch +=
            speed * cl_pitchspeed.value * (CL_KeyState(&in.back) - CL_KeyState(&in.forward)
        );
    }

    if (lUp || lDown)
        V_StopPitchDrift();

    ClampInRange(-70.f, &cl.viewangles.pitch, 80.f);    // down look
    ClampInRange(-50.f, &cl.viewangles.roll, 50.f);

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

    CL_AdjustAngles(); // Calc Aim

#if 0
    Q_memset(cmd, 0, sizeof(*cmd));

    if (kbIsDown(in.strafe)) {
        cmd->move.side += cl_sidespeed.value * (CL_KeyState(&in.right) - CL_KeyState(&in.left));
    }
    cmd->move.side += cl_sidespeed.value * (CL_KeyState(&in.moveright) - CL_KeyState(&in.moveleft));
    cmd->move.up += cl_upspeed.value * (CL_KeyState(&in.up) - CL_KeyState(&in.down));

    if (!(kbIsDown(in.klook))) {
        cmd->move.forward += cl_forwardspeed.value * (CL_KeyState(&in.forward) - CL_KeyState(&in.back));
    }
#else
    * cmd = (UserCmd_t){
        .move = {
            .forward = ((kbIsDown(in.klook))) ? 0.f :
                cl_forwardspeed.value * (CL_KeyState(&in.forward) - CL_KeyState(&in.back)),
            .side = cl_sidespeed.value * (
                (CL_KeyState(&in.moveright) - CL_KeyState(&in.moveleft) +
                    ((!kbIsDown(in.strafe)) ? 0.f :
                    CL_KeyState(&in.right) - CL_KeyState(&in.left))
                )),
            .up = cl_upspeed.value * (CL_KeyState(&in.up) - CL_KeyState(&in.down))
        }
    };
#endif
    //
    // adjust for speed key
    //
    if (kbIsDown(in.speed))
        cmd->move = VectorScale(cmd->move, cl_movespeedkey.value);

#ifdef QUAKE2
    cmd->lightlevel = cl.light_level;
#endif
}



/*
==============
CL_SendMove
==============
*/
void CL_SendMove(UserCmd_p cmd) {   /* <==> void CL_SendMove(UserCmd_p cmd) */
    static uint8_t data[128];
    sizebuf_t buf = {
        .maxsize = sizeof(data),
        // .cursize = 0,
        .data = data,
    };

    cl.cmd = *cmd;

    //
    // send the movement message
    //
    MSG_WriteByte(&buf, clc_move); {
        MSG_WriteFloat(&buf, (float)cl.mtime[Cur]); // so server can get ping times
        MSG_WriteAngles(&buf, cl.viewangles);
        MSG_WriteMoveVec(&buf, cmd->move);

        {   // send button bits
            uint8_t bits = 0x00;    // (bit0=attack, bit1=jump; cleared after send)
            if (kbWasPressed(in.attack))    bits |= (1 << 0); // bit0=attack
            kbClearImpulseDown(&in.attack);

            if (kbWasPressed(in.jump))      bits |= (1 << 1); // bit1=jump
            kbClearImpulseDown(&in.jump);

            MSG_WriteByte(&buf, bits);
        }
        MSG_WriteByte(&buf, in.impulse);    in.impulse = 0;

#ifdef QUAKE2
        //
        // light level
        //
        MSG_WriteByte(&buf, cmd->lightlevel);
#endif
    }
    //
    // deliver the message
    //
    if (cls.isDemoPlaying)   return;

    //
    // allways dump the first two message, because it may contain leftover inputs from the last level
    //
    if (++cl.movemessages <= 2) return;

    if (NET_SendUnreliableMessage(cls.netcon, &buf) == -1) {
        Con_Printf("CL_SendMove: lost server connection\n");
        CL_Disconnect();
    }
}


