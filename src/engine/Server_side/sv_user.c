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

#include "server.h"
#include "input.h"
#include "msg.h"
#include "q_tools.h"
#include "cvar_q1.h"
#include "protocol.h"
#include "cmd.h"
#include "cbuf.h"
#include "world.h"
#include <string.h>
#include "console.h"
#include "view.h"
#include "host.h"
#include "mathlib.h"
#include "transform.h"
#include "progs.h"
#include "GlobVars.h"
#include "GameRule.h"
#include "vector_tools.h"
#include "trace.h"




static vec3_t _wishDir;
static float _wishSpeed;

// world
static vec3_p _origin;     // TODO: make ti vec3_t
static vec3_p _velocity;   // TODO: make ti vec3_t

static bool _onGround;

UserCmd_t cmd;


static edict_p sv_player;
edict_p SvPlayer() { return sv_player; }

#include "Player.h"
#define MAX_FORWARD 6
void SV_SetIdealPitch() {
    if (!SvPlayer_IsFlag(FL_ONGROUND))   return;

    float z[MAX_FORWARD];
    float angleval = DEG2RAD(SvPlayer_Angles().yaw);
    float sinval = sinf(angleval);
    float cosval = cosf(angleval);

    int i = 0;
    for (; i < MAX_FORWARD; i++) {
        vec3_t top = VectorAdd(SvPlayer_Origin(), VecXYZ(
            cosval * (float)((i + 3) * 12),
            sinval * (float)((i + 3) * 12),
            SvPlayer_ViewOffs().z
        ));

        trace_t tr = SV_MoveDLine(top, VecZ(-160.f), MOVE_NOMONSTERS, SvPlayer());
        if (tr.allsolid)        return; // looking at a wall, leave ideal the way is was
        if (tr.fraction == 1.f)   return; // near a dropoff

        z[i] = top.z + tr.fraction * (-160.f);

    }

    float dir = 0;
    float steps = 0;
    for (int j = 1; j < i; j++) {
        float step = z[j] - z[j - 1];
        if ((step > -ON_EPSILON) && (step < ON_EPSILON))    continue;

        if (dir && (
            ((step - dir) > ON_EPSILON) ||
            ((step - dir) < -ON_EPSILON))
            )   return;  // mixed changes

        steps++;
        dir = step;
    }

    if (!dir) {
        SvPlayer_SetIdealPitch(0.f);
        return;
    }

    if (steps < 2)  return;
    SvPlayer_SetIdealPitch(-dir * sv_idealpitchscale.value);
}




void SV_UserFriction() {
    vec3_p vel = _velocity;

    float speed = sqrtf((vel->x * vel->x) + (vel->y * vel->y));
    if (!speed) return;

    // if the leading edge is over a dropoff, increase friction 
    vec3_t start = *_origin; {
        VectorScale(*vel, 16.f / speed);
        start.z = _origin->z + SvPlayer_Mins().z;
    };

    trace_t trace = SV_MoveDLine(start, VecZ(-34.f), MOVE_NOMONSTERS, SvPlayer());

    float friction = (trace.fraction == 1.f) ?
        (sv_friction.value * sv_edgefriction.value) : sv_friction.value;

    // apply friction
    float control = (speed < sv_stopspeed.value) ?
        sv_stopspeed.value : speed;
    float newspeed = (float)(speed - host_frametime * control * friction);
    ClampLessThen(&newspeed, 0.f);
    newspeed /= speed;

    *vel = VectorScale(*vel, newspeed);
}

/*
==============
SV_Accelerate
==============
*/

void SV_Accelerate() {
    float currentspeed = DotProduct(*_velocity, _wishDir);
    float addspeed = _wishSpeed - currentspeed;
    if (addspeed <= 0.f)
        return;

    float accelspeed = (float)(sv_accelerate.value * host_frametime * _wishSpeed);
    ClampMoreThen(&accelspeed, addspeed);

    *_velocity = VectorMA(*_velocity, accelspeed, _wishDir);
}

