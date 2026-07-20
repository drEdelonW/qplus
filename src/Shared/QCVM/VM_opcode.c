
#include "VM_opcode.h"


opcode_t pr_opcodes[] = {
    [OP_DONE] = {"<DONE>", "DONE", -1, false, &def_entity, &def_field, &def_void},

    [OP_MUL_F] = {"*", "MUL_F", 2, false, &def_float, &def_float, &def_float},
    [OP_MUL_V] = {"*", "MUL_V", 2, false, &def_vector, &def_vector, &def_float},
    [OP_MUL_FV] = {"*", "MUL_FV", 2, false, &def_float, &def_vector, &def_vector},
    [OP_MUL_VF] = {"*", "MUL_VF", 2, false, &def_vector, &def_float, &def_vector},

    [OP_DIV_F] = {"/", "DIV", 2, false, &def_float, &def_float, &def_float},

    [OP_ADD_F] = {"+", "ADD_F", 3, false, &def_float, &def_float, &def_float},
    [OP_ADD_V] = {"+", "ADD_V", 3, false, &def_vector, &def_vector, &def_vector},

    [OP_SUB_F] = {"-", "SUB_F", 3, false, &def_float, &def_float, &def_float},
    [OP_SUB_V] = {"-", "SUB_V", 3, false, &def_vector, &def_vector, &def_vector},

    [OP_EQ_F] = {"==", "EQ_F", 4, false, &def_float, &def_float, &def_float},
    [OP_EQ_V] = {"==", "EQ_V", 4, false, &def_vector, &def_vector, &def_float},
    [OP_EQ_S] = {"==", "EQ_S", 4, false, &def_string, &def_string, &def_float},
    [OP_EQ_E] = {"==", "EQ_E", 4, false, &def_entity, &def_entity, &def_float},
    [OP_EQ_FNC] = {"==", "EQ_FNC", 4, false, &def_function, &def_function, &def_float},

    [OP_NE_F] = {"!=", "NE_F", 4, false, &def_float, &def_float, &def_float},
    [OP_NE_V] = {"!=", "NE_V", 4, false, &def_vector, &def_vector, &def_float},
    [OP_NE_S] = {"!=", "NE_S", 4, false, &def_string, &def_string, &def_float},
    [OP_NE_E] = {"!=", "NE_E", 4, false, &def_entity, &def_entity, &def_float},
    [OP_NE_FNC] = {"!=", "NE_FNC", 4, false, &def_function, &def_function, &def_float},

    [OP_LE] = {"<=", "LE", 4, false, &def_float, &def_float, &def_float},
    [OP_GE] = {">=", "GE", 4, false, &def_float, &def_float, &def_float},
    [OP_LT] = {"<", "LT", 4, false, &def_float, &def_float, &def_float},
    [OP_GT] = {">", "GT", 4, false, &def_float, &def_float, &def_float},

    [OP_LOAD_F] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_float},
    [OP_LOAD_V] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_vector},
    [OP_LOAD_S] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_string},
    [OP_LOAD_ENT] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_entity},
    [OP_LOAD_FLD] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_field},
    [OP_LOAD_FNC] = {".", "INDIRECT", 1, false, &def_entity, &def_field, &def_function},

    [OP_ADDRESS] = {".", "ADDRESS", 1, false, &def_entity, &def_field, &def_pointer},

    [OP_STORE_F] = {"=", "STORE_F", 5, true, &def_float, &def_float, &def_float},
    [OP_STORE_V] = {"=", "STORE_V", 5, true, &def_vector, &def_vector, &def_vector},
    [OP_STORE_S] = {"=", "STORE_S", 5, true, &def_string, &def_string, &def_string},
    [OP_STORE_ENT] = {"=", "STORE_ENT", 5, true, &def_entity, &def_entity, &def_entity},
    [OP_STORE_FLD] = {"=", "STORE_FLD", 5, true, &def_field, &def_field, &def_field},
    [OP_STORE_FNC] = {"=", "STORE_FNC", 5, true, &def_function, &def_function, &def_function},

    [OP_STOREP_F] = {"=", "STOREP_F", 5, true, &def_pointer, &def_float, &def_float},
    [OP_STOREP_V] = {"=", "STOREP_V", 5, true, &def_pointer, &def_vector, &def_vector},
    [OP_STOREP_S] = {"=", "STOREP_S", 5, true, &def_pointer, &def_string, &def_string},
    [OP_STOREP_ENT] = {"=", "STOREP_ENT", 5, true, &def_pointer, &def_entity, &def_entity},
    [OP_STOREP_FLD] = {"=", "STOREP_FLD", 5, true, &def_pointer, &def_field, &def_field},
    [OP_STOREP_FNC] = {"=", "STOREP_FNC", 5, true, &def_pointer, &def_function, &def_function},

    [OP_RETURN] = {"<RETURN>", "RETURN", -1, false, &def_void, &def_void, &def_void},

    [OP_NOT_F] = {"!", "NOT_F", -1, false, &def_float, &def_void, &def_float},
    [OP_NOT_V] = {"!", "NOT_V", -1, false, &def_vector, &def_void, &def_float},
    [OP_NOT_S] = {"!", "NOT_S", -1, false, &def_vector, &def_void, &def_float},
    [OP_NOT_ENT] = {"!", "NOT_ENT", -1, false, &def_entity, &def_void, &def_float},
    [OP_NOT_FNC] = {"!", "NOT_FNC", -1, false, &def_function, &def_void, &def_float},

    [OP_IF] = {"<IF>", "IF", -1, false, &def_float, &def_float, &def_void},
    [OP_IFNOT] = {"<IFNOT>", "IFNOT", -1, false, &def_float, &def_float, &def_void},

    // calls returns REG_RETURN
    [OP_CALL0] = {"<CALL0>", "CALL0", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL1] = {"<CALL1>", "CALL1", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL2] = {"<CALL2>", "CALL2", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL3] = {"<CALL3>", "CALL3", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL4] = {"<CALL4>", "CALL4", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL5] = {"<CALL5>", "CALL5", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL6] = {"<CALL6>", "CALL6", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL7] = {"<CALL7>", "CALL7", -1, false, &def_function, &def_void, &def_void},
    [OP_CALL8] = {"<CALL8>", "CALL8", -1, false, &def_function, &def_void, &def_void},

    [OP_STATE] = {"<STATE>", "STATE", -1, false, &def_float, &def_float, &def_void},

    [OP_GOTO] = {"<GOTO>", "GOTO", -1, false, &def_float, &def_void, &def_void},

    [OP_AND] = {"&&", "AND", 6, false, &def_float, &def_float, &def_float},
    [OP_OR] = {"||", "OR", 6, false, &def_float, &def_float, &def_float},

    [OP_BITAND] = {"&", "BITAND", 2, false, &def_float, &def_float, &def_float},
    [OP_BITOR] = {"|", "BITOR", 2, false, &def_float, &def_float, &def_float},

    [OP_LAST] = {NULL}
};


