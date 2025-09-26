#include "sizebuf.h"

#include "types.h"
#include "q_tools.h"
#include "console.h"
#include "sys.h"
#include "zone.h"

//===========================================================================

void SZ_Alloc(sizebuf_p buf, int32_t startsize) {
    CLAMP_LESS(startsize, 256);
    buf->data = Hunk_AllocName(startsize, "sizebuf");
    buf->maxsize = startsize;
    buf->cursize = 0;
}


void SZ_Free(sizebuf_p buf) {
    //      Z_Free(buf->data);
    //      buf->data = NULL;
    //      buf->maxsize = 0;
    buf->cursize = 0;
}

void SZ_Clear(sizebuf_p buf) {
    buf->cursize = 0;
}

typeless_ptr SZ_GetSpace(sizebuf_p buf, int32_t length) {
    if ((buf->cursize + length) > buf->maxsize) {
        if (!buf->allowoverflow)
            Sys_Error("SZ_GetSpace: overflow without allowoverflow set");

        if (length > buf->maxsize)
            Sys_Error("SZ_GetSpace: %i is > full buffer size", length);

        buf->overflowed = true;
        Con_Printf("SZ_GetSpace: overflow");
        SZ_Clear(buf);
    }

    typeless_ptr data = buf->data + buf->cursize;
    buf->cursize += length;

    return data;
}

void SZ_Write(sizebuf_p buf, typeless_ptr data, int32_t length) {
    Q_memcpy(SZ_GetSpace(buf, length), data, length);
}

void SZ_Print(sizebuf_p buf, cstring data) {
    int len = Q_strlen(data) + 1;

    // uint8_p cast to keep VC++ happy
    if (buf->data[buf->cursize - 1])
        Q_memcpy((uint8_p)SZ_GetSpace(buf, len), data, len); // no trailing 0
    else
        Q_memcpy((uint8_p)SZ_GetSpace(buf, len - 1) - 1, data, len); // write over trailing 0
}