void SV_AirAccelerate(vec3_t wishveloc) {
    float wishspd = VectorNormalize(&wishveloc);
    ClampMoreThen(&wishspd, 30.f);

    float currentspeed = DotProduct(*_velocity, wishveloc);
    float addspeed = wishspd - currentspeed;
    if (addspeed <= 0.f)
        return;

    // accelspeed = sv_accelerate.value * host_frametime;
    float accelspeed = (float)(sv_accelerate.value * _wishSpeed * host_frametime);
    ClampMoreThen(&accelspeed, addspeed);

    *_velocity = VectorMA(*_velocity, accelspeed, wishveloc);
}

static inline float _AngleLen(ang3_t a) { // local trick without physical meaning
    return sqrtf(a.pitch * a.pitch + a.yaw * a.yaw + a.roll * a.roll);
}
void DropPunchAngle(void) {
    float orig_len = _AngleLen(SvPlayer_PunchAngle());
    if (orig_len == 0.f) return;

    float new_len = orig_len - 10.f * (float)host_frametime;
    ClampLessThen(&new_len, 0.f);

    SvPlayer_SetPunchAngle(
        AngleScale(
            SvPlayer_PunchAngle(),
            new_len / orig_len
        )
    );
}
/*
===================
SV_WaterMove

===================
*/
#include "Player.h"
extern Basis_t _bs; // leave in view.c
void SV_WaterMove() {
    // user intentions
    _bs = GetBasis(SvPlayer_ViewAngle());

    vec3_t wishvel = VectorMA(VectorScale(_bs.forward, cmd.move.forward), cmd.move.side, _bs.right);

    float goDownVal = -60.f; // drift towards bottom
    wishvel.z += (VectorCompare(cmd.move, v3Zero)) ? goDownVal : cmd.move.up;

    _wishSpeed = Length(wishvel);
    if (_wishSpeed > sv_maxspeed.value) {
        wishvel = VectorScale(wishvel, sv_maxspeed.value / _wishSpeed);
        _wishSpeed = sv_maxspeed.value;
    }
    _wishSpeed *= 0.7f;

    // water friction
    float speed = Length(*_velocity);
    float newspeed;
    if (speed) {
        newspeed = (float)(speed - host_frametime * speed * sv_friction.value);
        ClampLessThen(&newspeed, 0.f);
        *_velocity = VectorScale(*_velocity, newspeed / speed);
    }
    else
        newspeed = 0.f;

    // water acceleration
    if (!_wishSpeed)     return;

    float addspeed = _wishSpeed - newspeed;
    if (addspeed <= 0.f)  return;

    VectorNormalize(&wishvel);
    float accelspeed = (float)(sv_accelerate.value * _wishSpeed * host_frametime);
    ClampMoreThen(&accelspeed, addspeed);

    *_velocity = VectorMA(*_velocity, accelspeed, wishvel);
}

void SV_WaterJump() {
    if (SvPlayer_TeleportTimeElapsed() ||
        !SvPlayer_WaterLevel()
        ) {
        SvPlayer_ClrFlag(FL_WATERJUMP);
        SvPlayer_SetTeleportTime(0.f);
    }
    SvPlayer_pVelocity()->x = SvPlayer_MoveDir().x;
    SvPlayer_pVelocity()->y = SvPlayer_MoveDir().y;
}



void SV_AirMove() {
    _bs = GetBasis(SvPlayer_ViewAngle());

    float fmove = cmd.move.forward;
    float smove = cmd.move.side;

    // hack to not let you back into teleporter
    if ((!SvPlayer_TeleportTimeElapsed()) &&
        (fmove < 0.f)
        )   fmove = 0.f;

    vec3_t wishvel = VectorMA(VectorScale(_bs.forward, fmove), smove, _bs.right);
    wishvel.z = (SvPlayer_MoveType() != MOVETYPE_WALK) ?
        cmd.move.up : 0.f;

    _wishDir = wishvel;
    _wishSpeed = VectorNormalize(&_wishDir);
    if (_wishSpeed > sv_maxspeed.value) {
        wishvel = VectorScale(wishvel, sv_maxspeed.value / _wishSpeed);
        _wishSpeed = sv_maxspeed.value;
    }

    if (SvPlayer_MoveType() == MOVETYPE_NOCLIP) // noclip
        *_velocity = wishvel;
    else if (_onGround) {
        SV_UserFriction();
        SV_Accelerate();
    }
    else // not on ground, so little effect on velocity
        SV_AirAccelerate(wishvel);
}

