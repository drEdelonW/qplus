#include "msg.hpp"
#include "q_tools.h"
#include "endian_tools.h"

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles uint8_t ordering and avoids alignment errors
==============================================================================
*/

int  NetMsg::ReadCount() { return _readCount; }
bool NetMsg::BadRead() { return _badRead; }

void NetMsg::BeginReading() {
    _readCount = 0;
    _badRead = false;
}

void NetMsg::SetSizeBuf(sizebuf_p sb) {
    if (sb != NULL) {
        _sb = sb;
    }
}



// returns MSG_ERROR and sets _badRead if no more characters are available
int8_t NetMsg::ReadChar() {
    if ((_readCount + 1) > _sb->cursize) {
        _badRead = true;
        return 0;
    }

    int8_t c = (int8_t)_sb->data[_readCount];
    _readCount++;

    return c;
}

void NetMsg::WriteChar(sizebuf_p sb, int8_t c) {
    if (!sb)    return;
#ifdef PARANOID
    if ((c < -128) || (c > 127))
        Sys_Error("NetMsg::WriteChar: range error");
#endif

    uint8_p buf = (uint8_p)SZ_GetSpace(sb, 1);
    buf[0] = c;
}
void NetMsg::WriteChar(int8_t c) { WriteChar(_sb, c); }



uint8_t NetMsg::ReadByte() {
    if ((_readCount + 1) > _sb->cursize) {
        _badRead = true;
        return 0;
    }

    uint8_t c = (uint8_t)_sb->data[_readCount];
    _readCount++;

    return c;
}

void NetMsg::WriteByte(sizebuf_p sb, uint8_t c) {
    if (!sb)    return;
#ifdef PARANOID
    if ((c < 0x00) || (c > 0XFF))
        Sys_Error("NetMsg::WriteByte: range error");
#endif

    uint8_p buf = (uint8_p)SZ_GetSpace(sb, 1);
    buf[0] = c;
}
void NetMsg::WriteByte(uint8_t c) { WriteByte(_sb, c); }


int16_t NetMsg::ReadShort() {
    if ((_readCount + 2) > _sb->cursize) {
        _badRead = true;
        return 0;
    }

    int c = (int16_t)(
        _sb->data[_readCount] +
        (_sb->data[_readCount + 1] << 8)
        );

    _readCount += 2;

    return c;
}

void NetMsg::WriteShort(sizebuf_p sb, int16_t c) {
    if (!sb)    return;
#ifdef PARANOID
    if ((c < ((int16_t)0x8000)) || (c > (int16_t)0x7fff))
        Sys_Error("NetMsg::WriteShort: range error");
#endif

    uint8_p buf = (uint8_p)SZ_GetSpace(sb, 2);
    buf[0] = c & 0XFF;
    buf[1] = c >> 8;
}
void NetMsg::WriteShort(int16_t c) { WriteShort(_sb, c); }


int32_t NetMsg::ReadLong() {
    if ((_readCount + 4) > _sb->cursize) {
        _badRead = true;
        return 0;
    }

    int32_t c =
        (_sb->data[_readCount]) +
        (_sb->data[_readCount + 1] << 8) +
        (_sb->data[_readCount + 2] << 16) +
        (_sb->data[_readCount + 3] << 24);

    _readCount += 4;

    return c;
}

void NetMsg::WriteLong(sizebuf_p sb, int32_t c) {
    if (!sb)    return;
    uint8_p buf = (uint8_p)SZ_GetSpace(sb, 4);
    buf[0] = (c & 0XFF);
    buf[1] = (c >> 8) & 0XFF;
    buf[2] = (c >> 16) & 0XFF;
    buf[3] = (c >> 24);
}
void NetMsg::WriteLong(int32_t c) { WriteLong(_sb, c); }


float NetMsg::ReadFloat() {
    union {
        uint8_t b[4];
        float   f;
        int     l;
    } dat;

    dat.b[0] = _sb->data[_readCount];
    dat.b[1] = _sb->data[_readCount + 1];
    dat.b[2] = _sb->data[_readCount + 2];
    dat.b[3] = _sb->data[_readCount + 3];
    _readCount += 4;

    dat.l = LittleLong(dat.l);

    return dat.f;
}

void NetMsg::WriteFloat(sizebuf_p sb, float f) {
    if (!sb)    return;
    union {
        float   f;
        int     l;
    } dat;

    dat.f = f;
    dat.l = LittleLong(dat.l);

    SZ_Write(sb, &dat.l, 4);
}
void NetMsg::WriteFloat(float f) { WriteFloat(_sb, f); }


cString NetMsg::ReadString() {
    static char string[2048];

    int l = 0;
    do {
        int c = NetMsg::ReadChar();
        if ((_badRead) || (c == '\0'))
            break;
        string[l] = c;
        l++;
    } while (l < (sizeof(string) - 1));

    string[l] = 0;

    return string;
}

void NetMsg::WriteString(sizebuf_p sb, cString s) {
    if (!sb)    return;
    char empSt[2] = { 0 };    // ""
    if (!s) SZ_Write(sb, empSt, 1);
    else    SZ_Write(sb, s, (Q_strlen(s) + 1));
}
void NetMsg::WriteString(cString s) { WriteString(_sb, s); }



float NetMsg::ReadCoord() { return ReadShort() * (1.0 / 8); }
void NetMsg::WriteCoord(sizebuf_p sb, float f) { WriteShort(sb, (int)(f * 8)); }
void NetMsg::WriteCoord(float f) { WriteCoord(_sb, f); }

float NetMsg::ReadAngle() { return ReadChar() * (360.0 / 0xFF); }
void NetMsg::WriteAngle(sizebuf_p sb, float f) { WriteByte(sb, ((int)f * 0xFF / 360) & 0xFF); }
void NetMsg::WriteAngle(float f) { WriteAngle(_sb, f); }

