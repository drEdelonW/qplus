
#include "VM_opcode.h"


opcode_t pr_opcodes[] = {
    [OP_DONE] = {"<DONE>", "DONE", PrioNone, false, &def_entity, &def_field, &def_void},

    [OP_MUL_F] = {"*", "MUL_F", PrioMulDiv, false, &def_float, &def_float, &def_float},
    [OP_MUL_V] = {"*", "MUL_V", PrioMulDiv, false, &def_vector, &def_vector, &def_float},
    [OP_MUL_FV] = {"*", "MUL_FV", PrioMulDiv, false, &def_float, &def_vector, &def_vector},
    [OP_MUL_VF] = {"*", "MUL_VF", PrioMulDiv, false, &def_vector, &def_float, &def_vector},

    [OP_DIV_F] = {"/", "DIV", PrioMulDiv, false, &def_float, &def_float, &def_float},

    [OP_ADD_F] = {"+", "ADD_F", PrioAddSub, false, &def_float, &def_float, &def_float},
    [OP_ADD_V] = {"+", "ADD_V", PrioAddSub, false, &def_vector, &def_vector, &def_vector},

    [OP_SUB_F] = {"-", "SUB_F", PrioAddSub, false, &def_float, &def_float, &def_float},
    [OP_SUB_V] = {"-", "SUB_V", PrioAddSub, false, &def_vector, &def_vector, &def_vector},

    [OP_EQ_F] = {"==", "EQ_F", PrioCompare, false, &def_float, &def_float, &def_float},
    [OP_EQ_V] = {"==", "EQ_V", PrioCompare, false, &def_vector, &def_vector, &def_float},
    [OP_EQ_S] = {"==", "EQ_S", PrioCompare, false, &def_string, &def_string, &def_float},
    [OP_EQ_E] = {"==", "EQ_E", PrioCompare, false, &def_entity, &def_entity, &def_float},
    [OP_EQ_FNC] = {"==", "EQ_FNC", PrioCompare, false, &def_function, &def_function, &def_float},

    [OP_NE_F] = {"!=", "NE_F", PrioCompare, false, &def_float, &def_float, &def_float},
    [OP_NE_V] = {"!=", "NE_V", PrioCompare, false, &def_vector, &def_vector, &def_float},
    [OP_NE_S] = {"!=", "NE_S", PrioCompare, false, &def_string, &def_string, &def_float},
    [OP_NE_E] = {"!=", "NE_E", PrioCompare, false, &def_entity, &def_entity, &def_float},
    [OP_NE_FNC] = {"!=", "NE_FNC", PrioCompare, false, &def_function, &def_function, &def_float},

    [OP_LE] = {"<=", "LE", PrioCompare, false, &def_float, &def_float, &def_float},
    [OP_GE] = {">=", "GE", PrioCompare, false, &def_float, &def_float, &def_float},
    [OP_LT] = {"<", "LT", PrioCompare, false, &def_float, &def_float, &def_float},
    [OP_GT] = {">", "GT", PrioCompare, false, &def_float, &def_float, &def_float},

    [OP_LOAD_F] = {".", "INDIRECT", PrioAccess, false, &def_entity, &def_field, &def_float},
    [OP_LOAD_V] = {".", "INDIRECT", PrioAccess, false, &def_entity, &def_field, &def_vector},
    [OP_LOAD_S] = {".", "INDIRECT", PrioAccess, false, &def_entity, &def_field, &def_string},
    [OP_LOAD_ENT] = {".", "INDIRECT", PrioAccess, false, &def_entity, &def_field, &def_entity},
    [OP_LOAD_FLD] = {".", "INDIRECT", PrioAccess, false, &def_entity, &def_field, &def_field},
    [OP_LOAD_FNC] = {".", "INDIRECT", PrioAccess, false, &def_entity, &def_field, &def_function},

    [OP_ADDRESS] = {".", "ADDRESS", PrioAccess, false, &def_entity, &def_field, &def_pointer},

    [OP_STORE_F] = {"=", "STORE_F", PrioAssign, true, &def_float, &def_float, &def_float},
    [OP_STORE_V] = {"=", "STORE_V", PrioAssign, true, &def_vector, &def_vector, &def_vector},
    [OP_STORE_S] = {"=", "STORE_S", PrioAssign, true, &def_string, &def_string, &def_string},
    [OP_STORE_ENT] = {"=", "STORE_ENT", PrioAssign, true, &def_entity, &def_entity, &def_entity},
    [OP_STORE_FLD] = {"=", "STORE_FLD", PrioAssign, true, &def_field, &def_field, &def_field},
    [OP_STORE_FNC] = {"=", "STORE_FNC", PrioAssign, true, &def_function, &def_function, &def_function},

    [OP_STOREP_F] = {"=", "STOREP_F", PrioAssign, true, &def_pointer, &def_float, &def_float},
    [OP_STOREP_V] = {"=", "STOREP_V", PrioAssign, true, &def_pointer, &def_vector, &def_vector},
    [OP_STOREP_S] = {"=", "STOREP_S", PrioAssign, true, &def_pointer, &def_string, &def_string},
    [OP_STOREP_ENT] = {"=", "STOREP_ENT", PrioAssign, true, &def_pointer, &def_entity, &def_entity},
    [OP_STOREP_FLD] = {"=", "STOREP_FLD", PrioAssign, true, &def_pointer, &def_field, &def_field},
    [OP_STOREP_FNC] = {"=", "STOREP_FNC", PrioAssign, true, &def_pointer, &def_function, &def_function},

    [OP_RETURN] = {"<RETURN>", "RETURN", PrioNone, false, &def_void, &def_void, &def_void},

    [OP_NOT_F] = {"!", "NOT_F", PrioNone, false, &def_float, &def_void, &def_float},
    [OP_NOT_V] = {"!", "NOT_V", PrioNone, false, &def_vector, &def_void, &def_float},
    [OP_NOT_S] = {"!", "NOT_S", PrioNone, false, &def_vector, &def_void, &def_float},
    [OP_NOT_ENT] = {"!", "NOT_ENT", PrioNone, false, &def_entity, &def_void, &def_float},
    [OP_NOT_FNC] = {"!", "NOT_FNC", PrioNone, false, &def_function, &def_void, &def_float},

    [OP_IF] = {"<IF>", "IF", PrioNone, false, &def_float, &def_float, &def_void},
    [OP_IFNOT] = {"<IFNOT>", "IFNOT", PrioNone, false, &def_float, &def_float, &def_void},

    // calls returns REG_RETURN
    [OP_CALL0] = {"<CALL0>", "CALL0", PrioNone, false, &def_function, &def_void, &def_void},
    [OP_CALL1] = {"<CALL1>", "CALL1", PrioNone, false, &def_function, &def_void, &def_void},
    [OP_CALL2] = {"<CALL2>", "CALL2", PrioNone, false, &def_function, &def_void, &def_void},
    [OP_CALL3] = {"<CALL3>", "CALL3", PrioNone, false, &def_function, &def_void, &def_void},
    [OP_CALL4] = {"<CALL4>", "CALL4", PrioNone, false, &def_function, &def_void, &def_void},
    [OP_CALL5] = {"<CALL5>", "CALL5", PrioNone, false, &def_function, &def_void, &def_void},
    [OP_CALL6] = {"<CALL6>", "CALL6", PrioNone, false, &def_function, &def_void, &def_void},
    [OP_CALL7] = {"<CALL7>", "CALL7", PrioNone, false, &def_function, &def_void, &def_void},
    [OP_CALL8] = {"<CALL8>", "CALL8", PrioNone, false, &def_function, &def_void, &def_void},

    [OP_STATE] = {"<STATE>", "STATE", PrioNone, false, &def_float, &def_float, &def_void},

    [OP_GOTO] = {"<GOTO>", "GOTO", PrioNone, false, &def_float, &def_void, &def_void},

    [OP_AND] = {"&&", "AND", PrioLogic, false, &def_float, &def_float, &def_float},
    [OP_OR] = {"||", "OR", PrioLogic, false, &def_float, &def_float, &def_float},

    [OP_BITAND] = {"&", "BITAND", PrioMulDiv, false, &def_float, &def_float, &def_float},
    [OP_BITOR] = {"|", "BITOR", PrioMulDiv, false, &def_float, &def_float, &def_float},

    [OP_LAST] = {NULL}
};


