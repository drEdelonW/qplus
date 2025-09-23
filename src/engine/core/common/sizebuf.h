#pragma once

#include <stdint.h>
#include "qboolean.h"
#include "zone.h"


typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	uint8_t	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

void SZ_Alloc (sizebuf_t *buf, int startsize);
void SZ_Free (sizebuf_t *buf);
void SZ_Clear (sizebuf_t *buf);
typeless_ptr SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, typeless_ptr data, int length);
void SZ_Print (sizebuf_t *buf, cstring data);	// strcats onto the sizebuf