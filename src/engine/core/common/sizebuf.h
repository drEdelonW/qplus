#pragma once

#include "types.h"


typedef struct sizebuf_s {
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	uint8_t* data;
	int		maxsize;
	int		cursize;
} sizebuf_t;
typedef sizebuf_t* sizebuf_p;

void SZ_Alloc(sizebuf_p buf, int startsize);
void SZ_Free(sizebuf_p buf);
void SZ_Clear(sizebuf_p buf);
typeless_ptr SZ_GetSpace(sizebuf_p buf, int length);
void SZ_Write(sizebuf_p buf, typeless_ptr data, int length);
void SZ_Print(sizebuf_p buf, cstring data);	// strcats onto the sizebuf