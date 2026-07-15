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
// cl_main.c  -- client main loop

#include "client.h"
#include "mem_placement.h"
#include "host.h"
#include <string.h>
#include "console.h"
#include <stdlib.h>
#include "screen.h"
#include "cmd.h"
#include "cbuf.h"
#include "angle.h"
#include "cvar_q1.h"
#include "Light.h"
#include "Beam.h"
#include "eFrag.h"
#include "render.h"
#include "VA.h"
#include "vector_tools.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

ClientState_t cl;
ClientStatic_t cls;
// FIXME: put these on hunk?
static efrag_t  cl_efrags[MAX_EFRAGS] PLACE_TO_SDRAM;
r_Entity_t      cl_entities[EdictMax] PLACE_TO_SDRAM;
r_Entity_t      cl_static_entities[MAX_STATIC_ENTITIES] PLACE_TO_SDRAM;
LightStyle_t    cl_lightstyle[MAX_LIGHTSTYLES] PLACE_TO_SDRAM;

uint32_t        cl_numvisedicts;
r_Entity_p      cl_visedicts[MAX_VISEDICTS] PLACE_TO_SDRAM;

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState() {
    if (!Host_IsServerActive())
        Host_ClearMemory();

    // wipe the entire cl structure
    memset(&cl, 0, sizeof(cl));

    SZ_Clear(&cls.message);

    // clear other arrays
    memset(cl_efrags, 0, sizeof(cl_efrags));
    memset(cl_entities, 0, sizeof(cl_entities));
    memset(cl_dlights, 0, sizeof(cl_dlights));
    memset(cl_lightstyle, 0, sizeof(cl_lightstyle));
    memset(cl_temp_entities, 0, sizeof(cl_temp_entities));
    memset(cl_beams, 0, sizeof(cl_beams));

    //
    // allocate the efrags and chain together into a free list
    //
    cl.free_efrags = cl_efrags;
    int i = 0;
    for (; i < MAX_EFRAGS - 1; i++)
        cl.free_efrags[i].entnext = &cl.free_efrags[i + 1];
    cl.free_efrags[i].entnext = NULL;
}

