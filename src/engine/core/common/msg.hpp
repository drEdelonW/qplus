#pragma once

#include "sizebuf.h"

//============================================================================
class NetMsg {
public:
    void WriteChar(sizebuf_p sb, int8_t c);
    void WriteByte(sizebuf_p sb, uint8_t c);
    void WriteShort(sizebuf_p sb, int16_t c);
    void WriteLong(sizebuf_p sb, int32_t c);
    void WriteFloat(sizebuf_p sb, float f);
    void WriteString(sizebuf_p sb, cString s);
    void WriteCoord(sizebuf_p sb, float f);
    void WriteAngle(sizebuf_p sb, float f);

    int  ReadCount();
    bool BadRead();

    void    BeginReading();
    int8_t  ReadChar();
    uint8_t ReadByte();
    int16_t ReadShort();
    int32_t ReadLong();
    float   ReadFloat();
    cString ReadString();

    float ReadCoord();
    // vect_t ReadCoord();
    float ReadAngle();

private:
    int     _readCount;
    bool    _badRead; // set if a read goes beyond end of message
};