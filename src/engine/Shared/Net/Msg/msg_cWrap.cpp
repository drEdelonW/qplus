#include "msg.h"
#include "msg.hpp"

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles uint8_t ordering and avoids alignment errors
==============================================================================
*/

#include "net.h"  // net_message

void MSG_SetSizeBuf(sizebuf_p sb) { nMSG.SetSizeBuf(sb); }
void MSG_BeginReading() { nMSG.SetSizeBuf(&net_message); nMSG.BeginReading(); }

int  getMsgReadCount() { return nMSG.ReadCount(); }
bool getMsgBadRead() { return nMSG.BadRead(); }

void MSG_WriteChar(sizebuf_p sb, int8_t c) { ;      nMSG.SetSizeBuf(sb); nMSG.WriteChar(c); }
void MSG_WriteByte(sizebuf_p sb, uint8_t c) { ;     nMSG.SetSizeBuf(sb); nMSG.WriteByte(c); }
void MSG_WriteShort(sizebuf_p sb, int16_t c) { ;    nMSG.SetSizeBuf(sb); nMSG.WriteShort(c); }
void MSG_WriteLong(sizebuf_p sb, int32_t c) { ;     nMSG.SetSizeBuf(sb); nMSG.WriteLong(c); }
void MSG_WriteFloat(sizebuf_p sb, float f) { ;      nMSG.SetSizeBuf(sb); nMSG.WriteFloat(f); }
void MSG_WriteString(sizebuf_p sb, cStringRO s) { ; nMSG.SetSizeBuf(sb); nMSG.WriteString(s); }
void MSG_WriteCoord(sizebuf_p sb, float f) { ;      nMSG.SetSizeBuf(sb); nMSG.WriteCoord(f); }
void MSG_WriteAngle(sizebuf_p sb, float f) { ;      nMSG.SetSizeBuf(sb); nMSG.WriteAngle(f); }

#if 0
void MSG_WriteCharB(int8_t c) { ;   nMSG.WriteChar(c); }
void MSG_WriteByteB(uint8_t c) { ;  nMSG.WriteByte(c); }
void MSG_WriteShortB(int16_t c) { ; nMSG.WriteShort(c); }
void MSG_WriteLongB(int32_t c) { ;  nMSG.WriteLong(c); }
void MSG_WriteFloatB(float f) { ;   nMSG.WriteFloat(f); }
void MSG_WriteStringB(cStringRO s) { ;nMSG.WriteString(s); }
void MSG_WriteCoordB(float f) { ;   nMSG.WriteCoord(f); }
void MSG_WriteAngleB(float f) { ;   nMSG.WriteAngle(f); }
#endif

// returns MSG_ERROR and sets msg_badread if no more characters are available
int8_t MSG_ReadChar() { ;       nMSG.SetSizeBuf(&net_message); return nMSG.ReadChar(); }
uint8_t MSG_ReadByte() { ;      nMSG.SetSizeBuf(&net_message); return nMSG.ReadByte(); }
int16_t MSG_ReadShort() { ;     nMSG.SetSizeBuf(&net_message); return nMSG.ReadShort(); }
int32_t MSG_ReadLong() { ;      nMSG.SetSizeBuf(&net_message); return nMSG.ReadLong(); }
float MSG_ReadFloat() { ;       nMSG.SetSizeBuf(&net_message); return nMSG.ReadFloat(); }
cString MSG_ReadString() { ;    nMSG.SetSizeBuf(&net_message); return nMSG.ReadString(); }
float MSG_ReadCoord() { ;       nMSG.SetSizeBuf(&net_message); return nMSG.ReadCoord(); }
float MSG_ReadAngle() { ;       nMSG.SetSizeBuf(&net_message); return nMSG.ReadAngle(); }


