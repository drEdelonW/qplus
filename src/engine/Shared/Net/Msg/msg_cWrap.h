#pragma once
#error
#include "msg.h"
#ifdef __cplusplus
extern "C" {
#endif
    //============================================================================
    // #define MSG_ERROR   (-1)
    void MSG_SetSizeBuf(sizebuf_p sb);
    void MSG_WriteCharB(int8_t c);
    void MSG_WriteByteB(uint8_t c);
    void MSG_WriteShortB(int16_t c);
    void MSG_WriteLongB(int32_t c);
    void MSG_WriteFloatB(float f);
    void MSG_WriteStringB(cStringRO s);
    void MSG_WriteCoordB(float f);
    void MSG_WriteAngleB(float f);

#ifdef __cplusplus
}
#endif