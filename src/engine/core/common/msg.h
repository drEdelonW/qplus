#pragma once

#include "sizebuf.h"


//============================================================================
#define MSG_ERROR   (-1)
void MSG_WriteChar(sizebuf_p sb, int c);
void MSG_WriteByte(sizebuf_p sb, int c);
void MSG_WriteShort(sizebuf_p sb, int c);
void MSG_WriteLong(sizebuf_p sb, int c);
void MSG_WriteFloat(sizebuf_p sb, float f);
void MSG_WriteString(sizebuf_p sb, cstring s);
void MSG_WriteCoord(sizebuf_p sb, float f);
void MSG_WriteAngle(sizebuf_p sb, float f);

extern	int			msg_readcount;
extern	qboolean	msg_badread;		// set if a read goes beyond end of message

void  MSG_BeginReading();
int   MSG_ReadChar();
int   MSG_ReadByte();
int   MSG_ReadShort();
int   MSG_ReadLong();
float MSG_ReadFloat();
cstring MSG_ReadString();

float MSG_ReadCoord();
float MSG_ReadAngle();
