#pragma once

#include "types.h"
#include "VM_operation.h"   // prog_operation_t OP_LAST
#include "VM_types.h"       // def_p
#include "VM_priority.h"    // Priority_t
typedef struct {
    cStr_p      name;
    cStr_p      opname;
    Priority_t  priority;
    bool        right_associative;
    def_p       type_a;
    def_p       type_b;
    def_p       type_c;
} opcode_t;
typedef opcode_t* opcode_p;

extern opcode_t pr_opcodes[OP_LAST + 1];  // sized by initialization
def_p PR_Statement(opcode_p op, def_p var_a, def_p var_b);
