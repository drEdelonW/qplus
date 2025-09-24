#pragma once

#include "types.h"
//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct {
	cstring basedir;
	cstring cachedir;		// for development over ISDN lines
	int		argc;
	char** argv;
	typeless_ptr membase;
	int		memsize;
} quakeparms_t;
typedef quakeparms_t* quakeparms_p;