/*
===================
SV_ClientThink

the move fields specify an intended velocity in pix/sec
the angle fields specify an exact angular motion in degrees
===================
*/
void SV_ClientThink() {
    if (SvPlayer_MoveType() == MOVETYPE_NONE) return;

    _onGround = SvPlayer_IsFlag(FL_ONGROUND);

    _origin = SvPlayer_pOrigin();
    _velocity = SvPlayer_pVelocity();

    DropPunchAngle();

    // if dead, behave differently
    if (SvPlayer_IsDead())   return;

    // angles
    // show 1/3 the pitch angle and all the roll angle
    cmd = remoteClient->cmd;
    ang3_p _angles = SvPlayer_pAngles();

    ang3_t v_angle;
    v_angle = AngleAdd(SvPlayer_ViewAngle(), SvPlayer_PunchAngle());
    _angles->roll = V_CalcRoll(SvPlayer_Angles(), SvPlayer_Velocity()) * 4;
    if (!(SvPlayer_FixAngle())) {
        _angles->pitch = -v_angle.pitch / 3;
        _angles->yaw = v_angle.yaw;
    }

    if (SvPlayer_IsFlag(FL_WATERJUMP)) { SV_WaterJump(); return; }

    // walk
    if ((SvPlayer_WaterLevel() >= WL_Waist) &&
        (SvPlayer_MoveType() != MOVETYPE_NOCLIP)
        ) {
        SV_WaterMove();
        return;
    }

    SV_AirMove();
}



