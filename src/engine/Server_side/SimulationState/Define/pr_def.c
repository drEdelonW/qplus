#include "pr_def.h"
#include "pr_Statement.h"
#include <string.h>
#include "common.h"
#include "host.h"
#include "enginedefs.h"
#include "GlobVars.h"
#include "Edict.h"
#include "endian_tools.h"

dDef_p  pr_fielddefs; // extern //TODO: hide it
static int _GlobalDefsNum = 0;  // globaldefs.num
static int _FieldDefsNum = 0;   // fielddefs.num

int GetGlobalDefsNum() {
    if (!_GlobalDefsNum)    Host_SysError("_GlobalDefsNum empty");
    return _GlobalDefsNum;
}
int GetFieldDefsNum() {
    if (!_FieldDefsNum)    Host_SysError("_FieldDefsNum empty");
    return _FieldDefsNum;
}
/*
==============================================================================

                    ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

static dDef_p  _pGlobalDefs;
void ED_WriteGlobals(FILE* f) {
    fprintf(f, "{\n"); {
        for (int i = 0; i < _GlobalDefsNum; i++) {
            dDef_p def = &_pGlobalDefs[i];
            etype_t type = def->type;
            if (!(def->type & DEF_SAVEGLOBAL)
                )  continue;

            type &= ~DEF_SAVEGLOBAL;

            if ((type != ev_string) &&
                (type != ev_float) &&
                (type != ev_entity)
                )   continue;

            cString name = PR_GetQString(def->s_name);
            fprintf(f, "\"%s\" ", name);
            fprintf(f, "\"%s\"\n", PR_UglyValueString(type, GV_pEval(def->ofs)));
        }
    } fprintf(f, "}\n");
}


void ED_ParseGlobals(cString data) {
    while (1) {
        // parse key
        data = COM_Parse(data);
        if (com.token[0] == '}')    break;
        if (!data)                  Host_SysError("ED_ParseEntity: EOF without closing brace");

        nameStr_t keyname; strcpy(keyname, com.token);

        // parse value
        data = COM_Parse(data);
        if (!data)                  Host_SysError("ED_ParseEntity: EOF without closing brace");
        if (com.token[0] == '}')    Host_SysError("ED_ParseEntity: closing brace without data");

        dDef_p key = ED_FindGlobal(keyname);
        if (!key) {
            Host_Printf("'%s' is not a global\n", keyname);
            continue;
        }

        if (!ED_ParseEpair(GV_pEdict(OFS_NULL), key, com.token))
            Host_Error("ED_ParseGlobals: parse error");
    }
}


dDef_p ED_GlobalAtOfs(int ofs) {
    for (int i = 0; i < _GlobalDefsNum; i++) {
        dDef_p def = &_pGlobalDefs[i];
        if (def->ofs == ofs)
            return def;
    }
    return NULL;
}


dDef_p ED_FindGlobal(cString name) {
    for (int i = 0; i < _GlobalDefsNum; i++) {
        dDef_p def = &_pGlobalDefs[i];
        if (!strcmp(PR_GetQString(def->s_name), name))
            return def;
    }
    return NULL;
}


dDef_p ED_FieldAtOfs(int ofs) {
    for (int i = 0; i < _FieldDefsNum; i++) {
        dDef_p def = &pr_fielddefs[i];
        if (def->ofs == ofs)
            return def;
    }
    return NULL;
}


dDef_p ED_FindField(cString name) {
    for (int i = 0; i < _FieldDefsNum; i++) {
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
    for (int i = 0; i < GEFV_CACHESIZE; i++)
        if (!strcmp(field, gefvCache[i].field))
            return gefvCache[i].pcache;

    dDef_p def = ED_FindField(field);
    if (strlen(field) < MAX_FIELD_LEN) {
        gefvCache[_rep].pcache = def;
        strcpy(gefvCache[_rep].field, field);
        _rep ^= 1;
    }

    return def;
}

void initProgDefs(progLump_t plg, progLump_t plf) {
    ED_InitCache();

    // ======[Global Defs]======
    _pGlobalDefs = GetPtrFromLump(plg);
    _FieldDefsNum = plg.num;
    for (int i = 0; i < _FieldDefsNum; i++) {
        _pGlobalDefs[i].type = (op_type)LittleShort((int16_t)_pGlobalDefs[i].type);
        _pGlobalDefs[i].ofs = (uint16_t)LittleShort((int16_t)_pGlobalDefs[i].ofs);
        _pGlobalDefs[i].s_name = LittleLong(_pGlobalDefs[i].s_name);
    }

    // ======[File Defs]======
    pr_fielddefs = GetPtrFromLump(plf);
    _FieldDefsNum = plf.num;
    for (int i = 0; i < _FieldDefsNum; i++) {
        pr_fielddefs[i].type = (op_type)LittleShort((int16_t)pr_fielddefs[i].type);
        pr_fielddefs[i].ofs = (uint16_t)LittleShort((int16_t)pr_fielddefs[i].ofs);
        pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);

        if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)      Host_SysError("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
    }
}
