#pragma once

#include "types.h"
#include "qColor.h"

typedef struct {
    int32_t width;
    int32_t height;
    qColor8_t data[4];    // variably sized
} qPic_t;
typedef qPic_t* qPic_p;

#ifdef __cplusplus
extern "C" {
#endif

    void SwapPic(qPic_p pic);
    qPic_p  GetPicFromWad(cStringRO name);
    qPic_p  Draw_CachePic(cStringRO path);

#ifdef __cplusplus
}
#endif