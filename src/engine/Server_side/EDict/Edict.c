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
// sv_edict.c -- entity dictionary

// #include "pr_comp.h"
#include "progs.h"
#include "pr_Argument.h"
#include "pr_Function.h"
#include "Edict.h"
#include "world.h"
#include "q_tools.h"
#include "console.h"
#include "cmd.h"
#include "common.h"
#include "host.h"
#include "mathlib.h"
#include <string.h>
#include <stdlib.h>
#include "server.h"

#include "cvar_q1.h"

uint32_t pr_edict_size;      // in bytes
edict_p Edicts;
uint32_t EdictsMax = MAX_EDICTS;
uint32_t EdictsNum;



/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict(edict_p edict) {
    memset(&edict->v, 0, progs->entityfields * 4);
    edict->free = false;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_p ED_Alloc() {
    uint32_t i = svs.maxClients + 1;
    for (; i < EdictsNum; i++) {
        edict_p edict = ED_GetEDictByIdx(i);
        // the first couple seconds of server time can involve a lot of
        // freeing and allocating, so relax the replacement policy
        if ((edict->free) &&
            (
                (edict->freetime < 2.0f) ||
                ((SV_GetTime() - edict->freetime) > 0.5f)
                )
            ) {
            ED_ClearEdict(edict);
            return edict;
        }
    }

    if (i == MAX_EDICTS)            Host_SysError("ED_Alloc: no free edicts");

    EdictsNum++;
    edict_p edict = ED_GetEDictByIdx(i);
    ED_ClearEdict(edict);

    return edict;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free(edict_p ed) {
    SV_UnlinkEdict(ed); // unlink from world bsp

    ed->free = true;
    ed->v.model = 0;
    ed->v.takedamage = 0;
    ed->v.modelindex = 0;
    ed->v.colormap = 0;
    ed->v.skin = 0;
    ed->v.frame = 0;
    VectorCopy(vec3_origin, ed->v.origin);
    VectorCopy(vec3_origin, ed->v.angles);
    ed->v.nextthink = -1;
    ed->v.solid = 0;

    ed->freetime = (float)SV_GetTime();
}

//===========================================================================

/*
============
ED_GlobalAtOfs
============
*/
dDef_p ED_GlobalAtOfs(int ofs) {
    for (int i = 0; i < progs->globaldefs.num; i++) {
        dDef_p def = &pr_globaldefs[i];
        if (def->ofs == ofs)
            return def;
    }
    return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
dDef_p ED_FieldAtOfs(int ofs) {
    for (int i = 0; i < progs->fielddefs.num; i++) {
        dDef_p def = &pr_fielddefs[i];
        if (def->ofs == ofs)
            return def;
    }
    return NULL;
}

/*
============
ED_FindField
============
*/
dDef_p ED_FindField(cString name) {
    for (int i = 0; i < progs->fielddefs.num; i++) {
        dDef_p def = &pr_fielddefs[i];
        if (!strcmp(PR_GetQString(def->s_name), name))
            return def;
    }
    return NULL;
}


/*
============
ED_FindGlobal
============
*/
dDef_p ED_FindGlobal(cString name) {
    for (int i = 0; i < progs->globaldefs.num; i++) {
        dDef_p def = &pr_globaldefs[i];
        if (!strcmp(PR_GetQString(def->s_name), name))
            return def;
    }
    return NULL;
}


/*
============
ED_FindFunction
============
*/
dFunction_p ED_FindFunction(cString name) {
    for (int i = 0; i < progs->functions.num; i++) {
        dFunction_p func = &pr_functions[i];
        if (!strcmp(PR_GetQString(func->s_name), name))
            return func;
    }
    return NULL;
}


eval_p GetEdictFieldValue(edict_p ed, cString field) {
    static int _rep = 0;

    dDef_p def = NULL;
    for (int i = 0; i < GEFV_CACHESIZE; i++) {
        if (!strcmp(field, gefvCache[i].field)) {
            def = gefvCache[i].pcache;
            goto Done;
        }
    }

    def = ED_FindField(field);

    if (strlen(field) < MAX_FIELD_LEN) {
        gefvCache[_rep].pcache = def;
        strcpy(gefvCache[_rep].field, field);
        _rep ^= 1;
    }

Done:
    if (!def)   return NULL;

    return (eval_p)((cString)&ed->v + def->ofs * 4);
}




/*
=============
ED_Print

For debugging
=============
*/
void ED_Print(edict_p ed) {
    if (ed->free) { Host_Printf("FREE\n"); return; }

    Host_Printf("\nEDICT %i:\n", ED_GetEDictIdx(ed));
    for (int i = 1; i < progs->fielddefs.num; i++) {
        dDef_p d = &pr_fielddefs[i];
        cString name = PR_GetQString(d->s_name);
        if (name[strlen(name) - 2] == '_')
            continue; // skip _x, _y, _z vars

        int32_p v = (int32_p)((cString)&ed->v + d->ofs * 4);

        // if the value is still all 0, skip the field
        uint32_t type = d->type & ~DEF_SAVEGLOBAL;

        int j = 0;
        for (; j < type_size[type]; j++)
            if (v[j])
                break;

        if (j == type_size[type])
            continue;

        Host_Printf("%s", name);
        size_t l = strlen(name);
        while (l++ < 15)
            Host_Printf(" ");

        Host_Printf("%s\n", PR_ValueString(d->type, (eval_p)v));
    }
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write(FILE* f, edict_p ed) {
    fprintf(f, "{\n");

    if (ed->free) { fprintf(f, "}\n"); return; }

    for (int i = 1; i < progs->fielddefs.num; i++) {
        dDef_p d = &pr_fielddefs[i];
        cString name = PR_GetQString(d->s_name);
        if (name[strlen(name) - 2] == '_')
            continue; // skip _x, _y, _z vars

        int32_p v = (int32_p)((cString)&ed->v + d->ofs * 4);

        // if the value is still all 0, skip the field
        uint32_t type = d->type & ~DEF_SAVEGLOBAL;
        int j = 0;
        for (; j < type_size[type]; j++)
            if (v[j])
                break;

        if (j == type_size[type])
            continue;

        fprintf(f, "\"%s\" ", name);
        fprintf(f, "\"%s\"\n", PR_UglyValueString(d->type, (eval_p)v));
    }

    fprintf(f, "}\n");
}

void ED_PrintNum(uint32_t ent) {
    ED_Print(ED_GetEDictByIdx(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts() {
    Host_Printf("%i entities\n", EdictsNum);
    for (uint32_t i = 0; i < EdictsNum; i++)
        ED_PrintNum(i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f() {
    uint32_t i = (uint32_t)Q_atoi(Cmd_Argv(1));
    if (i >= EdictsNum) { Host_Printf("Bad edict number\n"); return; }
    ED_PrintNum(i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count() {
    int active = 0;
    int models = 0;
    int solid = 0;
    int step = 0;
    for (uint32_t i = 0; i < EdictsNum; i++) {
        edict_p ent = ED_GetEDictByIdx(i);
        if (ent->free)  continue;

        active++;
        if (ent->v.solid)   solid++;
        if (ent->v.model)   models++;
        if (ent->v.movetype == MOVETYPE_STEP)   step++;
    }

    Host_Printf("num_edicts:%3i\n", EdictsNum);
    Host_Printf("active    :%3i\n", active);
    Host_Printf("view      :%3i\n", models);
    Host_Printf("touch     :%3i\n", solid);
    Host_Printf("step      :%3i\n", step);

}

/*
==============================================================================

                    ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals(FILE* f) {
    fprintf(f, "{\n");
    for (int i = 0; i < progs->globaldefs.num; i++) {
        dDef_p def = &pr_globaldefs[i];
        uint32_t type = def->type;
        if (!(def->type & DEF_SAVEGLOBAL))  continue;
        type &= ~DEF_SAVEGLOBAL;

        if ((type != ev_string) &&
            (type != ev_float) &&
            (type != ev_entity)
            )
            continue;

        cString name = PR_GetQString(def->s_name);
        fprintf(f, "\"%s\" ", name);
        fprintf(f, "\"%s\"\n", PR_UglyValueString(type, (eval_p)&pr_globals[def->ofs]));
    }
    fprintf(f, "}\n");
}

/*
=============
ED_ParseGlobals
=============
*/
void ED_ParseGlobals(cString data) {
    while (1) {
        // parse key
        data = COM_Parse(data);
        if (com.token[0] == '}')    break;
        if (!data)                  Host_SysError("ED_ParseEntity: EOF without closing brace");

        char keyname[NAME_LENGTH];
        strcpy(keyname, com.token);

        // parse value
        data = COM_Parse(data);
        if (!data)                  Host_SysError("ED_ParseEntity: EOF without closing brace");
        if (com.token[0] == '}')    Host_SysError("ED_ParseEntity: closing brace without data");

        dDef_p key = ED_FindGlobal(keyname);
        if (!key) {
            Host_Printf("'%s' is not a global\n", keyname);
            continue;
        }

        if (!ED_ParseEpair((TypeLess_ptr)pr_globals, key, com.token))
            Host_Error("ED_ParseGlobals: parse error");
    }
}

//============================================================================


/*
=============
ED_NewString
=============
*/
cString ED_NewString(cString string) {
    size_t l = strlen(string) + 1;
    cString new = Hunk_Alloc(l);
    cString new_p = new;

    for (int i = 0; i < l; i++) {
        if ((string[i] == '\\') &&
            (i < (l - 1))
            ) {
            i++;
            if (string[i] == 'n')   *new_p++ = '\n';
            else                    *new_p++ = '\\';
        }
        else                        *new_p++ = string[i];
    }

    return new;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
bool ED_ParseEpair(TypeLess_ptr base, dDef_p key, cString s) {
    TypeLess_ptr d = (TypeLess_ptr)((int32_p)base + key->ofs);

    switch (key->type & ~DEF_SAVEGLOBAL) {
    case ev_string:     *(string_t*)d = PR_SetQString(ED_NewString(s));               break;
    case ev_float:      *(float_p)d = (float)atof(s);                               break;
    case ev_entity:     *(int32_p)d = ED_GetEDictOffs(ED_GetEDictByIdx((uint32_t)atoi(s)));  break;

    case ev_vector: {
        char string[128];
        strcpy(string, s);
        cString v = string;
        cString w = string;
        for (int i = 0; i < 3; i++) {
            while ((*v != 0) && (*v != ' '))
                v++;
            *v = 0;
            ((float_p)d)[i] = (float)atof(w);
            w = v = v + 1;
        }
    } break;

    case ev_field: {
        dDef_p def = ED_FindField(s);
        if (!def) {
            Host_Printf("Can't find field %s\n", s);
            return false;
        }
        *(int32_p)d = G_INT(def->ofs);
    } break;

    case ev_function: {
        dFunction_p func = ED_FindFunction(s);
        if (!func) {
            Host_Printf("Can't find function %s\n", s);
            return false;
        }
        *(func_t*)d = func - pr_functions;
    } break;

    default:    break;
    }
    return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
cString ED_ParseEdict(cString data, edict_p ent) {
    bool init = false;

    // clear it
    if (ent != Edicts) // hack
        memset(&ent->v, 0, progs->entityfields * 4);

    // go through all the dictionary pairs
    while (1) {
        // parse key
        data = COM_Parse(data);
        if (com.token[0] == '}')    break;
        if (!data)                  Host_SysError("ED_ParseEntity: EOF without closing brace");

        // anglehack is to allow QuakeEd to write single scalar angles
        // and allow them to be turned into vectors. (FIXME...)
        bool anglehack;
        if (!strcmp(com.token, "angle")) {
            strcpy(com.token, "angles");
            anglehack = true;
        }
        else
            anglehack = false;

        // FIXME: change light to _light to get rid of this hack
        if (!strcmp(com.token, "light"))
            strcpy(com.token, "light_lev"); // hack for single light def

        char keyname[256];
        strcpy(keyname, com.token);

        // another hack to fix heynames with trailing spaces
        size_t n = strlen(keyname);
        while (n && keyname[n - 1] == ' ') {
            keyname[n - 1] = 0;
            n--;
        }

        // parse value
        data = COM_Parse(data);
        if (!data)                  Host_SysError("ED_ParseEntity: EOF without closing brace");
        if (com.token[0] == '}')    Host_SysError("ED_ParseEntity: closing brace without data");

        init = true;

        // keynames with a leading underscore are used for utility comments,
        // and are immediately discarded by quake
        if (keyname[0] == '_')
            continue;

        dDef_p key = ED_FindField(keyname);
        if (!key) {
            Host_Printf("'%s' is not a field\n", keyname);
            continue;
        }

        if (anglehack) {
            char temp[32];
            strcpy(temp, com.token);
            snprintf(com.token, sizeof(com.token), "0 %s 0", temp);
        }

        if (!ED_ParseEpair((TypeLess_ptr)&ent->v, key, com.token))
            Host_Error("ED_ParseEdict: parse error");
    }

    if (!init)
        ent->free = true;

    return data;
}


/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile(cString data) {
    edict_p ent = NULL;
    int inhibit = 0;
    pr_global_struct->time = (float)SV_GetTime();

    // parse ents
    while (1) {
        // parse the opening brace
        data = COM_Parse(data);
        if (!data)  break;
        if (com.token[0] != '{')    Host_SysError("ED_LoadFromFile: found %s when expecting {", com.token);

        if (!ent)   ent = ED_GetEDictByIdx(0);
        else        ent = ED_Alloc();
        data = ED_ParseEdict(data, ent);

        // remove things from different skill levels or deathmatch
        if (deathmatch.value) {
            if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH)) {
                ED_Free(ent);
                inhibit++;
                continue;
            }
        }
        else
            if (((current_skill == 0) && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY)) ||
                ((current_skill == 1) && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
                ((current_skill >= 2) && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD))) {
                ED_Free(ent);
                inhibit++;
                continue;
            }

        //
        // immediately call spawn function
        //
        if (!ent->v.classname) {
            Host_Printf("No classname for:\n");
            ED_Print(ent);
            ED_Free(ent);
            continue;
        }

        // look for the spawn function
        dFunction_p func = ED_FindFunction(PR_GetQString(ent->v.classname));

        if (!func) {
            Host_Printf("No spawn function for:\n");
            ED_Print(ent);
            ED_Free(ent);
            continue;
        }

        pr_global_struct->self = ED_GetEDictOffs(ent);
        PR_ExecuteProgram(func - pr_functions);
    }

    Con_DPrintf("%i entities inhibited\n", inhibit);
}



edict_p FindViewthing() {
    for (uint32_t i = 0; i < EdictsNum; i++) {
        edict_p eDict = ED_GetEDictByIdx(i);
        if (!strcmp(PR_GetQString(eDict->v.classname), "viewthing"))  return eDict;
    }
    Con_Printf("No viewthing on map\n");
    return NULL;
}

/*
===============
PR_Init
===============
*/
void ED_Init() {
        // allocate server memory
    // sv.max_edicts = MAX_EDICTS;
    // EdictsMax = max_edicts;

    // Edicts = Hunk_AllocName((uint32_t)EdictsMax * pr_edict_size, "edicts");
    // sv.edicts = Edicts;

    Cmd_AddCommand("edict", ED_PrintEdict_f);
    Cmd_AddCommand("edicts", ED_PrintEdicts);
    Cmd_AddCommand("edictcount", ED_Count);
}