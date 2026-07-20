
#include "types.h"          // cStringRO
#include "VM_operation.h"   // prog_operation_t
static cStringRO _pr_opNames[OP_LAST] = {
    [OP_DONE] = "DONE",

    [OP_MUL_F] = "MUL_F",
    [OP_MUL_V] = "MUL_V",
    [OP_MUL_FV] = "MUL_FV",
    [OP_MUL_VF] = "MUL_VF",

    [OP_DIV_F] = "DIV",

    [OP_ADD_F] = "ADD_F",
    [OP_ADD_V] = "ADD_V",

    [OP_SUB_F] = "SUB_F",
    [OP_SUB_V] = "SUB_V",

    [OP_EQ_F] = "EQ_F",
    [OP_EQ_V] = "EQ_V",
    [OP_EQ_S] = "EQ_S",
    [OP_EQ_E] = "EQ_E",
    [OP_EQ_FNC] = "EQ_FNC",

    [OP_NE_F] = "NE_F",
    [OP_NE_V] = "NE_V",
    [OP_NE_S] = "NE_S",
    [OP_NE_E] = "NE_E",
    [OP_NE_FNC] = "NE_FNC",

    [OP_LE] = "LE",
    [OP_GE] = "GE",
    [OP_LT] = "LT",
    [OP_GT] = "GT",
#if 0
    [OP_LOAD_F] = "INDIRECT",
    [OP_LOAD_V] = "INDIRECT",
    [OP_LOAD_S] = "INDIRECT",
    [OP_LOAD_ENT] = "INDIRECT",
    [OP_LOAD_FLD] = "INDIRECT",
    [OP_LOAD_FNC] = "INDIRECT",
#else
    [OP_LOAD_F] = "LOAD_F",
    [OP_LOAD_V] = "LOAD_V",
    [OP_LOAD_S] = "LOAD_S",
    [OP_LOAD_ENT] = "LOAD_ENT",
    [OP_LOAD_FLD] = "LOAD_FLD",
    [OP_LOAD_FNC] = "LOAD_FNC",
#endif
    [OP_ADDRESS] = "ADDRESS",

    [OP_STORE_F] = "STORE_F",
    [OP_STORE_V] = "STORE_V",
    [OP_STORE_S] = "STORE_S",
    [OP_STORE_ENT] = "STORE_ENT",
    [OP_STORE_FLD] = "STORE_FLD",
    [OP_STORE_FNC] = "STORE_FNC",

    [OP_STOREP_F] = "STOREP_F",
    [OP_STOREP_V] = "STOREP_V",
    [OP_STOREP_S] = "STOREP_S",
    [OP_STOREP_ENT] = "STOREP_ENT",
    [OP_STOREP_FLD] = "STOREP_FLD",
    [OP_STOREP_FNC] = "STOREP_FNC",

    [OP_RETURN] = "RETURN",

    [OP_NOT_F] = "NOT_F",
    [OP_NOT_V] = "NOT_V",
    [OP_NOT_S] = "NOT_S",
    [OP_NOT_ENT] = "NOT_ENT",
    [OP_NOT_FNC] = "NOT_FNC",

    [OP_IF] = "IF",
    [OP_IFNOT] = "IFNOT",

    [OP_CALL0] = "CALL0",
    [OP_CALL1] = "CALL1",
    [OP_CALL2] = "CALL2",
    [OP_CALL3] = "CALL3",
    [OP_CALL4] = "CALL4",
    [OP_CALL5] = "CALL5",
    [OP_CALL6] = "CALL6",
    [OP_CALL7] = "CALL7",
    [OP_CALL8] = "CALL8",

    [OP_STATE] = "STATE",

    [OP_GOTO] = "GOTO",

    [OP_AND] = "AND",
    [OP_OR] = "OR",

    [OP_BITAND] = "BITAND",
    [OP_BITOR] = "BITOR"
};

#include "console.h"    // Con_Printf
#include <string.h>     // strlen
void PR_PrintOperation(prog_operation_t op) {
    if (op < OP_LAST) {
        Con_Printf("%s ", _pr_opNames[op]);
        size_t i = strlen(_pr_opNames[op]);
        for (; i < 10; i++)
            Con_Printf(" ");
    }
}

#include "progs.h"
#include "GlobVars.h"
#include "pr_ops.h"
#include "vmValue.h"
#include <stdio.h>

#include "pr_def.h" // dDef_p
dDef_p ED_GlobalAtOfs(int ofs);
cString PR_ValueString(etype_t type, eval_p val);



/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
static char _line[128];
cString PR_GlobalString(int32_t ofs) {
    eval_p val = GV_pEval(ofs);
    dDef_p def = ED_GlobalAtOfs(ofs);
    if (!def)
        snprintf(_line,
            sizeof(_line),
            "%i(???)",
            ofs
        );
    else
        snprintf(_line,
            sizeof(_line),
            "%i(%s)%s",
            ofs, PR_GetQString(def->s_name), PR_ValueString(def->type, val)
        );

    size_t i = strlen(_line);
    for (; i < 20; i++)
        strcat(_line, " ");
    strcat(_line, " ");

    return _line;
}

cString PR_GlobalStringNoContents(int32_t ofs) {
    dDef_p def = ED_GlobalAtOfs(ofs);
    if (!def)   snprintf(_line, sizeof(_line), "%i(???)", ofs);
    else        snprintf(_line, sizeof(_line), "%i(%s)", ofs, PR_GetQString(def->s_name));

    size_t i = strlen(_line);
    for (; i < 20; i++)
        strcat(_line, " ");
    strcat(_line, " ");

    return _line;
}
