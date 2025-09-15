#pragma once

#include <stdint.h>

#if !defined BYTE_DEFINED
	// typedef unsigned char 		byte; // uint8_t
	typedef uint8_t 		byte;
	#define BYTE_DEFINED 1
#endif