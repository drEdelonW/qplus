#include "pr_Function.h"
#include <string.h>

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