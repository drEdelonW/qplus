#include "pr_Argument.h"


int type_size[ev_LAST] = {
    1,                          // ev_void,
    sizeof(string_t) / 4,       // ev_string,
    1,                          // ev_float,
    3,                          // ev_vector,
    1,                          // ev_entity,
    1,                          // ev_field,
    sizeof(func_t) / 4,         // ev_function,
    sizeof(TypeLess_ptr) / 4    // ev_pointer
};