#include "GlobVars.h"
// #include "progdefs.h"
#include "endian_tools.h"

globalvars_p   pr_global_struct;   // much more
float_p        pr_globals;         // same as pr_global_struct


void initProgGlobals(TypeLess_ptr base, progLump_t pl) {
    pr_global_struct = (globalvars_p)((uint8_p)base + pl.ofs);
    pr_globals = (float_p)pr_global_struct;
    for (int i = 0; i < pl.num; i++)
        ((int32_p)pr_globals)[i] = LittleLong(((int32_p)pr_globals)[i]);

}