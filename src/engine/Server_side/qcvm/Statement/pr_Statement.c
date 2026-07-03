#include "pr_Statement.h"
#include "pr_ops.h"
#include "endian_tools.h"
#include "console.h"

dStatement_p   pr_statements;


void initProgStatement(TypeLess_ptr base, progLump_t pl) {
    pr_statements = (dStatement_p)((uint8_p)base + pl.ofs);
    // uint8_t swap the lumps
    for (int i = 0; i < pl.num; i++) {
        pr_statements[i].op = (op_type)LittleShort((int16_t)pr_statements[i].op);
        pr_statements[i].a = LittleShort(pr_statements[i].a);
        pr_statements[i].b = LittleShort(pr_statements[i].b);
        pr_statements[i].c = LittleShort(pr_statements[i].c);
    }
}


/*
    =================
    PR_PrintStatement
    =================
*/
void PR_PrintStatement(dStatement_p state) {
    // if ((uint32_t)state->op < (sizeof(_pr_opNames) / sizeof(_pr_opNames[0]))) {
    PR_PrintOperation(state->op);

#if 0
    if ((state->op == OP_IF) ||
        (state->op == OP_IFNOT)
        )
        Con_Printf("%sbranch %i", PR_GlobalString(state->a), state->b);
    else if (state->op == OP_GOTO)
        Con_Printf("branch %i", state->a);
    else if ((uint32_t)(state->op - OP_STORE_F) < 6) {
        Con_Printf("%s", PR_GlobalString(state->a));
        Con_Printf("%s", PR_GlobalStringNoContents(state->b));
    }
    else {
        if (state->a)   Con_Printf("%s", PR_GlobalString(state->a));
        if (state->b)   Con_Printf("%s", PR_GlobalString(state->b));
        if (state->c)   Con_Printf("%s", PR_GlobalStringNoContents(state->c));
    }
#else
    switch (state->op) {
    case OP_IF:
    case OP_IFNOT: { Con_Printf("%sbranch %i", PR_GlobalString(state->a), state->b); } break;
    case OP_GOTO: { Con_Printf("branch %i", state->a); } break;
    case OP_STORE_F:
    case OP_STORE_V:
    case OP_STORE_S:
    case OP_STORE_ENT:
    case OP_STORE_FLD:
    case OP_STORE_FNC: {
        Con_Printf("%s", PR_GlobalString(state->a));
        Con_Printf("%s", PR_GlobalStringNoContents(state->b));
    } break;
    default: {
        if (state->a)   Con_Printf("%s", PR_GlobalString(state->a));
        if (state->b)   Con_Printf("%s", PR_GlobalString(state->b));
        if (state->c)   Con_Printf("%s", PR_GlobalStringNoContents(state->c));
    } break;
    }
#endif
    Con_Printf("\n");
}


dStatement_p PR_GetStack(int32_t stack) {
    return &pr_statements[stack];
}