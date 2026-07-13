#pragma once

#include "sv_phys.h"
#include "server.h"
#include "cvar_q1.h"
#include "q_tools.h"
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "console.h"
#include "host.h"
#include "mathlib.h"
#include "transform.h"
#include "progs.h"
#include "GlobVars.h"
#include "GameRule.h"
#include "trace.h"


/* --- movement clip flags (bitmask) --- */
typedef enum {
    MOVECLIP_NONE       = 0u,        /* no block */
    MOVECLIP_FLOOR      = 1u << 0,   /* floor (normal[Z_AX] > 0) */
    MOVECLIP_WALL       = 1u << 1,   /* wall/step (normal[Z_AX] == 0) */
    MOVECLIP_DEADSTOP   = 1u << 2,   /* dead stop (reserved by original comment) */

    /* --- special early-return results (non-bitmask) --- */
    /* note: 3 == (MOVECLIP_FLOOR|MOVECLIP_WALL) by value; here it is used as “trapped/allsolid” */
    FLYMOVE_TRAPPED = MOVECLIP_FLOOR | MOVECLIP_WALL,   /* allsolid / clipped to too many planes */
    FLYMOVE_STUCK = FLYMOVE_TRAPPED | MOVECLIP_DEADSTOP /* unresolvable geometry / still stuck */
} MoveClipFlags_e;

/*
=============================================================================
sv_phys_priv.h - private declarations for sv_phys / sv_move subsystem
=============================================================================
*/
#pragma once

// sv_phys_core.c
MoveClipFlags_e ClipVelocity(vec3_t in, vec3_t normal, vec3_p out, float overbounce);
MoveClipFlags_e SV_FlyMove(edict_p ent, SimDt_t time, trace_p steptrace);
void            SV_AddGravity(edict_p ent);
void            SV_CheckVelocity(edict_p ent);
void            SV_Impact(edict_p e1, edict_p e2);
bool            SV_RunThink(edict_p ent);

// sv_phys_push.c
trace_t SV_PushEntity(edict_p ent, vec3_t push);
void    SV_PushMove(edict_p pusher, SimDt_t movetime);
void    SV_Physics_Pusher(edict_p ent);
#ifdef QUAKE2
void    SV_PushRotate(edict_p pusher, float movetime);
#endif

// sv_phys_client.c
void            SV_CheckStuck(edict_p ent);
bool            SV_CheckWater(edict_p ent);
void            SV_WallFriction(edict_p ent, trace_p trace);
MoveClipFlags_e SV_TryUnstick(edict_p ent, vec3_t oldvel);
void            SV_WalkMove(edict_p ent);
void            SV_Physics_Client(edict_p ent, EdIdx clNum);

// sv_phys_ent.c
void    SV_CheckWaterTransition(edict_p ent);
void    SV_Physics_Toss(edict_p ent);
void    SV_Physics_None(edict_p ent);
void    SV_Physics_Noclip(edict_p ent);
void    SV_Physics_Step(edict_p ent);
#ifdef QUAKE2
void    SV_Physics_Follow(edict_p ent);
#endif
#ifndef QUAKE2
trace_t SV_Trace_Toss(edict_p ent, edict_p ignore);
#endif

// sv_phys.c
void SV_CheckAllEnts(void);
void SV_Physics(void);

// sv_monster_move.c
bool SV_movestep(edict_p ent, vec3_t move, bool relink);

// sv_monster_ai.c
bool SV_CloseEnough(edict_p ent, edict_p goal, float dist);
void SV_NewChaseDir(edict_p actor, edict_p enemy, float dist);
void SV_MoveToGoal(void);

edict_p SV_TestEntityPosition(edict_p ent);
