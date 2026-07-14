#include "pr_Statement.h"
#include "pr_ops.h"
#include "endian_tools.h"
#include "console.h"

static dStatement_p _ProcState;
void initProgStatement(progLump_t pl) {
    _ProcState = GetPtrFromLump(pl);
    // uint8_t swap the lumps
    for (int i = 0; i < pl.num; i++) {
        _ProcState[i].op = (op_type)LittleShort((int16_t)_ProcState[i].op);
        _ProcState[i].a = LittleShort(_ProcState[i].a);
        _ProcState[i].b = LittleShort(_ProcState[i].b);
        _ProcState[i].c = LittleShort(_ProcState[i].c);
    }
}


void PR_PrintStatement(dStatement_p state) {
    PR_PrintOperation(state->op);
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
    Con_Printf("\n");
}


dStatement_p PR_GetStack(int32_t stack) {
    return &_ProcState[stack];
}