void SV_ReadClientMove(UserCmd_p move) {    /* <==> void CL_SendMove(UserCmd_p cmd) */
    // read ping time
    remoteClient->ping_times[remoteClient->num_pings % NUM_PING_TIMES] = (float)SV_GetTime() - MSG_ReadFloat();
    remoteClient->num_pings++;

    remoteClient->edict->v.v_angle = MSG_ReadAngles();  // read current angles
    move->move = MSG_ReadMoveVec();                     // read movement

    uint8_t bits = MSG_ReadByte();                      // read buttons
    remoteClient->edict->v.button0 = (float)((bits & (1 << 0)) >> 0);
    remoteClient->edict->v.button2 = (float)((bits & (1 << 1)) >> 1);

    uint8_t i = MSG_ReadByte();
    if (i)
        remoteClient->edict->v.impulse = (float)i;

#ifdef QUAKE2
    // read light level
    remoteClient->edict->v.light_level = MSG_ReadByte();
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
        ret = NET_GetMessage(remoteClient->netconnection);
        if (ret == -1) {
            Host_Printf("SV_ReadClientMessage: NET_GetMessage failed\n");
            return false;
        }
        if (!ret) return true;

        MSG_BeginReading();

        while (1) {
            if (!remoteClient->active) return false; // a command caused an error
            if (getMsgBadRead()) { Host_Printf("SV_ReadClientMessage: badread\n"); return false; }

            clc_t cmd = MSG_ReadChar();
            if (getMsgBadRead()) goto nextmsg;

            switch (cmd) {
                // case MSG_ERROR:
                //     goto nextmsg;  // end of message

            default: Host_Printf("SV_ReadClientMessage: unknown command char\n"); return false;

            case clc_nop: /* Host_Printf ("clc_nop\n"); */ break;

            case clc_stringcmd: {
                cString str = MSG_ReadString();
                if (remoteClient->privileged)   ret = 2;
                else                            ret = 0;

                if ((Q_strncasecmp(str, "status", 6)    /**/ == 0) ||
                    (Q_strncasecmp(str, "god", 3)       /**/ == 0) ||
                    (Q_strncasecmp(str, "notarget", 8)  /**/ == 0) ||
                    (Q_strncasecmp(str, "fly", 3)       /**/ == 0) ||
                    (Q_strncasecmp(str, "name", 4)      /**/ == 0) ||
                    (Q_strncasecmp(str, "noclip", 6)    /**/ == 0) ||
                    (Q_strncasecmp(str, "say", 3)       /**/ == 0) ||
                    (Q_strncasecmp(str, "say_team", 8)  /**/ == 0) ||
                    (Q_strncasecmp(str, "tell", 4)      /**/ == 0) ||
                    (Q_strncasecmp(str, "color", 5)     /**/ == 0) ||
                    (Q_strncasecmp(str, "kill", 4)      /**/ == 0) ||
                    (Q_strncasecmp(str, "pause", 5)     /**/ == 0) ||
                    (Q_strncasecmp(str, "spawn", 5)     /**/ == 0) ||
                    (Q_strncasecmp(str, "begin", 5)     /**/ == 0) ||
                    (Q_strncasecmp(str, "prespawn", 8)  /**/ == 0) ||
                    (Q_strncasecmp(str, "kick", 4)      /**/ == 0) ||
                    (Q_strncasecmp(str, "ping", 4)      /**/ == 0) ||
                    (Q_strncasecmp(str, "give", 4)      /**/ == 0) ||
                    (Q_strncasecmp(str, "ban", 3)       /**/ == 0))
                    ret = 1;

                /**/ if (ret == 2)  Cbuf_InsertText(str);
                else if (ret == 1)  Cmd_ExecuteString(str, src_client);
                else                Con_DPrintf("%s tried to %s\n", remoteClient->name, str);
            }    break;

            case clc_disconnect: /* Host_Printf ("SV_ReadClientMessage: client disconnected\n"); */
                return false;

            case clc_move: SV_ReadClientMove(&remoteClient->cmd); break;
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
    remoteClient = svs.clients;
    for (int i = 0; i < GetSvMaxClients(); i++, remoteClient++) {
        if (!remoteClient->active) continue;

        sv_player = remoteClient->edict;

        if (!SV_ReadClientMessage()) {
            SV_DropClient(false); // client misbehaved...
            continue;
        }

        if (!remoteClient->spawned) {   // clear client movement until a new packet is received
            memset(&remoteClient->cmd, 0, sizeof(remoteClient->cmd));
            continue;
        }

        // always pause in single player if in console or menus
        if (!sv.paused &&
            (
                (GetSvMaxClients() > 1) ||
                (key.dest == key_game))
            )   SV_ClientThink();
    }
}



/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f() {
    if (isCliCmd()) {
        Cmd_ForwardToServer();
        return;
    }
    if ((pGame()->deathmatch) &&
        !(remoteClient->privileged)
        )  return;

    SvPlayer_ToggleFlag(FL_GODMODE);
    SV_ClientPrintf("godmode %s\n",
        (SvPlayer_IsFlag(FL_GODMODE)) ?
        "ON" : "OFF"
    );
}


void Host_Notarget_f() {
    if (isCliCmd()) {
        Cmd_ForwardToServer();
        return;
    }
    if ((pGame()->deathmatch) &&
        !(remoteClient->privileged)
        )  return;

    SvPlayer_ToggleFlag(FL_NOTARGET);
    SV_ClientPrintf("notarget %s\n",
        (SvPlayer_IsFlag(FL_NOTARGET)) ?
        "ON" : "OFF"
    );
}

bool noclip_anglehack;

void Host_Noclip_f() {
    if (isCliCmd()) {
        Cmd_ForwardToServer();
        return;
    }
    if ((pGame()->deathmatch) &&
        !(remoteClient->privileged)
        )  return;

    if (SvPlayer_MoveType() == MOVETYPE_NOCLIP) {
        noclip_anglehack = false;
        SvPlayer_SetMoveType(MOVETYPE_WALK);
        SV_ClientPrintf("noclip OFF\n");
    }
    else {
        noclip_anglehack = true;
        SvPlayer_SetMoveType(MOVETYPE_NOCLIP);
        SV_ClientPrintf("noclip ON\n");
    }
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f() {
    if (isCliCmd()) {
        Cmd_ForwardToServer();
        return;
    }

    if ((pGame()->deathmatch) &&
        !(remoteClient->privileged)
        )  return;

    if (SvPlayer_MoveType() == MOVETYPE_FLY) {
        SvPlayer_SetMoveType(MOVETYPE_WALK);
        SV_ClientPrintf("flymode OFF\n");
    }
    else {
        SvPlayer_SetMoveType(MOVETYPE_FLY);
        SV_ClientPrintf("flymode ON\n");
    }
}

