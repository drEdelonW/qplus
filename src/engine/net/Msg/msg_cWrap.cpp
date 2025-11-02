#include "msg.h"
#include "msg.hpp"

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles uint8_t ordering and avoids alignment errors
==============================================================================
*/

#include "net.h"  // net_message


void MSG_SetSizeBuf(sizebuf_p sb) { MSG.SetSizeBuf(sb); }
void MSG_BeginReading() { MSG.SetSizeBuf(&net_message); MSG.BeginReading(); }
int getMsgReadCount() { return MSG.ReadCount(); }
bool getMsgBadRead() { return MSG.BadRead(); }

void MSG_WriteChar(sizebuf_p sb, int8_t c) { ;      MSG.SetSizeBuf(sb); MSG.WriteChar(c); }
void MSG_WriteByte(sizebuf_p sb, uint8_t c) { ;     MSG.SetSizeBuf(sb); MSG.WriteByte(c); }
void MSG_WriteShort(sizebuf_p sb, int16_t c) { ;    MSG.SetSizeBuf(sb); MSG.WriteShort(c); }
void MSG_WriteLong(sizebuf_p sb, int32_t c) { ;     MSG.SetSizeBuf(sb); MSG.WriteLong(c); }
void MSG_WriteFloat(sizebuf_p sb, float f) { ;      MSG.SetSizeBuf(sb); MSG.WriteFloat(f); }
void MSG_WriteString(sizebuf_p sb, cStringRO s) { ; MSG.SetSizeBuf(sb); MSG.WriteString(s); }
void MSG_WriteCoord(sizebuf_p sb, float f) { ;      MSG.SetSizeBuf(sb); MSG.WriteCoord(f); }
void MSG_WriteAngle(sizebuf_p sb, float f) { ;      MSG.SetSizeBuf(sb); MSG.WriteAngle(f); }

#if 0
void MSG_WriteCharB(int8_t c) { ;   MSG.WriteChar(c); }
void MSG_WriteByteB(uint8_t c) { ;  MSG.WriteByte(c); }
void MSG_WriteShortB(int16_t c) { ; MSG.WriteShort(c); }
void MSG_WriteLongB(int32_t c) { ;  MSG.WriteLong(c); }
void MSG_WriteFloatB(float f) { ;   MSG.WriteFloat(f); }
void MSG_WriteStringB(cStringRO s) { ;MSG.WriteString(s); }
void MSG_WriteCoordB(float f) { ;   MSG.WriteCoord(f); }
void MSG_WriteAngleB(float f) { ;   MSG.WriteAngle(f); }
#endif

// returns MSG_ERROR and sets msg_badread if no more characters are available
int8_t MSG_ReadChar() { ;       MSG.SetSizeBuf(&net_message); return MSG.ReadChar(); }
uint8_t MSG_ReadByte() { ;      MSG.SetSizeBuf(&net_message); return MSG.ReadByte(); }
int16_t MSG_ReadShort() { ;     MSG.SetSizeBuf(&net_message); return MSG.ReadShort(); }
int32_t MSG_ReadLong() { ;      MSG.SetSizeBuf(&net_message); return MSG.ReadLong(); }
float MSG_ReadFloat() { ;       MSG.SetSizeBuf(&net_message); return MSG.ReadFloat(); }
cString MSG_ReadString() { ;    MSG.SetSizeBuf(&net_message); return MSG.ReadString(); }
float MSG_ReadCoord() { ;       MSG.SetSizeBuf(&net_message); return MSG.ReadCoord(); }
float MSG_ReadAngle() { ;       MSG.SetSizeBuf(&net_message); return MSG.ReadAngle(); }


