#include "progs.h"
#include "Edict.h"
#include "common.h"
#include "crc.h"
#include "console.h"
#include "host.h"
#include "endian_tools.h"
#include "cmd.h"

#include "cvar_q1.h"

dprograms_p    progs;

dFunction_p    pr_functions;
static cString  _pr_strings;         // much more // should be static
dDef_p         pr_fielddefs;
dDef_p         pr_globaldefs;
dStatement_p   pr_statements;
globalvars_p   pr_global_struct;   // much more
float_p        pr_globals;         // same as pr_global_struct

uint16_t pr_crc;



/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
static char _line[256];
cString PR_ValueString(etype_t type, eval_p val) {
    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
        case ev_string:     snprintf(_line, sizeof(_line), "%s", PR_GetQString(val->string));                                       break;
        case ev_entity:     snprintf(_line, sizeof(_line), "entity %i", ED_GetEDictIdx(ED_GetEDictByOffs(val->edict)));                break;
        case ev_function:   snprintf(_line, sizeof(_line), "%s()", PR_GetQString((pr_functions + val->function)->s_name));          break;
        case ev_field:      snprintf(_line, sizeof(_line), ".%s", PR_GetQString(ED_FieldAtOfs(val->_int)->s_name));                 break;
        case ev_void:       snprintf(_line, sizeof(_line), "void");                                                                 break;
        case ev_float:      snprintf(_line, sizeof(_line), "%5.1f", val->_float);                                                   break;
        case ev_vector:     snprintf(_line, sizeof(_line), "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);  break;
        case ev_pointer:    snprintf(_line, sizeof(_line), "pointer");                                                              break;
        default:            snprintf(_line, sizeof(_line), "bad type %i", type);                                                    break;
    }

    return _line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
cString PR_UglyValueString(etype_t type, eval_p val) {
    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
        case ev_string:     snprintf(_line, sizeof(_line), "%s", PR_GetQString(val->string));                           break;
        case ev_entity:     snprintf(_line, sizeof(_line), "%i", ED_GetEDictIdx(ED_GetEDictByOffs(val->edict)));             break;
        case ev_function:   snprintf(_line, sizeof(_line), "%s", PR_GetQString((pr_functions + val->function)->s_name));break;
        case ev_field:      snprintf(_line, sizeof(_line), "%s", PR_GetQString(ED_FieldAtOfs(val->_int)->s_name));      break;
        case ev_void:       snprintf(_line, sizeof(_line), "void");                                                     break;
        case ev_float:      snprintf(_line, sizeof(_line), "%f", val->_float);                                          break;
        case ev_vector:     snprintf(_line, sizeof(_line), "%f %f %f", val->vector[0], val->vector[1], val->vector[2]); break;
        default:            snprintf(_line, sizeof(_line), "bad type %i", type);                                        break;
    }

    return _line;
}


