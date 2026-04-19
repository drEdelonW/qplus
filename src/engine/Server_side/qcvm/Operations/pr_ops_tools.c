#include "progs.h"
#include "pr_def.h"
#include "pr_ops.h"
#include "pr_Argument.h"
#include "types.h"
#include "console.h"
#include <stdio.h>
#include <string.h>

static cString _pr_opNames[OP_LAST] = {
    "DONE",

    "MUL_F",
    "MUL_V",
    "MUL_FV",
    "MUL_VF",

    "DIV",

    "ADD_F",
    "ADD_V",

    "SUB_F",
    "SUB_V",

    "EQ_F",
    "EQ_V",
    "EQ_S",
    "EQ_E",
    "EQ_FNC",

    "NE_F",
    "NE_V",
    "NE_S",
    "NE_E",
    "NE_FNC",

    "LE",
    "GE",
    "LT",
    "GT",
#if 0
    "INDIRECT",
    "INDIRECT",
    "INDIRECT",
    "INDIRECT",
    "INDIRECT",
    "INDIRECT",
#else
    "LOAD_F",
    "LOAD_V",
    "LOAD_S",
    "LOAD_ENT",
    "LOAD_FLD",
    "LOAD_FNC",
#endif
    "ADDRESS",

    "STORE_F",
    "STORE_V",
    "STORE_S",
    "STORE_ENT",
    "STORE_FLD",
    "STORE_FNC",

    "STOREP_F",
    "STOREP_V",
    "STOREP_S",
    "STOREP_ENT",
    "STOREP_FLD",
    "STOREP_FNC",

    "RETURN",

    "NOT_F",
    "NOT_V",
    "NOT_S",
    "NOT_ENT",
    "NOT_FNC",

    "IF",
    "IFNOT",

    "CALL0",
    "CALL1",
    "CALL2",
    "CALL3",
    "CALL4",
    "CALL5",
    "CALL6",
    "CALL7",
    "CALL8",

    "STATE",

    "GOTO",

    "AND",
    "OR",

    "BITAND",
    "BITOR"
};

void PR_PrintOperation(prog_operation_e op){
    if (op < OP_LAST) {
        Con_Printf("%s ", _pr_opNames[op]);
        size_t i = strlen(_pr_opNames[op]);
        for (; i < 10; i++)
            Con_Printf(" ");
    }
}


dDef_p ED_GlobalAtOfs(int ofs);
cString PR_ValueString(etype_t type, eval_p val);



/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
cString PR_GlobalString(int32_t ofs) {
    static char _line[128];

    TypeLess_ptr val = (TypeLess_ptr)&pr_globals[ofs];
    dDef_p def = ED_GlobalAtOfs(ofs);
    if (!def)
        snprintf(_line, sizeof(_line), "%i(???)", ofs);
    else {
        cString s = PR_ValueString(def->type, val);
        snprintf(_line, sizeof(_line), "%i(%s)%s", ofs, PR_GetQString(def->s_name), s);
    }

    size_t i = strlen(_line);
    for (; i < 20; i++)
        strcat(_line, " ");
    strcat(_line, " ");

    return _line;
}

cString PR_GlobalStringNoContents(int32_t ofs) {
    static char _line[128];

    dDef_p def = ED_GlobalAtOfs(ofs);
    if (!def)   snprintf(_line, sizeof(_line), "%i(???)", ofs);
    else        snprintf(_line, sizeof(_line), "%i(%s)", ofs, PR_GetQString(def->s_name));

    size_t i = strlen(_line);
    for (; i < 20; i++)
        strcat(_line, " ");
    strcat(_line, " ");

    return _line;
}
