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
// sv_user.c -- server code for moving users

#include "sys.h"
#include "msg.h"
#include "q_tools.h"
#include "cvar_q1.h"
#include "protocol.h"
#include "server.h"
#include "cmd.h"
#include "world.h"
#include <string.h>
#include "console.h"
#include "view.h"
#include "host.h"

edict_p sv_player;

static vec3_t  _forward, _right, _up;

vec3_t wishdir;
float wishspeed;

// world
float_p angles;
float_p origin;
float_p velocity;

bool onground;

UserCmd_t cmd;


/*
===============
SV_SetIdealPitch
===============
*/
#define MAX_FORWARD 6
void SV_SetIdealPitch() {
    if (!((int)sv_player->v.flags & FL_ONGROUND))   return;

    float z[MAX_FORWARD];
    float angleval = sv_player->v.angles[YAW] * M_PI * 2 / 360;
    float sinval = sin(angleval);
    float cosval = cos(angleval);

    int i = 0;
    for (; i < MAX_FORWARD; i++) {
        vec3_t top = {
            sv_player->v.origin[0] + cosval * (i + 3) * 12,
            sv_player->v.origin[1] + sinval * (i + 3) * 12,
            sv_player->v.origin[2] + sv_player->v.view_ofs[2]
        };

        vec3_t bottom = {
            bottom[0] = top[0],
            bottom[1] = top[1],
            bottom[2] = top[2] - 160
        };

        trace_t tr = SV_Move(top, vec3_origin, vec3_origin, bottom, 1, sv_player);
        if (tr.allsolid)        return; // looking at a wall, leave ideal the way is was
        if (tr.fraction == 1)   return; // near a dropoff

        z[i] = top[2] + tr.fraction * (bottom[2] - top[2]);
    }

    int dir = 0;
    int steps = 0;
    for (int j = 1; j < i; j++) {
        int step = z[j] - z[j - 1];
        if ((step > -ON_EPSILON) && (step < ON_EPSILON))    continue;

        if (dir && (((step - dir) > ON_EPSILON) || ((step - dir) < -ON_EPSILON))) return;  // mixed changes

        steps++;
        dir = step;
    }

    if (!dir) {
        sv_player->v.idealpitch = 0;
        return;
    }

    if (steps < 2)  return;
    sv_player->v.idealpitch = -dir * sv_idealpitchscale.value;
}


/*
==================
SV_UserFriction

==================
*/
void SV_UserFriction() {
    vec3_t start, stop;
    float friction;

    float_p vel = velocity;

    float speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
    if (!speed) return;

    // if the leading edge is over a dropoff, increase friction
    start[0] = stop[0] = origin[0] + vel[0] / speed * 16;
    start[1] = stop[1] = origin[1] + vel[1] / speed * 16;
    start[2] = origin[2] + sv_player->v.mins[2];
    stop[2] = start[2] - 34;

    trace_t trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, sv_player);

    if (trace.fraction == 1.0)
        friction = sv_friction.value * sv_edgefriction.value;
    else
        friction = sv_friction.value;

    // apply friction
    float control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
    float newspeed = speed - host_frametime * control * friction;

    if (newspeed < 0)
        newspeed = 0;
    newspeed /= speed;

    vel[0] = vel[0] * newspeed;
    vel[1] = vel[1] * newspeed;
    vel[2] = vel[2] * newspeed;
}

/*
==============
SV_Accelerate
==============
*/

#if 0
void SV_Accelerate(vec3_t wishvel) {
    if (wishspeed == 0) return;

    vec3_t  pushvec;
    VectorSubtract(wishvel, velocity, pushvec);
    float addspeed = VectorNormalize(pushvec);

    float accelspeed = sv_accelerate.value * host_frametime * addspeed;
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    for (int i = 0; i < VECT_DIM; i++)
        velocity[i] += accelspeed * pushvec[i];
}
#endif
void SV_Accelerate() {
    float currentspeed = DotProduct(velocity, wishdir);
    float addspeed = wishspeed - currentspeed;
    if (addspeed <= 0)
        return;

    float accelspeed = sv_accelerate.value * host_frametime * wishspeed;
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    for (int i = 0; i < VECT_DIM; i++)
        velocity[i] += accelspeed * wishdir[i];
}

void SV_AirAccelerate(vec3_t wishveloc) {
    float wishspd = VectorNormalize(wishveloc);
    if (wishspd > 30)
        wishspd = 30;

    float currentspeed = DotProduct(velocity, wishveloc);
    float addspeed = wishspd - currentspeed;
    if (addspeed <= 0)
        return;

    // accelspeed = sv_accelerate.value * host_frametime;
    float accelspeed = sv_accelerate.value * wishspeed * host_frametime;
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    for (int i = 0; i < VECT_DIM; i++)
        velocity[i] += accelspeed * wishveloc[i];
}


