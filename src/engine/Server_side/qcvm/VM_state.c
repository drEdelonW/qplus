#include "progs.h"
#include "host.h"
#include "common.h"
#include "console.h"
#include "crc.h"
#include "endian_tools.h"
#include "GlobVars.h"
#include "pr_Statement.h"
#include "pr_def.h"
#include "Edict.h"
#include "cmd.h"
// #include "pr_Function.h"
// #include "pr_qString.h"

#include "cvar_q1.h"

dprograms_p progs;
uint16_t    pr_crc;

/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs() {
    // ======[Prog.DAT]======
    progs = (dprograms_p)COM_LoadHunkFile("progs.dat");
    if (!progs)             Host_SysError("PR_LoadProgs: couldn't load progs.dat");
    else                    Con_DPrintf("Programs occupy %iK.\n", com.filesize / 1024);

    // ======[CRC]======
    CRC_Init(&pr_crc);
    for (int i = 0; i < com.filesize; i++)
        CRC_ProcessByte(&pr_crc, ((uint8_p)progs)[i]);

    // byte swap the header
    for (int i = 0; i < sizeof(*progs) / 4; i++)
        ((int32_p)progs)[i] = LittleLong(((int32_p)progs)[i]);

    if (progs->version != PROG_VERSION)     Host_SysError("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
    if (progs->crc != PROGHEADER_CRC)       Host_SysError("progs.dat system vars have been modified, progdefs.h is out of date");

    initProgString(progs, progs->strings);                      // ======[Prog Strings]======
    initProgGlobals(progs, progs->globals);                     // ======[Global Struct]======
    initProgStatement(progs, progs->statements);                // ======[Statements]======
    initProgFunction(progs, progs->functions);                  // ======[Function]======
    initProgDefs(progs, progs->globaldefs, progs->fielddefs);   // ======[Global Defs]======

    EdictSize = PROG_HEADER_SIZE + progs->entityfields * 4;     // ======[Edict Size]======
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
