#pragma once

#include "sizebuf.h"

//============================================================================
class NetMsg {
public:
    NetMsg(sizebuf_p sb = NULL) : _sb(sb), _readCount(0), _badRead(false) {};

    void BeginReading();
    int  ReadCount();
    bool BadRead();
    void SetSizeBuf(sizebuf_p sb);

    int8_t  ReadChar();
    void    WriteChar(sizebuf_p sb, int8_t c);
    void    WriteChar(int8_t c);

    uint8_t ReadByte();
    void    WriteByte(sizebuf_p sb, uint8_t c);
    void    WriteByte(uint8_t c);

    int16_t ReadShort();
    void    WriteShort(sizebuf_p sb, int16_t c);
    void    WriteShort(int16_t c);

    int32_t ReadLong();
    void    WriteLong(sizebuf_p sb, int32_t c);
    void    WriteLong(int32_t c);

    float   ReadFloat();
    void    WriteFloat(sizebuf_p sb, float f);
    void    WriteFloat(float f);

    cString ReadString();
    void    WriteString(sizebuf_p sb, cStringRO s);
    void    WriteString(cStringRO s);

    float   ReadCoord();
    void    WriteCoord(sizebuf_p sb, float f);
    void    WriteCoord(float f);

    float   ReadAngle();
    void    WriteAngle(sizebuf_p sb, float f);
    void    WriteAngle(float f);


    // vect_t ReadCoord();

private:
    sizebuf_p   _sb;
    int         _readCount;
    bool        _badRead; // set if a read goes beyond end of message
};

extern NetMsg nMSG;
