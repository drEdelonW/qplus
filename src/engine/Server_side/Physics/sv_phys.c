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
// sv_phys.c

#include "sv_phys_priv.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/


/*
================
SV_CheckAllEnts
================
*/
void SV_CheckAllEnts() {
    // see if any solid entities are inside the final position
    for (EdIdx e = EdictPlayer1; e < GetEdNum(); e++) {
        edict_p check = ED_GetEDictByIdx(e);

        if (check->free) continue;

        switch ((movetype_t)check->v.movetype) {
#ifdef QUAKE2
        case MOVETYPE_FOLLOW:
#endif
        case MOVETYPE_NONE:
        case MOVETYPE_PUSH:
        case MOVETYPE_NOCLIP:   continue;
        default:    break;
        }

        if (SV_TestEntityPosition(check))
            Con_Printf("entity in invalid position\n");
    }
}



/*
================
SV_Physics

================
*/
void SV_Physics() {
    // let the progs know that a new frame has started
    pr_global_struct->self = ED_GetEDictOffs(Edicts); // should be 0
    pr_global_struct->other = ED_GetEDictOffs(Edicts); // should be 0s
    pr_global_struct->time = (float)SV_GetTime();
    PR_ExecuteProgram(pr_global_struct->StartFrame);

    //SV_CheckAllEnts();

    //
    // treat each object in turn
    for (EdIdx e = EdictWorld; e < GetEdNum(); e++) {
        edict_p ent = ED_GetEDictByIdx(e);

        if (ent->free)  continue;

        if (pr_global_struct->force_retouch)
            SV_LinkEdict(ent, true); // force retouch even for stationary

        if ((e > EdictWorld) &&
            (e <= GetSvMaxClients())
            )   SV_Physics_Client(ent, e);
        else {
            switch ((movetype_t)ent->v.movetype) {
#ifdef QUAKE2
            case MOVETYPE_FOLLOW:       SV_Physics_Follow(ent); break;
            case MOVETYPE_BOUNCEMISSILE:
#endif
            case MOVETYPE_TOSS:
            case MOVETYPE_BOUNCE:
            case MOVETYPE_FLY:
            case MOVETYPE_FLYMISSILE:   SV_Physics_Toss(ent);   break;
            case MOVETYPE_PUSH:         SV_Physics_Pusher(ent); break;
            case MOVETYPE_NONE:         SV_Physics_None(ent);   break;
            case MOVETYPE_NOCLIP:       SV_Physics_Noclip(ent); break;
            case MOVETYPE_STEP:         SV_Physics_Step(ent);   break;

            default:    Host_SysError("SV_Physics: bad movetype %i", (movetype_t)ent->v.movetype); break;
            }
        }
    }

    if (pr_global_struct->force_retouch)
        pr_global_struct->force_retouch--;

    AddSvSimTime(host_frametime);
}

