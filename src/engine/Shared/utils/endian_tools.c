#include "endian_tools.h"
#include "types.h"
/*
============================================================================

                    BYTE ORDER FUNCTIONS

============================================================================
*/

bool    bigendien;  // not used yet
int16_t (*BigShort)     (int16_t l);
int16_t (*LittleShort)  (int16_t l);
int32_t (*BigLong)      (int32_t l);
int32_t (*LittleLong)   (int32_t l);
float   (*BigFloat)     (float l);
float   (*LittleFloat)  (float l);

int16_t ShortSwap(int16_t l) {
    uint8_t b1 =  l       & 0xFF;
    uint8_t b2 = (l >> 8) & 0xFF;

    return
        (b1 << 8) +
         b2;
}

int16_t ShortNoSwap(int16_t l) {
    return l;
}

int32_t LongSwap(int32_t l) {
    uint8_t b1 =  l        & 0xFF;
    uint8_t b2 = (l >>  8) & 0xFF;
    uint8_t b3 = (l >> 16) & 0xFF;
    uint8_t b4 = (l >> 24) & 0xFF;

    return
        ((int32_t)b1 << 24) +
        ((int32_t)b2 << 16) +
        ((int32_t)b3 <<  8) +
                  b4;
}

int32_t LongNoSwap(int32_t l) {
    return l;
}

float FloatSwap(float f) {
    union {
        float   f;
        uint8_t b[4];
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

void Endian_Init() {
    static uint8_t _swapTest[2] = { 1, 0 };

    // set the uint8_t swapping variables in a portable manner
    if (*(int16_p)_swapTest == 1) {
        bigendien   = false;
        BigShort    = ShortSwap;
        BigLong     = LongSwap;
        BigFloat    = FloatSwap;
        LittleShort = ShortNoSwap;
        LittleLong  = LongNoSwap;
        LittleFloat = FloatNoSwap;
    }
    else {
        bigendien   = true;
        BigShort    = ShortNoSwap;
        BigLong     = LongNoSwap;
        BigFloat    = FloatNoSwap;
        LittleShort = ShortSwap;
        LittleLong  = LongSwap;
        LittleFloat = FloatSwap;
    }
}


