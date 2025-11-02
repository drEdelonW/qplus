#pragma once

#include "sizebuf.h"
#ifdef __cplusplus
extern "C" {
#endif
//============================================================================
// #define MSG_ERROR   (-1)
void MSG_WriteChar(sizebuf_p sb, int8_t c);
void MSG_WriteByte(sizebuf_p sb, uint8_t c);
void MSG_WriteShort(sizebuf_p sb, int16_t c);
void MSG_WriteLong(sizebuf_p sb, int32_t c);
void MSG_WriteFloat(sizebuf_p sb, float f);
void MSG_WriteString(sizebuf_p sb, cStringRO s);
void MSG_WriteCoord(sizebuf_p sb, float f);
void MSG_WriteAngle(sizebuf_p sb, float f);

// extern int  msg_readcount;
int getMsgReadCount();
// extern bool msg_badread;  // set if a read goes beyond end of message
bool getMsgBadRead();

void    MSG_BeginReading();
int8_t  MSG_ReadChar();
uint8_t MSG_ReadByte();
int16_t MSG_ReadShort();
int32_t MSG_ReadLong();
float   MSG_ReadFloat();
cString MSG_ReadString();

float MSG_ReadCoord();
// vect_t MSG_ReadCoord();
float MSG_ReadAngle();

#ifdef __cplusplus
}
#endif