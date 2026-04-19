#pragma once

#include "types.h"

typedef enum {
    ev_void     = 0u,
    ev_string,
    ev_float,
    ev_vector,
    ev_entity,
    ev_field,
    ev_function,
    ev_pointer,
    ev_LAST,

    DEF_SAVEGLOBAL = (1U << 15)
} etype_t;  // :uint16_t

typedef int16_t arg_type;   // should be signed int

cString PR_ValueString(etype_t type, eval_p val);
cString PR_UglyValueString(etype_t type, eval_p val);