void DropPunchAngle() {
    float len = VectorNormalize(sv_player->v.punchangle);

    len -= 10 * host_frametime;
    if (len < 0)
        len = 0;
    VectorScale(sv_player->v.punchangle, len, sv_player->v.punchangle);
}

/*
===================
SV_WaterMove

===================
*/
void SV_WaterMove() {
    // user intentions
    AngleVectors(sv_player->v.v_angle, _forward, _right, _up);

    vec3_t wishvel;
    for (int i = 0; i < VECT_DIM; i++)
        wishvel[i] = _forward[i] * cmd.forwardmove + _right[i] * cmd.sidemove;

    if (!cmd.forwardmove &&
        !cmd.sidemove &&
        !cmd.upmove)
        wishvel[2] -= 60;  // drift towards bottom
    else
        wishvel[2] += cmd.upmove;

    wishspeed = Length(wishvel);
    if (wishspeed > sv_maxspeed.value) {
        VectorScale(wishvel, sv_maxspeed.value / wishspeed, wishvel);
        wishspeed = sv_maxspeed.value;
    }
    wishspeed *= 0.7;

    // water friction
    float speed = Length(velocity);
    float newspeed;
    if (speed) {
        newspeed = speed - host_frametime * speed * sv_friction.value;
        if (newspeed < 0)
            newspeed = 0;
        VectorScale(velocity, newspeed / speed, velocity);
    }
    else
        newspeed = 0;

    // water acceleration
    if (!wishspeed)     return;

    float addspeed = wishspeed - newspeed;
    if (addspeed <= 0)  return;

    VectorNormalize(wishvel);
    float accelspeed = sv_accelerate.value * wishspeed * host_frametime;
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    for (int i = 0; i < VECT_DIM; i++)
        velocity[i] += accelspeed * wishvel[i];
}

void SV_WaterJump() {
    if (sv.time > sv_player->v.teleport_time ||
        !sv_player->v.waterlevel
        ) {
        sv_player->v.flags = (int)sv_player->v.flags & ~FL_WATERJUMP;
        sv_player->v.teleport_time = 0;
    }
    sv_player->v.velocity[0] = sv_player->v.movedir[0];
    sv_player->v.velocity[1] = sv_player->v.movedir[1];
}


/*
===================
SV_AirMove

===================
*/
void SV_AirMove() {
    AngleVectors(sv_player->v.angles, _forward, _right, _up);

    float fmove = cmd.forwardmove;
    float smove = cmd.sidemove;

    // hack to not let you back into teleporter
    if (sv.time < sv_player->v.teleport_time && fmove < 0)
        fmove = 0;

    vec3_t  wishvel;
    for (int i = 0; i < VECT_DIM; i++)
        wishvel[i] = _forward[i] * fmove + _right[i] * smove;

    if ((int)sv_player->v.movetype != MOVETYPE_WALK)
        wishvel[2] = cmd.upmove;
    else
        wishvel[2] = 0;

    VectorCopy(wishvel, wishdir);
    wishspeed = VectorNormalize(wishdir);
    if (wishspeed > sv_maxspeed.value) {
        VectorScale(wishvel, sv_maxspeed.value / wishspeed, wishvel);
        wishspeed = sv_maxspeed.value;
    }

    if (sv_player->v.movetype == MOVETYPE_NOCLIP) { // noclip
        VectorCopy(wishvel, velocity);
    }
    else if (onground) {
        SV_UserFriction();
        SV_Accelerate();
    }
    else { // not on ground, so little effect on velocity
        SV_AirAccelerate(wishvel);
    }
}

/*
===================
SV_ClientThink

the move fields specify an intended velocity in pix/sec
the angle fields specify an exact angular motion in degrees
===================
*/
void SV_ClientThink() {
    if (sv_player->v.movetype == MOVETYPE_NONE) return;

    onground = (int)sv_player->v.flags & FL_ONGROUND;

    origin = sv_player->v.origin;
    velocity = sv_player->v.velocity;

    DropPunchAngle();

    // if dead, behave differently
    if (sv_player->v.health <= 0)   return;

    // angles
    // show 1/3 the pitch angle and all the roll angle
    cmd = host_client->cmd;
    angles = sv_player->v.angles;

    vec3_t v_angle;
    VectorAdd(sv_player->v.v_angle, sv_player->v.punchangle, v_angle);
    angles[ROLL] = V_CalcRoll(sv_player->v.angles, sv_player->v.velocity) * 4;
    if (!sv_player->v.fixangle) {
        angles[PITCH] = -v_angle[PITCH] / 3;
        angles[YAW] = v_angle[YAW];
    }

    if ((int)sv_player->v.flags & FL_WATERJUMP) { SV_WaterJump(); return; }

    // walk
    if ((sv_player->v.waterlevel >= 2)
        && (sv_player->v.movetype != MOVETYPE_NOCLIP)) {
        SV_WaterMove();
        return;
    }

    SV_AirMove();
}


