#pragma once

typedef enum {
    PrioNone     = -1, // not an infix operator for PR_Expression: DONE, RETURN, NOT_*, IF, IFNOT, CALL0..CALL8, STATE, GOTO
    PrioTerm     = 0,  // PR_Term() base case: literal / identifier / "(" expr ")" - no opcode row, not in pr_opcodes[]
    PrioAccess   = 1,  // '.' field load or address-of (LOAD_*, ADDRESS); also postfix call "(" handled as special case at this level in PR_Expression
    PrioMulDiv   = 2,  // '*' '/' '&' '|'  - MUL_*, DIV_F, BITAND, BITOR
    PrioAddSub   = 3,  // '+' '-'          - ADD_*, SUB_*
    PrioCompare  = 4,  // '==' '!=' '<=' '>=' '<' '>' - EQ_*, NE_*, LE, GE, LT, GT
    PrioAssign   = 5,  // '=' right-associative - STORE_*, STOREP_*
    PrioLogic    = 6,  // '&&' '||'        - AND, OR; entry point for a full expression

    // legacy names, kept for existing call sites until grepped:
    NOT_PRIORITY = PrioCompare,  // [unverified] operand priority for unary '!' inside PR_Term - body not seen this session
    TOP_PRIORITY = PrioLogic,
} Priority_t;


#include "VM_types.h"       // def_p
def_p PR_Expression(Priority_t priority);
