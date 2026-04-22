#include "pr_def.h"
#include "pr_Statement.h"
#include <string.h>
#include "common.h"
#include "host.h"
#include "enginedefs.h"
#include "Edict.h"
#include "endian_tools.h"

dDef_p         pr_fielddefs;
dDef_p         pr_globaldefs;

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


#define MAX_FIELD_LEN (64)
#define GEFV_CACHESIZE (2)

typedef struct {
    dDef_p  pcache;
    char    field[MAX_FIELD_LEN];
} gefv_cache;

gefv_cache gefvCache[GEFV_CACHESIZE] = {
    {NULL, ""},
    {NULL, ""}
};
static int _rep = 0;
// extern gefv_cache   gefvCache[GEFV_CACHESIZE];

void ED_InitCache() {
    _rep = 0;
    for (int i = 0; i < GEFV_CACHESIZE; i++)
        gefvCache[i].field[0] = 0;
}

dDef_p ED_FindFieldCached(cString field) {

    dDef_p def = NULL;
    for (int i = 0; i < GEFV_CACHESIZE; i++) {
        if (!strcmp(field, gefvCache[i].field)) {
            return gefvCache[i].pcache;
        }
    }

    def = ED_FindField(field);

    if (strlen(field) < MAX_FIELD_LEN) {
        gefvCache[_rep].pcache = def;
        strcpy(gefvCache[_rep].field, field);
        _rep ^= 1;
    }

   return def;
}

void initProgDefs(TypeLess_ptr base, progLump_t plg, progLump_t plf) {
     ED_InitCache();

    // ======[Global Defs]======
    pr_globaldefs = (dDef_p)((uint8_p)base + plg.ofs);
    for (int i = 0; i < plg.num; i++) {
        pr_globaldefs[i].type = (op_type)LittleShort((int16_t)pr_globaldefs[i].type);
        pr_globaldefs[i].ofs = (uint16_t)LittleShort((int16_t)pr_globaldefs[i].ofs);
        pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
    }

    // ======[File Defs]======
    pr_fielddefs = (dDef_p)((uint8_p)base + plf.ofs);
    for (int i = 0; i < plf.num; i++) {
        pr_fielddefs[i].type = (op_type)LittleShort((int16_t)pr_fielddefs[i].type);
        pr_fielddefs[i].ofs = (uint16_t)LittleShort((int16_t)pr_fielddefs[i].ofs);
        pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);

        if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)      Host_SysError("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
    }
}
