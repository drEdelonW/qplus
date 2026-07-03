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

#include "progs.h"
#include "pr_cmds.h"
#include "pf_list_priv.h"
#include "server_priv.h" // SV_MoveToGoal


void PF_Fixme() { PR_RunError("unimplemented bulitin"); }


static builtin_t _pr_builtin[] = {
    PF_Fixme,
    PF_makevectors,     // void(entity e) makevectors                           = #1;
    PF_setorigin,       // void(entity e, vector o) setorigin                   = #2;
    PF_setmodel,        // void(entity e, string m) setmodel                    = #3;
    PF_setsize,         // void(entity e, vector min, vector max) setsize       = #4;
    PF_Fixme,           // void(entity e, vector min, vector max) setabssize    = #5;
    PF_break,           // void() break                                         = #6;
    PF_random,          // float() random                                       = #7;
    PF_sound,           // void(entity e, float chan, string samp) sound        = #8;
    PF_normalize,       // vector(vector v) normalize                           = #9;
    PF_error,           // void(string e) error                                 = #10;
    PF_objerror,        // void(string e) objerror                              = #11;
    PF_vlen,            // float(vector v) vlen                                 = #12;
    PF_vectoyaw,        // float(vector v) vectoyaw                             = #13;
    PF_Spawn,           // entity() spawn                                       = #14;
    PF_Remove,          // void(entity e) remove                                = #15;
    PF_traceline,       // float(vector v1, vector v2, float tryents) traceline = #16;
    PF_checkclient,     // entity() clientlist                                  = #17;
    PF_Find,            // entity(entity start, .string fld, string match) find = #18;
    PF_precache_sound,  // void(string str) precache_sound                      = #19;
    PF_precache_model,  // void(string str) precache_model                      = #20;
    PF_stuffcmd,        // void(entity client, string str)stuffcmd              = #21;
    PF_findradius,      // entity(vector org, float rad) findradius             = #22;
    PF_bprint,          // void(string str) bprint                              = #23;
    PF_sprint,          // void(entity client, string str) sprint               = #24;
    PF_dprint,          // void(string str) dprint                              = #25;
    PF_ftos,            // void(string str) ftos                                = #26;
    PF_vtos,            // void(string str) vtos                                = #27;
    PF_coredump,
    PF_traceon,
    PF_traceoff,
    PF_eprint,          // void(entity e) debug print an entire entity
    PF_walkmove,        // float(float yaw, float dist) walkmove
    PF_Fixme,           // float(float yaw, float dist) walkmove
    PF_droptofloor,
    PF_lightstyle,
    PF_rint,
    PF_floor,
    PF_ceil,
    PF_Fixme,
    PF_checkbottom,
    PF_pointcontents,
    PF_Fixme,
    PF_fabs,
    PF_aim,
    PF_cvar,
    PF_localcmd,
    PF_nextent,
    PF_particle,
    PF_changeyaw,
    PF_Fixme,
    PF_vectoangles,

    PF_WriteByte,
    PF_WriteChar,
    PF_WriteShort,
    PF_WriteLong,
    PF_WriteCoord,
    PF_WriteAngle,
    PF_WriteString,
    PF_WriteEntity,

#ifdef QUAKE2
    PF_sin,
    PF_cos,
    PF_sqrt,
    PF_changepitch,
    PF_TraceToss,
    PF_etos,
    PF_WaterMove,
#else
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
#endif

    SV_MoveToGoal,
    PF_precache_file,
    PF_makestatic,

    PF_changelevel,
    PF_Fixme,

    PF_cvar_set,
    PF_centerprint,

    PF_ambientsound,

    PF_precache_model,
    PF_precache_sound, // precache_sound2 is different only for qcc
    PF_precache_file,

    PF_setspawnparms
};

builtin_t* pr_builtins = _pr_builtin;
int32_t pr_numbuiltins = sizeof(_pr_builtin) / sizeof(_pr_builtin[0]);
