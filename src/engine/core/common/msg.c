#include "msg.h"
#include "q_tools.h"
#include "endian_tools.h"
// #include "net.h"
extern	sizebuf_t	net_message;
/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles uint8_t ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t* sb, int c){
#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error ("MSG_WriteChar: range error");
#endif

	uint8_t* buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t* sb, int c){
#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error ("MSG_WriteByte: range error");
#endif

	uint8_t* buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t* sb, int c){
#ifdef PARANOID
	if (c < ((int16_t)0x8000) || c > (int16_t)0x7fff)
		Sys_Error ("MSG_WriteShort: range error");
#endif

	uint8_t* buf = SZ_GetSpace(sb, 2);
	buf[0] = c & 0xff;
	buf[1] = c >> 8;
}

void MSG_WriteLong (sizebuf_t* sb, int c){
	uint8_t* buf = SZ_GetSpace(sb, 4);
	buf[0] =  c & 0xff;
	buf[1] = (c >> 8) & 0xff;
	buf[2] = (c >> 16) & 0xff;
	buf[3] =  c >> 24;
}

void MSG_WriteFloat(sizebuf_t* sb, float f){
    union{
        float   f;
        int     l;
    } dat;

	dat.f = f;
	dat.l = LittleLong(dat.l);

	SZ_Write(sb, &dat.l, 4);
}

void MSG_WriteString(sizebuf_t* sb, cstring s){
	if (!s)
		SZ_Write(sb, "", 1);
	else
		SZ_Write(sb, s, (Q_strlen(s) + 1));
}

void MSG_WriteCoord(sizebuf_t* sb, float f){
	MSG_WriteShort(sb, (int)(f*8));
}

void MSG_WriteAngle(sizebuf_t* sb, float f){
	MSG_WriteByte(sb, ((int)f*256/360) & 255);
}

//
// reading functions
//
int         msg_readcount;
qboolean    msg_badread;

void MSG_BeginReading(){
	msg_readcount = 0;
	msg_badread = false;
}

// returns MSG_ERROR and sets msg_badread if no more characters are available
int MSG_ReadChar(){
	if ((msg_readcount + 1) > net_message.cursize)	{
		msg_badread = true;
		return MSG_ERROR;
	}

	int c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadByte(){
	if ((msg_readcount + 1) > net_message.cursize){
		msg_badread = true;
		return MSG_ERROR;
	}

	int c = (uint8_t)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadShort(){
	if ((msg_readcount + 2) > net_message.cursize)	{
		msg_badread = true;
		return MSG_ERROR;
	}

	int c = (int16_t)(
         net_message.data[msg_readcount] +
        (net_message.data[msg_readcount + 1] << 8)
    );

	msg_readcount += 2;

	return c;
}

int MSG_ReadLong(){
	if ((msg_readcount + 4) > net_message.cursize)	{
		msg_badread = true;
		return MSG_ERROR;
	}

	int c =
         net_message.data[msg_readcount] +
        (net_message.data[msg_readcount + 1] << 8) +
        (net_message.data[msg_readcount + 2] << 16)	+
        (net_message.data[msg_readcount + 3] << 24);

	msg_readcount += 4;

	return c;
}

float MSG_ReadFloat(){
	union{
		uint8_t b[4];
		float   f;
		int     l;
	} dat;

	dat.b[0] = net_message.data[msg_readcount];
	dat.b[1] = net_message.data[msg_readcount + 1];
	dat.b[2] = net_message.data[msg_readcount + 2];
	dat.b[3] = net_message.data[msg_readcount + 3];
	msg_readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;
}

cstring MSG_ReadString(){
	static char string[2048];

	int l = 0;
	do{
		int c = MSG_ReadChar();
		if ((c == MSG_ERROR) || (c == '\0'))
			break;
		string[l] = c;
		l++;
	} while (l < (sizeof(string) - 1));

	string[l] = 0;

	return string;
}

float MSG_ReadCoord(){
	return MSG_ReadShort() * (1.0/8);
}

float MSG_ReadAngle(){
	return MSG_ReadChar() * (360.0/256);
}


