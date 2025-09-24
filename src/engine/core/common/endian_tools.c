#include "endian_tools.h"
#include <stdint.h>
/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

qboolean        bigendien;

int16_t(*BigShort) (int16_t l);
int16_t(*LittleShort) (int16_t l);
int     (*BigLong) (int l);
int     (*LittleLong) (int l);
float   (*BigFloat) (float l);
float   (*LittleFloat) (float l);

int16_t   ShortSwap(int16_t l) {
	uint8_t    b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return (b1 << 8) + b2;
}

int16_t   ShortNoSwap(int16_t l) {
	return l;
}

int    LongSwap(int l) {
	uint8_t    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

int     LongNoSwap(int l) {
	return l;
}

float FloatSwap(float f) {
	union
	{
		float   f;
		uint8_t    b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap(float f) {
	return f;
}

void COM_Endian_Init() {
	uint8_t    swaptest[2] = { 1,0 };

	// set the uint8_t swapping variables in a portable manner
	if (*(int16_t*)swaptest == 1) {
		bigendien = false;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else {
		bigendien = true;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}
}


