#pragma once

// #if !defined BYTE_DEFINED	// needed by [vid_sunxil.c]
	#if 0
		// typedef unsigned char 		byte; // uint8_t
	#else
		#include <stdint.h>
		typedef uint8_t 		byte;
	#endif
	// #define BYTE_DEFINED 1
// #endif