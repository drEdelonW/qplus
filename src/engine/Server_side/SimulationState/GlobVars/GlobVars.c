#include "GlobVars.h"
// #include "progdefs.h"
#include "endian_tools.h"

globalvars_p   pr_global_struct;   // much more
float_p        pr_globals;         // same as pr_global_struct


void initProgGlobals(progLump_t pl) {
    pr_global_struct = GetPtrFromLump(pl);
    pr_globals = GetPtrFromLump(pl);
    for (int i = 0; i < pl.num; i++)
        pr_globals[i] = LittleFloat(pr_globals[i]);
}