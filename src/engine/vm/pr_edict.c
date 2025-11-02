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

#include "progs.h"
#include "server.h"
#include "sys.h"
#include "world.h"
#include "q_tools.h"
#include "console.h"
#include "cmd.h"
#include "common.h"
#include "host.h"
#include "crc.h"
#include "mathlib.h"
#include "endian_tools.h"
#include <string.h>
#include <stdlib.h>

#include "cvar_q1.h"

/* extern */ dprograms_p    progs;
/* extern */ dFunction_p    pr_functions;
/* extern */ cString        pr_strings;         // much more
/* extern */ dDef_p         pr_fielddefs;
/* extern */ dDef_p         pr_globaldefs;
/* extern */ dStatement_p   pr_statements;
/* extern */ globalvars_p   pr_global_struct;   // much more
/* extern */ float_p        pr_globals;         // same as pr_global_struct
/* extern */ int32_t        pr_edict_size;      // in bytes

uint16_t pr_crc;

int type_size[8] = {
    1,                          // ev_void,
    sizeof(string_t) / 4,       // ev_string,
    1,                          // ev_float,
    3,                          // ev_vector,
    1,                          // ev_entity,
    1,                          // ev_field,
    sizeof(func_t) / 4,         // ev_function,
    sizeof(TypeLess_ptr) / 4    // ev_pointer
};



#define MAX_FIELD_LEN (64)
#define GEFV_CACHESIZE (2)
typedef struct {
    dDef_p  pcache;
    char    field[MAX_FIELD_LEN];
} gefv_cache;

static gefv_cache _gefvCache[GEFV_CACHESIZE] = {
    {NULL, ""},
    {NULL, ""}
};

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
    edict_p edict;
    int i = svs.maxclients + 1;
    for (; i < sv.num_edicts; i++) {
        edict = EDICT_NUM(i);
        // the first couple seconds of server time can involve a lot of
        // freeing and allocating, so relax the replacement policy
        if ((edict->free) &&
            (
                (edict->freetime < 2) ||
                (sv.time - edict->freetime > 0.5)
                )
            ) {
            ED_ClearEdict(edict);
            return edict;
        }
    }

    if (i == MAX_EDICTS)            Sys_Error("ED_Alloc: no free edicts");

    sv.num_edicts++;
    edict = EDICT_NUM(i);
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

    ed->freetime = sv.time;
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
        if (!strcmp(pr_strings + def->s_name, name))
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
        if (!strcmp(pr_strings + def->s_name, name))
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
        if (!strcmp(pr_strings + func->s_name, name))
            return func;
    }
    return NULL;
}


eval_p GetEdictFieldValue(edict_p ed, cString field) {
    static int  rep = 0;

    dDef_p def = NULL;
    for (int i = 0; i < GEFV_CACHESIZE; i++) {
        if (!strcmp(field, _gefvCache[i].field)) {
            def = _gefvCache[i].pcache;
            goto Done;
        }
    }

    def = ED_FindField(field);

    if (strlen(field) < MAX_FIELD_LEN) {
        _gefvCache[rep].pcache = def;
        strcpy(_gefvCache[rep].field, field);
        rep ^= 1;
    }

Done:
    if (!def)   return NULL;

    return (eval_p)((cString)&ed->v + def->ofs * 4);
}


