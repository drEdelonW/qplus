#include "progs.h"
#include "edicts.h"
#include "common.h"
#include "crc.h"
#include "console.h"
#include "host.h"
#include "endian_tools.h"
#include "cmd.h"

#include "cvar_q1.h"

dprograms_p    progs;
dFunction_p    pr_functions;
cString        pr_strings;         // much more
dDef_p         pr_fielddefs;
dDef_p         pr_globaldefs;
dStatement_p   pr_statements;
globalvars_p   pr_global_struct;   // much more
float_p        pr_globals;         // same as pr_global_struct
uint32_t       pr_edict_size;      // in bytes

uint16_t pr_crc;



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
    if (!progs)                             Host_SysError("PR_LoadProgs: couldn't load progs.dat");

    Con_DPrintf("Programs occupy %iK.\n", com.filesize / 1024);

    for (int i = 0; i < com.filesize; i++)
        CRC_ProcessByte(&pr_crc, ((uint8_p)progs)[i]);

    // byte swap the header
    for (int i = 0; i < sizeof(*progs) / 4; i++)
        ((int32_p)progs)[i] = LittleLong(((int32_p)progs)[i]);

    if (progs->version != PROG_VERSION)     Host_SysError("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
    if (progs->crc != PROGHEADER_CRC)       Host_SysError("progs.dat system vars have been modified, progdefs.h is out of date");

    pr_functions = (dFunction_p)((uint8_p)progs + progs->functions.ofs);
    pr_strings = (cString)((uint8_p)progs + progs->strings.ofs);
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
        pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
        pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
        pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
        pr_functions[i].locals = LittleLong(pr_functions[i].locals);
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
