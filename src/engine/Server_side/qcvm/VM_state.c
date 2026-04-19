#include "progs.h"
#include "pr_Statment.h"
#include "pr_Function.h"
#include "Edict.h"
#include "common.h"
#include "crc.h"
#include "console.h"
#include "host.h"
#include "endian_tools.h"
#include "cmd.h"
#include "pr_qString.h"

#include "cvar_q1.h"

dprograms_p    progs;

globalvars_p   pr_global_struct;   // much more
float_p        pr_globals;         // same as pr_global_struct

uint16_t pr_crc;

gefv_cache gefvCache[GEFV_CACHESIZE] = {
    {NULL, ""},
    {NULL, ""}
};

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

    initProgSrting(progs);

    pr_global_struct = (globalvars_p)((uint8_p)progs + progs->globals.ofs);
    pr_globals = (float_p)pr_global_struct;

    pr_edict_size = progs->entityfields * 4 + sizeof(edict_t) - sizeof(entvars_t);

    pr_statements = (dStatement_p)((uint8_p)progs + progs->statements.ofs);
    // uint8_t swap the lumps
    for (int i = 0; i < progs->statements.num; i++) {
        pr_statements[i].op = (op_type)LittleShort((int16_t)pr_statements[i].op);
        pr_statements[i].a = LittleShort(pr_statements[i].a);
        pr_statements[i].b = LittleShort(pr_statements[i].b);
        pr_statements[i].c = LittleShort(pr_statements[i].c);
    }


    pr_functions = (dFunction_p)((uint8_p)progs + progs->functions.ofs);
    for (int i = 0; i < progs->functions.num; i++) {
        pr_functions[i].first_statement = LittleLong(pr_functions[i].first_statement);
        pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
        pr_functions[i].locals = LittleLong(pr_functions[i].locals);

        pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
        pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
        pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
    }


    pr_globaldefs = (dDef_p)((uint8_p)progs + progs->globaldefs.ofs);
    for (int i = 0; i < progs->globaldefs.num; i++) {
        pr_globaldefs[i].type = (op_type)LittleShort((int16_t)pr_globaldefs[i].type);
        pr_globaldefs[i].ofs = (uint16_t)LittleShort((int16_t)pr_globaldefs[i].ofs);
        pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
    }


    pr_fielddefs = (dDef_p)((uint8_p)progs + progs->fielddefs.ofs);
    for (int i = 0; i < progs->fielddefs.num; i++) {
        pr_fielddefs[i].type = (op_type)LittleShort((int16_t)pr_fielddefs[i].type);
        if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)      Host_SysError("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");

        pr_fielddefs[i].ofs = (uint16_t)LittleShort((int16_t)pr_fielddefs[i].ofs);
        pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
    }

    for (int i = 0; i < progs->globals.num; i++)
        ((int32_p)pr_globals)[i] = LittleLong(((int32_p)pr_globals)[i]);
}


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
