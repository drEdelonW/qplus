#include "msg.h"
#include "msg.hpp"

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles uint8_t ordering and avoids alignment errors
==============================================================================
*/

NetMsg MSG;

//
// writing functions
//


void MSG_WriteChar(sizebuf_p sb, int8_t c) {
    MSG.WriteChar(sb, c);
}

void MSG_WriteByte(sizebuf_p sb, uint8_t c) {
    MSG.WriteByte(sb, c);
}

void MSG_WriteShort(sizebuf_p sb, int16_t c) {
    MSG.WriteShort(sb, c);
}

void MSG_WriteLong(sizebuf_p sb, int32_t c) {
    MSG.WriteLong(sb, c);
}

void MSG_WriteFloat(sizebuf_p sb, float f) {
    MSG.WriteFloat(sb, f);
}

void MSG_WriteString(sizebuf_p sb, cString s) {
    MSG.WriteString(sb, s);
}

void MSG_WriteCoord(sizebuf_p sb, float f) {
    MSG.WriteCoord(sb, f);
}

void MSG_WriteAngle(sizebuf_p sb, float f) {
    MSG.WriteAngle(sb, f);
}

//
// reading functions
//
// int     msg_readcount;
int getMsgReadCount() { return MSG.ReadCount(); }

// bool    msg_badread;
bool getMsgBadRead() { return MSG.BadRead(); }

void MSG_BeginReading() {
    MSG.BeginReading();
}

// returns MSG_ERROR and sets msg_badread if no more characters are available
int8_t MSG_ReadChar() {
    return MSG.ReadChar();
}

uint8_t MSG_ReadByte() {
    return MSG.ReadByte();
}

int16_t MSG_ReadShort() {
    return MSG.ReadShort();
}

int32_t MSG_ReadLong() {
    return MSG.ReadLong();
}

float MSG_ReadFloat() {
    return MSG.ReadFloat();
}

cString MSG_ReadString() {
    return MSG.ReadString();
}

float MSG_ReadCoord() {
    return MSG.ReadCoord();
}

float MSG_ReadAngle() {
    return MSG.ReadAngle();
}