/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
cString PR_ValueString(etype_t type, eval_p val) {
    static char line[256];
    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
    case ev_string:     sprintf(line, "%s", pr_strings + val->string);                                          break;
    case ev_entity:     sprintf(line, "entity %i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));                   break;
    case ev_function:
        dFunction_p f = pr_functions + val->function;
        sprintf(line, "%s()", pr_strings + f->s_name);
        break;
    case ev_field:
        dDef_p def = ED_FieldAtOfs(val->_int);
        sprintf(line, ".%s", pr_strings + def->s_name);
        break;
    case ev_void:       sprintf(line, "void");                                                                  break;
    case ev_float:      sprintf(line, "%5.1f", val->_float);                                                    break;
    case ev_vector:     sprintf(line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);   break;
    case ev_pointer:    sprintf(line, "pointer");                                                               break;
    default:            sprintf(line, "bad type %i", type);                                                     break;
    }

    return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
cString PR_UglyValueString(etype_t type, eval_p val) {
    static char line[256];
    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
    case ev_string:     sprintf(line, "%s", pr_strings + val->string);                              break;
    case ev_entity:     sprintf(line, "%i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));              break;
    case ev_function:
        dFunction_p f = pr_functions + val->function;
        sprintf(line, "%s", pr_strings + f->s_name);
        break;
    case ev_field:
        dDef_p def = ED_FieldAtOfs(val->_int);
        sprintf(line, "%s", pr_strings + def->s_name);
        break;
    case ev_void:       sprintf(line, "void");                                                      break;
    case ev_float:      sprintf(line, "%f", val->_float);                                           break;
    case ev_vector:     sprintf(line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);  break;
    default:            sprintf(line, "bad type %i", type);                                         break;
    }

    return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
cString PR_GlobalString(int ofs) {
    static char line[128];

    TypeLess_ptr val = (TypeLess_ptr)&pr_globals[ofs];
    dDef_p def = ED_GlobalAtOfs(ofs);
    if (!def)
        sprintf(line, "%i(???)", ofs);
    else {
        cString s = PR_ValueString(def->type, val);
        sprintf(line, "%i(%s)%s", ofs, pr_strings + def->s_name, s);
    }

    int i = strlen(line);
    for (; i < 20; i++)
        strcat(line, " ");
    strcat(line, " ");

    return line;
}

cString PR_GlobalStringNoContents(int ofs) {
    static char line[128];

    dDef_p def = ED_GlobalAtOfs(ofs);
    if (!def)   sprintf(line, "%i(???)", ofs);
    else        sprintf(line, "%i(%s)", ofs, pr_strings + def->s_name);

    int i = strlen(line);
    for (; i < 20; i++)
        strcat(line, " ");
    strcat(line, " ");

    return line;
}


/*
=============
ED_Print

For debugging
=============
*/
void ED_Print(edict_p ed) {
    if (ed->free) { Con_Printf("FREE\n"); return; }

    Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
    for (int i = 1; i < progs->fielddefs.num; i++) {
        dDef_p d = &pr_fielddefs[i];
        cString name = pr_strings + d->s_name;
        if (name[strlen(name) - 2] == '_')
            continue; // skip _x, _y, _z vars

        int* v = (int*)((cString)&ed->v + d->ofs * 4);

        // if the value is still all 0, skip the field
        int type = d->type & ~DEF_SAVEGLOBAL;

        int j = 0;
        for (; j < type_size[type]; j++)
            if (v[j])
                break;

        if (j == type_size[type])
            continue;

        Con_Printf("%s", name);
        int l = strlen(name);
        while (l++ < 15)
            Con_Printf(" ");

        Con_Printf("%s\n", PR_ValueString(d->type, (eval_p)v));
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
        cString name = pr_strings + d->s_name;
        if (name[strlen(name) - 2] == '_')
            continue; // skip _x, _y, _z vars

        int* v = (int*)((cString)&ed->v + d->ofs * 4);

        // if the value is still all 0, skip the field
        int type = d->type & ~DEF_SAVEGLOBAL;
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

void ED_PrintNum(int ent) {
    ED_Print(EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts() {
    Con_Printf("%i entities\n", sv.num_edicts);
    for (int i = 0; i < sv.num_edicts; i++)
        ED_PrintNum(i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f() {
    int i = Q_atoi(Cmd_Argv(1));
    if (i >= sv.num_edicts) { Con_Printf("Bad edict number\n"); return; }
    ED_PrintNum(i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count() {
    int  active, models, solid, step;
    active = models = solid = step = 0;
    for (int i = 0; i < sv.num_edicts; i++) {
        edict_p ent = EDICT_NUM(i);
        if (ent->free)  continue;

        active++;
        if (ent->v.solid)   solid++;
        if (ent->v.model)   models++;
        if (ent->v.movetype == MOVETYPE_STEP)   step++;
    }

    Con_Printf("num_edicts:%3i\n", sv.num_edicts);
    Con_Printf("active    :%3i\n", active);
    Con_Printf("view      :%3i\n", models);
    Con_Printf("touch     :%3i\n", solid);
    Con_Printf("step      :%3i\n", step);

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
        int type = def->type;
        if (!(def->type & DEF_SAVEGLOBAL))  continue;
        type &= ~DEF_SAVEGLOBAL;

        if ((type != ev_string) &&
            (type != ev_float) &&
            (type != ev_entity)
            )
            continue;

        cString name = pr_strings + def->s_name;
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
        if (!data)                  Sys_Error("ED_ParseEntity: EOF without closing brace");

        char keyname[64];
        strcpy(keyname, com.token);

        // parse value
        data = COM_Parse(data);
        if (!data)                  Sys_Error("ED_ParseEntity: EOF without closing brace");
        if (com.token[0] == '}')    Sys_Error("ED_ParseEntity: closing brace without data");

        dDef_p key = ED_FindGlobal(keyname);
        if (!key) {
            Con_Printf("'%s' is not a global\n", keyname);
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
    int l = strlen(string) + 1;
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
    TypeLess_ptr d = (TypeLess_ptr)((int*)base + key->ofs);

    switch (key->type & ~DEF_SAVEGLOBAL) {
    case ev_string:     *(string_t*)d = ED_NewString(s) - pr_strings;       break;

    case ev_float:      *(float_p)d = atof(s);                              break;

    case ev_vector:
        char string[128];
        strcpy(string, s);
        cString v = string;
        cString w = string;
        for (int i = 0; i < 3; i++) {
            while (*v && *v != ' ')
                v++;
            *v = 0;
            ((float_p)d)[i] = atof(w);
            w = v = v + 1;
        }
        break;

    case ev_entity:     *(int*)d = EDICT_TO_PROG(EDICT_NUM(atoi(s)));       break;

    case ev_field:
        dDef_p def = ED_FindField(s);
        if (!def) {
            Con_Printf("Can't find field %s\n", s);
            return false;
        }
        *(int*)d = G_INT(def->ofs);
        break;

    case ev_function:
        dFunction_p func = ED_FindFunction(s);
        if (!func) {
            Con_Printf("Can't find function %s\n", s);
            return false;
        }
        *(func_t*)d = func - pr_functions;
        break;

    default:        break;
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
    if (ent != sv.edicts) // hack
        memset(&ent->v, 0, progs->entityfields * 4);

    // go through all the dictionary pairs
    while (1) {
        // parse key
        data = COM_Parse(data);
        if (com.token[0] == '}')    break;
        if (!data)                  Sys_Error("ED_ParseEntity: EOF without closing brace");

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
        int n = strlen(keyname);
        while (n && keyname[n - 1] == ' ') {
            keyname[n - 1] = 0;
            n--;
        }

        // parse value
        data = COM_Parse(data);
        if (!data)                  Sys_Error("ED_ParseEntity: EOF without closing brace");
        if (com.token[0] == '}')    Sys_Error("ED_ParseEntity: closing brace without data");

        init = true;

        // keynames with a leading underscore are used for utility comments,
        // and are immediately discarded by quake
        if (keyname[0] == '_')
            continue;

        dDef_p key = ED_FindField(keyname);
        if (!key) {
            Con_Printf("'%s' is not a field\n", keyname);
            continue;
        }

        if (anglehack) {
            char temp[32];
            strcpy(temp, com.token);
            sprintf(com.token, "0 %s 0", temp);
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
    pr_global_struct->time = sv.time;

    // parse ents
    while (1) {
        // parse the opening brace
        data = COM_Parse(data);
        if (!data)  break;
        if (com.token[0] != '{')    Sys_Error("ED_LoadFromFile: found %s when expecting {", com.token);

        if (!ent)       ent = EDICT_NUM(0);
        else            ent = ED_Alloc();
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
            Con_Printf("No classname for:\n");
            ED_Print(ent);
            ED_Free(ent);
            continue;
        }

        // look for the spawn function
        dFunction_p func = ED_FindFunction(pr_strings + ent->v.classname);

        if (!func) {
            Con_Printf("No spawn function for:\n");
            ED_Print(ent);
            ED_Free(ent);
            continue;
        }

        pr_global_struct->self = EDICT_TO_PROG(ent);
        PR_ExecuteProgram(func - pr_functions);
    }

    Con_DPrintf("%i entities inhibited\n", inhibit);
}


/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs() {
    // flush the non-C variable lookup cache
    for (int i = 0; i < GEFV_CACHESIZE; i++)
        _gefvCache[i].field[0] = 0;

    CRC_Init(&pr_crc);

    progs = (dprograms_p)COM_LoadHunkFile("progs.dat");
    if (!progs)                             Sys_Error("PR_LoadProgs: couldn't load progs.dat");

    Con_DPrintf("Programs occupy %iK.\n", com.filesize / 1024);

    for (int i = 0; i < com.filesize; i++)
        CRC_ProcessByte(&pr_crc, ((uint8_p)progs)[i]);

    // byte swap the header
    for (int i = 0; i < sizeof(*progs) / 4; i++)
        ((int*)progs)[i] = LittleLong(((int*)progs)[i]);

    if (progs->version != PROG_VERSION)     Sys_Error("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
    if (progs->crc != PROGHEADER_CRC)       Sys_Error("progs.dat system vars have been modified, progdefs.h is out of date");

    pr_functions = (dFunction_p)((uint8_p)progs + progs->functions.ofs);
    pr_strings = (cString)progs + progs->strings.ofs;
    pr_globaldefs = (dDef_p)((uint8_p)progs + progs->globaldefs.ofs);
    pr_fielddefs = (dDef_p)((uint8_p)progs + progs->fielddefs.ofs);
    pr_statements = (dStatement_p)((uint8_p)progs + progs->statements.ofs);

    pr_global_struct = (globalvars_p)((uint8_p)progs + progs->globals.ofs);
    pr_globals = (float_p)pr_global_struct;

    pr_edict_size = progs->entityfields * 4 + sizeof(edict_t) - sizeof(entvars_t);

    // uint8_t swap the lumps
    for (int i = 0; i < progs->statements.num; i++) {
        pr_statements[i].op = LittleShort(pr_statements[i].op);
        pr_statements[i].a = LittleShort(pr_statements[i].a);
        pr_statements[i].b = LittleShort(pr_statements[i].b);
        pr_statements[i].c = LittleShort(pr_statements[i].c);
    }

    for (int i = 0; i < progs->functions.num; i++) {
        pr_functions[i].first_statement = LittleLong(pr_functions[i].first_statement);
        pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
        pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
        pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
        pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
        pr_functions[i].locals = LittleLong(pr_functions[i].locals);
    }

    for (int i = 0; i < progs->globaldefs.num; i++) {
        pr_globaldefs[i].type = LittleShort(pr_globaldefs[i].type);
        pr_globaldefs[i].ofs = LittleShort(pr_globaldefs[i].ofs);
        pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
    }

    for (int i = 0; i < progs->fielddefs.num; i++) {
        pr_fielddefs[i].type = LittleShort(pr_fielddefs[i].type);
        if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)      Sys_Error("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");

        pr_fielddefs[i].ofs = LittleShort(pr_fielddefs[i].ofs);
        pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
    }

    for (int i = 0; i < progs->globals.num; i++)
        ((int32_t*)pr_globals)[i] = LittleLong(((int32_t*)pr_globals)[i]);
}


/*
===============
PR_Init
===============
*/
void PR_Init() {
    Cmd_AddCommand("edict", ED_PrintEdict_f);
    Cmd_AddCommand("edicts", ED_PrintEdicts);
    Cmd_AddCommand("edictcount", ED_Count);
    Cmd_AddCommand("profile", PR_Profile_f);
    Cvar_RegisterVariable(&nomonsters);
    Cvar_RegisterVariable(&gamecfg);
    Cvar_RegisterVariable(&scratch1);
    Cvar_RegisterVariable(&scratch2);
    Cvar_RegisterVariable(&scratch3);
    Cvar_RegisterVariable(&scratch4);
    Cvar_RegisterVariable(&savedgamecfg);
    Cvar_RegisterVariable(&saved1);
    Cvar_RegisterVariable(&saved2);
    Cvar_RegisterVariable(&saved3);
    Cvar_RegisterVariable(&saved4);
}

edict_p EDICT_NUM(int n) {
    if ((n < 0) ||
        (n >= sv.max_edicts))
        Sys_Error("EDICT_NUM: bad number %i", n);

    return (edict_p)((uint8_p)sv.edicts + ((n)*pr_edict_size));
}

int NUM_FOR_EDICT(edict_p edict) {
    int b = ((uint8_p)edict - (uint8_p)sv.edicts) / pr_edict_size;

    if ((b < 0) ||
        (b >= sv.num_edicts))
        Sys_Error("NUM_FOR_EDICT: bad pointer");

    return b;
}
