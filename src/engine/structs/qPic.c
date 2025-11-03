#include "qPic.h"
#include "endian_tools.h"

void SwapPic(qPic_p pic) {
    pic->width = LittleLong(pic->width);
    pic->height = LittleLong(pic->height);
}
