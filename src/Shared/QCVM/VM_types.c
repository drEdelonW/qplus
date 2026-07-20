#include "VM_types.h"

def_p def_for_type[ev_LAST] = {
    [ev_void]    /**/ = &def_void,
    [ev_string]  /**/ = &def_string,
    [ev_float]   /**/ = &def_float,
    [ev_vector]  /**/ = &def_vector,
    [ev_entity]  /**/ = &def_entity,
    [ev_field]   /**/ = &def_field,
    [ev_function]/**/ = &def_function,
    [ev_pointer] /**/ = &def_pointer
};

cStringRO typeCName[ev_LAST] = {
#if 0
    [ev_void]    /**/ = "void     ",
    [ev_string]  /**/ = "string_t ",
    [ev_float]   /**/ = "float    ",
    [ev_vector]  /**/ = "vec3_t   ",
    [ev_entity]  /**/ = "EdIdx    ",
    [ev_field]   /**/ = "",
    [ev_function]/**/ = "func_t   ",
    [ev_pointer] /**/ = "void*    "
#else
    [ev_void]    /**/ = "void     ",
    [ev_string]  /**/ = "string_t ",
    [ev_float]   /**/ = "float    ",
    [ev_vector]  /**/ = "vec3_t   ",
    [ev_entity]  /**/ = "int      ",
    [ev_field]   /**/ = "skipped  ",
    [ev_function]/**/ = "func_t   ",
    [ev_pointer] /**/ = "int      "
#endif
};
