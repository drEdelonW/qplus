#pragma once

typedef enum {
    ev_void = 0u,
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

extern const int type_size[ev_LAST];
