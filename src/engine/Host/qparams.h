#pragma once

#include "types.h"
//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct {
    cString         baseDir;
    cString         cacheDir;  // for development over ISDN lines
    int             argc;
    cStringArray    argv;
    TypeLess_ptr    membase;
    size_t          memsize;
} QuakeParms_t;
typedef QuakeParms_t* QuakeParms_p;