void CL_Disconnect_f() {
    CL_Disconnect();
    if (Host_IsServerActive())  Host_ShutdownServer(false);
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection(cString host) {
    if ((Host_IsDedicated()) ||
        (cls.isDemoPlaying))
        return;

    CL_Disconnect();

    cls.netcon = NET_Connect(host);
    if (!cls.netcon)
        Host_Error("CL_Connect: connect failed\n");

    Con_DPrintf("CL_EstablishConnection: connected to %s\n", host);

    cls.demonum = -1;   // not in the demo loop now
    cls.state = ca_connected;
    cls.signon = 0;    // need all the signon messages before playing
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo() {
    if (cls.demonum == -1)      return;  // don't play demos

    SCR_BeginLoadingPlaque();

    if (!cls.demos[cls.demonum][0] ||
        cls.demonum == MAX_DEMOS
        ) {
        cls.demonum = 0;
        if (!cls.demos[cls.demonum][0]) {
            Con_Printf("No demos listed with startdemos\n");
            cls.demonum = -1;
            return;
        }
    }
#if 0
    VaBuff_t str;
    snprintf(str, sizeof(str), "playdemo %s\n", cls.demos[cls.demonum]);
    Cbuf_InsertText(str);
#else
    Cbuf_InsertText(va("playdemo %s\n", cls.demos[cls.demonum]));
#endif
    cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f() {
    for (int i = 0; i < cl.num_entities; i++) {
        Con_Printf("%3i:", i);
        if (!cl_entities[i].model) {
            Con_Printf("EMPTY\n");
            continue;
        }
        Con_Printf(
            "%s:%2i  "
            "(%5.1f,%5.1f,%5.1f) "
            "[%5.1f %5.1f %5.1f]\n",
            cl_entities[i].model->name,
            cl_entities[i].frame,

            cl_entities[i].pose.loc.x,
            cl_entities[i].pose.loc.y,
            cl_entities[i].pose.loc.z,

            cl_entities[i].pose.aim.pitch,
            cl_entities[i].pose.aim.yaw,
            cl_entities[i].pose.aim.roll
        );
    }
}




/*
===============
SetPal

Debugging tool, just flashes the screen
===============
*/

typedef enum {
    PalNormal = 0,   // host_basepal
    PalLowFrac = 1,   // green flash — frac < -0.01
    PalHighFrac = 2,   // blue flash  — frac > 1.01
} DbgPal_t;

void SetPal(DbgPal_t i) {
#if 0   /* seems like color frame profiling */
# if 0
    static DbgPal_t _old;

    if (i == _old)   return;
    _old = i;

    if (i == PalNormal)
        VID_SetPalette(host_basepal);
    else if (i == PalLowFrac) {
        uint8_t pal[768];
        for (int c = 0; c < 768; c += 3) {
            pal[c + 0] = 0;
            pal[c + 1] = 255;
            pal[c + 2] = 0;
        }
        VID_SetPalette(pal);
    }
    else { // PalHighFrac
        uint8_t pal[768];
        for (int c = 0; c < 768; c += 3) {
            pal[c + 0] = 0;
            pal[c + 1] = 0;
            pal[c + 2] = 255;
        }
        VID_SetPalette(pal);
    }
# else
    static DbgPal_t _old;

    if (i == _old)   return;
    _old = i;

    switch (i) {
    case PalNormal: {
        VID_SetPalette(host_basepal);
    } break;

    case PalLowFrac: {
        Con_Printf("low frac\n");
        uint8_t pal[768];
        for (int c = 0; c < 768; c += 3) {
            pal[c + 0] = 0;
            pal[c + 1] = 255;
            pal[c + 2] = 0;
        }
        VID_SetPalette(pal);
    } break;

    case PalHighFrac: {
        Con_Printf("high frac\n");
        uint8_t pal[768];
        for (int c = 0; c < 768; c += 3) {
            pal[c + 0] = 0;
            pal[c + 1] = 0;
            pal[c + 2] = 255;
        }
        VID_SetPalette(pal);
    } break;
    };
# endif
#endif
}


/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
LegDt_t CL_LerpPoint() {
    LegDt_t dT = (LegDt_t)(cl.mtime[Cur] - cl.mtime[Prev]);

    if (!dT ||
        cl_nolerp.value ||
        cls.timedemo ||
        Host_IsServerActive()
        ) {
        SetClSimTime(cl.mtime[Cur]);
        return 1.f;
    }

    if (dT > 0.1f) { // dropped packet, or start of demo
        cl.mtime[Prev] = cl.mtime[Cur] - 0.1;
        dT = 0.1f;
    }

    LegDt_t frac = (LegDt_t)(GetClSimTime() - cl.mtime[Prev]) / dT;
    //Con_Printf ("frac: %f\n",frac);
    if (frac < 0.f) {
        if (frac < -0.01f) {
            SetPal(PalLowFrac);
            SetClSimTime(cl.mtime[Prev]);
        }
        frac = 0.f;
    }
    else if (frac > 1.f) {
        if (frac > 1.01f) {
            SetPal(PalHighFrac);
            SetClSimTime(cl.mtime[Cur]);
        }
        frac = 1.f;
    }
    else
        SetPal(PalNormal);

    return frac;
}


/*
===============
CL_RelinkEntities
===============
*/
void CL_RelinkEntities() {
    // determine partial update time
    LegDt_t frac = CL_LerpPoint();

    cl_numvisedicts = 0;

    // interpolate player info
    cl.velocity = VectorMA(cl.mvelocity[Prev],
        frac, VectorSubtract(
            cl.mvelocity[Cur], cl.mvelocity[Prev]
        )
    );

    if (cls.isDemoPlaying) {
        // interpolate the angles
        cl.viewangles = AngleMA(cl.mviewangles[Prev],
            frac, AngleSubtract(
                cl.mviewangles[Cur], cl.mviewangles[Prev]
            )
        );
    }

    float bobjrotate = anglemod((100.f * GetClSimTime()));

    // start on the entity after the world
    r_Entity_p ent = cl_entities + 1;
    for (int i = 1; i < cl.num_entities; i++, ent++) {
        if (!ent->model) { // empty slot
            if (ent->forcelink)
                R_RemoveEfrags(ent); // just became empty
            continue;
        }

        // if the object wasn't included in the last packet, remove it
        if (ent->msgtime != cl.mtime[Cur]) {
            ent->model = NULL;
            continue;
        }

        vec3_t oldorg = ent->pose.loc;

        if (ent->forcelink) { // the entity was not updated in the last message so move to the final loc
            ent->pose.loc = ent->msgPoses[Cur].loc;
            ent->pose.aim = ent->msgPoses[Cur].aim;
        }
        else {  // if the delta is large, assume a teleport and don't lerp
            LegDt_t fr = frac;
            vec3_t delta = VectorSubtract(ent->msgPoses[Cur].loc, ent->msgPoses[Prev].loc);
            if (isVectorOutOfRange(delta, 100.f))
                fr = 1.f;  // assume a teleportation, not a motion

            // interpolate the origin and angles
            ent->pose.loc = VectorMA(ent->msgPoses[Prev].loc, fr, delta);
            ent->pose.aim = AngleMA(ent->msgPoses[Prev].aim,
                fr, AngleSubtract(
                    ent->msgPoses[Cur].aim, ent->msgPoses[Prev].aim
                )
            );
        }

        dLight_p dl = CL_AllocDlight(i);
        vec3_t lOrig = ent->pose.loc; { lOrig.z += 16.f; }

        switch (ent->effects) {
        default: Host_Error("Multiply effects flags setted [0x%X] at same time!", ent->effects); break;
        case EF_NONE: break;

        case EF_BRIGHTFIELD:    R_EntityParticles(ent);    break;
# ifdef QUAKE2
        case EF_DARKFIELD:      R_DarkFieldParticles(ent); break;
# endif

        case EF_MUZZLEFLASH: {
            lOrig = VectorMA(lOrig, 18.f, GetBasis(ent->pose.aim).forward);
            *(dl) = (dLight_t){
                .origin = lOrig,
                .radius = 200.f + (float)((rand() & 31)),
                .die = (sSimTime_t)(GetClSimTime() + 0.1),
                // .decay = 0.f,
                .minlight = 32.f,
                .key = dl->key
            };
        } break;

        case EF_BRIGHTLIGHT:
            *dl = (dLight_t){
                .origin = lOrig,
                .radius = 400.f + (float)((rand() & 31)),
                .die = (sSimTime_t)(GetClSimTime() + 0.001),
                // .decay = 0.f,
                // .minlight = 0.f,
                .key = dl->key
            }; break;

        case EF_DIMLIGHT | EF_MUZZLEFLASH: // 0x0A
        case EF_DIMLIGHT:
            *dl = (dLight_t){
                .origin = lOrig,
                .radius = 200.f + (float)((rand() & 31)),
                .die = (sSimTime_t)(GetClSimTime() + 0.001),
                // .decay = 0.f,
                // .minlight = 0.f,
                .key = dl->key
            }; break;
# ifdef QUAKE2
        case EF_DARKLIGHT:
            *dl = (dLight_t){
                .origin = lOrig,
                .radius = 200.f + (float)((rand() & 31)),
                .die = (sSimTime_t)(GetClSimTime() + 0.001),
                // .decay = 0.f,
                // .minlight = 0.f,
                .key = dl->key,
                .dark = true;
            }; break;
        case EF_LIGHT:
            *dl = (dLight_t){
                .origin = lOrig,
                .radius = 200.f,
                .die = (sSimTime_t)(GetClSimTime() + 0.001),
                // .decay = 0.f,
                // .minlight = 0.f,
                .key = dl->key,
            }; break;
# endif
        }

        // rotate binary objects locally
        switch (ent->model->flags) {
        default: Host_Error("Multiply model flags setted [0x%X] at same time!", ent->model->flags); break;
        case EF_NONE: break;

        case EF_ROTATE: ent->pose.aim.yaw = bobjrotate; break;
        case EF_ROCKET: {
            R_RocketTrail(oldorg, ent->pose.loc, RT_ROCKET);
            *dl = (dLight_t){
                .origin = ent->pose.loc,
                .radius = 200.f,
                .die = (sSimTime_t)(GetClSimTime() + 0.01),
                // .decay = 0.f,
                // .minlight = 0.f,
                .key = dl->key
            };
        } break;
        case EF_GIB:        R_RocketTrail(oldorg, ent->pose.loc, RT_GIB);     break;
        case EF_ZOMGIB:     R_RocketTrail(oldorg, ent->pose.loc, RT_ZOMGIB);  break;
        case EF_TRACER:     R_RocketTrail(oldorg, ent->pose.loc, RT_TRACER);  break;
        case EF_TRACER2:    R_RocketTrail(oldorg, ent->pose.loc, RT_TRACER2); break;
        case EF_GRENADE:    R_RocketTrail(oldorg, ent->pose.loc, RT_GRENADE); break;
        case EF_TRACER3:    R_RocketTrail(oldorg, ent->pose.loc, RT_TRACER3); break;
        };

        ent->forcelink = false;

        if ((i == cl.viewentity) &&
            (!chase_active.value)
            )  continue;

#ifdef QUAKE2
        if (ent->effects & EF_NODRAW)                   continue;
#endif
        if (cl_numvisedicts < MAX_VISEDICTS) {
            cl_visedicts[cl_numvisedicts] = ent;
            cl_numvisedicts++;
        }
    }

}


/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
void CL_ReadFromServer() {
    cl.oldtime = GetClSimTime();
    AddClSimTime(host_frametime);

    int ret;
    do {
        ret = CL_GetMessage();
        if (ret == -1)  Host_Error("CL_ReadFromServer: lost server connection");
        if (!ret)       break;

        cl.last_received_message = GetRealTime();
        CL_ParseServerMessage();
    } while (ret && (cls.state == ca_connected));

    if (cl_shownet.value)   Con_Printf("\n");

    CL_RelinkEntities();
    CL_UpdateTEnts();

    //
    // bring the links up to date
    //
    // return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd() {
    if (cls.state != ca_connected)  return;

    if (cls.signon == SIGNONS) {
        UserCmd_t  cmd;
        CL_BaseMove(&cmd);  // get basic movement from keyboard
        IN_Move(&cmd);      // allow mice or other external controllers to add to the move
        CL_SendMove(&cmd);  // send the unreliable message
    }

    if (cls.isDemoPlaying) { SZ_Clear(&cls.message); return; }

    // send the reliable message
    if (!cls.message.cursize)   return;  // no message at all

    if (!NET_CanSendMessage(cls.netcon)) {
        Con_DPrintf("CL_WriteToServer: can't send\n");
        return;
    }

    if (NET_SendMessage(cls.netcon, &cls.message) == -1)
        Host_Error("CL_WriteToServer: lost server connection");

    SZ_Clear(&cls.message);
}

/*
=================
CL_Init
=================
*/
void CL_Init() {
    SZ_Alloc(&cls.message, 1024);

    CL_InitInput();
    CL_InitTEnts();

    //
    // register our commands
    //
    Cvar_RegisterVariable(&cl_name);
    Cvar_RegisterVariable(&cl_color);
    Cvar_RegisterVariable(&cl_upspeed);
    Cvar_RegisterVariable(&cl_forwardspeed);
    Cvar_RegisterVariable(&cl_backspeed);
    Cvar_RegisterVariable(&cl_sidespeed);
    Cvar_RegisterVariable(&cl_movespeedkey);
    Cvar_RegisterVariable(&cl_yawspeed);
    Cvar_RegisterVariable(&cl_pitchspeed);
    Cvar_RegisterVariable(&cl_anglespeedkey);
    Cvar_RegisterVariable(&cl_shownet);
    Cvar_RegisterVariable(&cl_nolerp);
    Cvar_RegisterVariable(&lookspring);
    Cvar_RegisterVariable(&lookstrafe);
    Cvar_RegisterVariable(&sensitivity);

    Cvar_RegisterVariable(&m_pitch);
    Cvar_RegisterVariable(&m_yaw);
    Cvar_RegisterVariable(&m_forward);
    Cvar_RegisterVariable(&m_side);

    // Cvar_RegisterVariable(&cl_autofire);

    Cmd_AddCommand("entities", CL_PrintEntities_f);
    Cmd_AddCommand("disconnect", CL_Disconnect_f);
    Cmd_AddCommand("record", CL_Record_f);
    Cmd_AddCommand("stop", CL_Stop_f);
    Cmd_AddCommand("playdemo", CL_PlayDemo_f);
    Cmd_AddCommand("timedemo", CL_TimeDemo_f);
}

