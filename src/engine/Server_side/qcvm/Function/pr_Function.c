#include "pr_Function.h"
#include <string.h>
#include "endian_tools.h"

dFunction_p pr_functions;
dFunction_p pr_xFunction;

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


void initProgFunction(TypeLess_ptr base, progLump_t pl) {
    pr_functions = (dFunction_p)((uint8_p)base + pl.ofs);
    for (int i = 0; i < pl.num; i++) {
        pr_functions[i].first_statement = LittleLong(pr_functions[i].first_statement);
        pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
        pr_functions[i].locals = LittleLong(pr_functions[i].locals);

        pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
        pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);

        pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
    }

}