#error deprecated!

#include "msg.h"
#include "q_tools.h"
#include "endian_tools.h"
#include "net.h"

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles uint8_t ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar(sizebuf_p sb, int8_t c) {
#ifdef PARANOID
    if ((c < -128) || (c > 127))
        Sys_Error("MSG_WriteChar: range error");
#endif

    uint8_p buf = SZ_GetSpace(sb, 1);
    buf[0] = c;
}

void MSG_WriteByte(sizebuf_p sb, uint8_t c) {
#ifdef PARANOID
    if ((c < 0) || (c > 255))
        Sys_Error("MSG_WriteByte: range error");
#endif

    uint8_p buf = SZ_GetSpace(sb, 1);
    buf[0] = c;
}

void MSG_WriteShort(sizebuf_p sb, int16_t c) {
#ifdef PARANOID
    if ((c < ((int16_t)0x8000)) || (c > (int16_t)0x7fff))
        Sys_Error("MSG_WriteShort: range error");
#endif

    uint8_p buf = SZ_GetSpace(sb, 2);
    buf[0] = c & 0xff;
    buf[1] = c >> 8;
}

void MSG_WriteLong(sizebuf_p sb, int32_t c) {
    uint8_p buf = SZ_GetSpace(sb, 4);
    buf[0] = c & 0xff;
    buf[1] = (c >> 8) & 0xff;
    buf[2] = (c >> 16) & 0xff;
    buf[3] = c >> 24;
}

void MSG_WriteFloat(sizebuf_p sb, float f) {
    union {
        float   f;
        int     l;
    } dat;

    dat.f = f;
    dat.l = LittleLong(dat.l);

    SZ_Write(sb, &dat.l, 4);
}

void MSG_WriteString(sizebuf_p sb, cStringRO s) {
    if (!s)
        SZ_Write(sb, "", 1);
    else
        SZ_Write(sb, s, (Q_strlen(s) + 1));
}

void MSG_WriteCoord(sizebuf_p sb, float f) {
    MSG_WriteShort(sb, (int)(f * 8));
}

void MSG_WriteAngle(sizebuf_p sb, float f) {
    MSG_WriteByte(sb, ((int)f * 256 / 360) & 255);
}

//
// reading functions
//
static int     _msgReadCount;
static bool    _msgBadRead;

void MSG_BeginReading() {
    _msgReadCount = 0;
    _msgBadRead = false;
}

// returns MSG_ERROR and sets _msgBadRead if no more characters are available
int8_t MSG_ReadChar() {
    if ((_msgReadCount + 1) > net_message.cursize) {
        _msgBadRead = true;
        return 0;
    }

    int8_t c = (int8_t)net_message.data[_msgReadCount];
    _msgReadCount++;

    return c;
}

uint8_t MSG_ReadByte() {
    if ((_msgReadCount + 1) > net_message.cursize) {
        _msgBadRead = true;
        return 0;
    }

    uint8_t c = (uint8_t)net_message.data[_msgReadCount];
    _msgReadCount++;

    return c;
}

int16_t MSG_ReadShort() {
    if ((_msgReadCount + 2) > net_message.cursize) {
        _msgBadRead = true;
        return 0;
    }

    int c = (int16_t)(
        net_message.data[_msgReadCount] +
        (net_message.data[_msgReadCount + 1] << 8)
        );

    _msgReadCount += 2;

    return c;
}

int32_t MSG_ReadLong() {
    if ((_msgReadCount + 4) > net_message.cursize) {
        _msgBadRead = true;
        return 0;
    }

    int32_t c =
        net_message.data[_msgReadCount] +
        (net_message.data[_msgReadCount + 1] << 8) +
        (net_message.data[_msgReadCount + 2] << 16) +
        (net_message.data[_msgReadCount + 3] << 24);

    _msgReadCount += 4;

    return c;
}

float MSG_ReadFloat() {
    union {
        uint8_t b[4];
        float   f;
        int     l;
    } dat;

    dat.b[0] = net_message.data[_msgReadCount];
    dat.b[1] = net_message.data[_msgReadCount + 1];
    dat.b[2] = net_message.data[_msgReadCount + 2];
    dat.b[3] = net_message.data[_msgReadCount + 3];
    _msgReadCount += 4;

    dat.l = LittleLong(dat.l);

    return dat.f;
}

cString MSG_ReadString() {
    static char string[2048];

    int l = 0;
    do {
        int c = MSG_ReadChar();
        if ((_msgBadRead) || (c == '\0'))
            break;
        string[l] = c;
        l++;
    } while (l < (sizeof(string) - 1));

    string[l] = 0;

    return string;
}

float MSG_ReadCoord() {
    return MSG_ReadShort() * (1.0 / 8);
}

float MSG_ReadAngle() {
    return MSG_ReadChar() * (360.0 / 256);
}


