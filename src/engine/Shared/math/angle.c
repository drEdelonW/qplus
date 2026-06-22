#include "angle.h"

float anglemod(float a) {
#if 0
    if (a >= 0) a -= 360 * (int)(a / 360);
    else        a += 360 * (1 + (int)(-a / 360));
#endif
    a = (360.0f / (0xFFFF + 1)) * ((int)(a * ((0xFFFF + 1) / 360.0f)) & 0xFFFF);
    return a;
}