/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs() {
    // flush the non-C variable lookup cache
    for (int i = 0; i < GEFV_CACHESIZE; i++)
        gefvCache[i].field[0] = 0;

    CRC_Init(&pr_crc);

    progs = (dprograms_p)COM_LoadHunkFile("progs.dat");
    if (!progs)             Host_SysError("PR_LoadProgs: couldn't load progs.dat");

    Con_DPrintf("Programs occupy %iK.\n", com.filesize / 1024);

    for (int i = 0; i < com.filesize; i++)
        CRC_ProcessByte(&pr_crc, ((uint8_p)progs)[i]);

    // byte swap the header
    for (int i = 0; i < sizeof(*progs) / 4; i++)
        ((int32_p)progs)[i] = LittleLong(((int32_p)progs)[i]);

    if (progs->version != PROG_VERSION)     Host_SysError("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
    if (progs->crc != PROGHEADER_CRC)       Host_SysError("progs.dat system vars have been modified, progdefs.h is out of date");

    pr_functions = (dFunction_p)((uint8_p)progs + progs->functions.ofs);

    PR_ClearAppStrings();
    _pr_strings = (cString)((uint8_p)progs + progs->strings.ofs);
    pr_globaldefs = (dDef_p)((uint8_p)progs + progs->globaldefs.ofs);
    pr_fielddefs = (dDef_p)((uint8_p)progs + progs->fielddefs.ofs);
    pr_statements = (dStatement_p)((uint8_p)progs + progs->statements.ofs);

    pr_global_struct = (globalvars_p)((uint8_p)progs + progs->globals.ofs);
    pr_globals = (float_p)pr_global_struct;

    pr_edict_size = progs->entityfields * 4 + sizeof(edict_t) - sizeof(entvars_t);

    // uint8_t swap the lumps
    for (int i = 0; i < progs->statements.num; i++) {
        pr_statements[i].op = (op_type)LittleShort((int16_t)pr_statements[i].op);
        pr_statements[i].a = LittleShort(pr_statements[i].a);
        pr_statements[i].b = LittleShort(pr_statements[i].b);
        pr_statements[i].c = LittleShort(pr_statements[i].c);
    }

    for (int i = 0; i < progs->functions.num; i++) {
        pr_functions[i].first_statement = LittleLong(pr_functions[i].first_statement);
        pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
        pr_functions[i].locals = LittleLong(pr_functions[i].locals);

        pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
        pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
        pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
    }

    for (int i = 0; i < progs->globaldefs.num; i++) {
        pr_globaldefs[i].type = (op_type)LittleShort((int16_t)pr_globaldefs[i].type);
        pr_globaldefs[i].ofs = (uint16_t)LittleShort((int16_t)pr_globaldefs[i].ofs);
        pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
    }

    for (int i = 0; i < progs->fielddefs.num; i++) {
        pr_fielddefs[i].type = (op_type)LittleShort((int16_t)pr_fielddefs[i].type);
        if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)      Host_SysError("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");

        pr_fielddefs[i].ofs = (uint16_t)LittleShort((int16_t)pr_fielddefs[i].ofs);
        pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
    }

    for (int i = 0; i < progs->globals.num; i++)
        ((int32_p)pr_globals)[i] = LittleLong(((int32_p)pr_globals)[i]);
}

#if 0
qVmString_t PR_SetQString(cString str) {
#if 0
    return str - _pr_strings;
#else
    ptrdiff_t delta = str - _pr_strings;

    if ((delta > INT32_MAX) || (delta < INT32_MIN)) {
        Host_SysError("PR_SetQString \"%s\": delta out of int32 (%lld) 0x%llX", str, (long long)delta, (long long)delta);
    }

    return (qVmString_t)delta;
#endif
}

cStringRO PR_GetQString(qVmString_t offs) {
    return _pr_strings + offs;
}
#else
typedef int32_t qVmString_t;

#define PR_APPSTR_MAX 4096

static cString      _pr_appstrings[PR_APPSTR_MAX];
static qVmString_t  _pr_appstrings_num = 0;

void PR_ClearAppStrings() {
    for (qVmString_t i = 0; i < _pr_appstrings_num; ++i) {
        _pr_appstrings[i] = NULL;
    }

    _pr_appstrings_num = 0;
}

qVmString_t PR_SetQString(cString str) {
    if (!str)   return 0;

    ptrdiff_t delta = str - _pr_strings;

    if ((delta >= 0) &&
        (delta <= INT32_MAX))
        return (qVmString_t)delta;

    for (qVmString_t i = 0; i < _pr_appstrings_num; ++i)
        if (_pr_appstrings[i] == str)
            return -(i + 1);

    if (_pr_appstrings_num >= PR_APPSTR_MAX)
        Host_SysError("PR_SetQString: app string table overflow");

    qVmString_t idx = _pr_appstrings_num;

    _pr_appstrings[_pr_appstrings_num] = str;
    _pr_appstrings_num++;

    return -(idx + 1);
}

cString PR_GetQString(qVmString_t offs) {
    int32_t idx;

    if (offs >= 0)
        return _pr_strings + offs;

    idx = -offs - 1;

    if ((idx < 0) || (idx >= _pr_appstrings_num))
        Host_SysError("PR_GetQString: bad app string index %d", idx);


    return _pr_appstrings[idx];
}
#endif


/*
===============
PR_Init
===============
*/
void PR_Init() {
    ED_Init();
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