/*
===================
SV_ReadClientMove
===================
*/
void SV_ReadClientMove(UserCmd_p move) {
    // read ping time
    host_client->ping_times[host_client->num_pings % NUM_PING_TIMES]
        = sv.time - MSG_ReadFloat();
    host_client->num_pings++;

    // read current angles
    // for (int i = 0; i < VECT_DIM; i++)
    //     angle[i] = MSG_ReadAngle();
    vec3_t angle = {
        MSG_ReadAngle(),
        MSG_ReadAngle(),
        MSG_ReadAngle()
    };

    VectorCopy(angle, host_client->edict->v.v_angle);

    // read movement
    move->forwardmove = MSG_ReadShort();
    move->sidemove = MSG_ReadShort();
    move->upmove = MSG_ReadShort();

    // read buttons
    int bits = MSG_ReadByte();
    host_client->edict->v.button0 = bits & 1;
    host_client->edict->v.button2 = (bits & 2) >> 1;

    int i = MSG_ReadByte();
    if (i)
        host_client->edict->v.impulse = i;

#ifdef QUAKE2
    // read light level
    host_client->edict->v.light_level = MSG_ReadByte();
#endif
}

/*
===================
SV_ReadClientMessage

Returns false if the client should be killed
===================
*/
bool SV_ReadClientMessage() {
    int  ret;
    do {
    nextmsg:
        ret = NET_GetMessage(host_client->netconnection);
        if (ret == -1) {
            Sys_Printf("SV_ReadClientMessage: NET_GetMessage failed\n");
            return false;
        }
        if (!ret) return true;

        MSG_BeginReading();

        while (1) {
            if (!host_client->active) return false; // a command caused an error
            if (msg_badread) { Sys_Printf("SV_ReadClientMessage: badread\n"); return false; }

            clc_t cmd = MSG_ReadChar();
            if (msg_badread) goto nextmsg;

            switch (cmd) {
                // case MSG_ERROR:
                //     goto nextmsg;  // end of message

            default: Sys_Printf("SV_ReadClientMessage: unknown command char\n"); return false;

            case clc_nop:
                //    Sys_Printf ("clc_nop\n");
                break;

            case clc_stringcmd:
                cString str = MSG_ReadString();
                if (host_client->privileged)    ret = 2;
                else                            ret = 0;

                if ((Q_strncasecmp(str, "status", 6) == 0) ||
                    (Q_strncasecmp(str, "god", 3) == 0) ||
                    (Q_strncasecmp(str, "notarget", 8) == 0) ||
                    (Q_strncasecmp(str, "fly", 3) == 0) ||
                    (Q_strncasecmp(str, "name", 4) == 0) ||
                    (Q_strncasecmp(str, "noclip", 6) == 0) ||
                    (Q_strncasecmp(str, "say", 3) == 0) ||
                    (Q_strncasecmp(str, "say_team", 8) == 0) ||
                    (Q_strncasecmp(str, "tell", 4) == 0) ||
                    (Q_strncasecmp(str, "color", 5) == 0) ||
                    (Q_strncasecmp(str, "kill", 4) == 0) ||
                    (Q_strncasecmp(str, "pause", 5) == 0) ||
                    (Q_strncasecmp(str, "spawn", 5) == 0) ||
                    (Q_strncasecmp(str, "begin", 5) == 0) ||
                    (Q_strncasecmp(str, "prespawn", 8) == 0) ||
                    (Q_strncasecmp(str, "kick", 4) == 0) ||
                    (Q_strncasecmp(str, "ping", 4) == 0) ||
                    (Q_strncasecmp(str, "give", 4) == 0) ||
                    (Q_strncasecmp(str, "ban", 3) == 0))
                    ret = 1;

                if (ret == 2)       Cbuf_InsertText(str);
                else if (ret == 1)  Cmd_ExecuteString(str, src_client);
                else                Con_DPrintf("%s tried to %s\n", host_client->name, str);
                break;

            case clc_disconnect:
                //    Sys_Printf ("SV_ReadClientMessage: client disconnected\n");
                return false;

            case clc_move: SV_ReadClientMove(&host_client->cmd); break;
            }
        }
    } while (ret == 1);

    return true;
}


/*
==================
SV_RunClients
==================
*/
void SV_RunClients() {
    host_client = svs.clients;
    for (int i = 0; i < svs.maxclients; i++, host_client++) {
        if (!host_client->active) continue;

        sv_player = host_client->edict;

        if (!SV_ReadClientMessage()) {
            SV_DropClient(false); // client misbehaved...
            continue;
        }

        if (!host_client->spawned) {
            // clear client movement until a new packet is received
            memset(&host_client->cmd, 0, sizeof(host_client->cmd));
            continue;
        }

        // always pause in single player if in console or menus
        if (!sv.paused &&
            ((svs.maxclients > 1) ||
                (key_dest == key_game)
                )
            )
            SV_ClientThink();
    }
}

