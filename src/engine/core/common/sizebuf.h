#pragma once
#include "types.h"

typedef struct sizebuf_s {
    bool    allowoverflow;  // if false, do a Sys_Error
    bool    overflowed;     // set to true if the buffer size failed
    uint8_p data;
    int32_t maxsize;
    int32_t cursize;
} sizebuf_t;
typedef sizebuf_t* sizebuf_p;

#ifdef __cplusplus
extern "C" {
#endif
void SZ_Alloc(sizebuf_p buf, int32_t startsize);
void SZ_Free(sizebuf_p buf);
void SZ_Clear(sizebuf_p buf);
TypeLess_ptr SZ_GetSpace(sizebuf_p buf, int32_t length);
void SZ_Write(sizebuf_p buf, TypeLess_ptr data, int32_t length);
void SZ_Print(sizebuf_p buf, cString data);     // strcats onto the sizebuf

#ifdef __cplusplus
}
#endif