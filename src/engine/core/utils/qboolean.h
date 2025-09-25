#pragma once


#if 0
    #undef true
    #undef false

    typedef enum {
        false = 0,
        true = 1
    } qboolean;
#else
    #include <stdbool.h>

    typedef bool qboolean;
#endif