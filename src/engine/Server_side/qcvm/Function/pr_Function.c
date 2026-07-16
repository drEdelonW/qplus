#include "pr_Function.h"
#include <string.h>
#include "endian_tools.h"

dFunction_p pr_functions;



dFunction_p ED_FindFunction(cString name) {
    for (int i = 0; i < pProgsDat->functions.num; i++)
        if (!strcmp(PR_GetQString(pr_functions[i].s_name), name))
            return &pr_functions[i];

    return NULL;
}


void initProgFunction(progLump_t pl) {
    pr_functions = GetPtrFromLump(pl);
    for (int i = 0; i < pl.num; i++) {
        pr_functions[i].first_statement = LittleLong(pr_functions[i].first_statement);
        pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
        pr_functions[i].locals = LittleLong(pr_functions[i].locals);

        pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
        pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);

        pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
    }

}

