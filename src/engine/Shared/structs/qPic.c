#include "qPic.h"
#include "endian_tools.h"

void SwapPic(qPic_p pic) {
    pic->width = LittleLong(pic->width);
    pic->height = LittleLong(pic->height);
}


#include "z_cache.h"
#include "enginedefs.h" // qPathStr_t
#include <string.h>
#include "host.h"

typedef struct CachePic_s {
    qPathStr_t  name;
    CacheUser_t cache;
} CachePic_t;
typedef CachePic_t* CachePic_p;

#ifndef GLQUAKE
#include "wad.h"
qPic_p GetPicFromWad(cStringRO name) {
    return W_GetLumpName(name);
}

/*
================
Draw_CachePic
================
*/
#define MAX_CACHED_PICS  128
static CachePic_t   _cachePics[MAX_CACHED_PICS];
static int          _numCachePics = 0;

qPic_p Draw_CachePic(cStringRO path) {
    int i = 0;
    for (; i < _numCachePics; i++)
        if (!strcmp(path, _cachePics[i].name))
            break;

    if (i == _numCachePics) {
        if (_numCachePics == MAX_CACHED_PICS)
            Host_SysError("_numCachePics == MAX_CACHED_PICS");

        _numCachePics++;
        strcpy(_cachePics[i].name, path);
    }

    qPic_p dat = Cache_Check(&_cachePics[i].cache);
    if (dat)
        return dat;

    //
    // load the pic from disk
    //
    COM_LoadCacheFile(path, &_cachePics[i].cache);

    dat = (qPic_p)_cachePics[i].cache.data;
    if (!dat)
        Host_SysError("Draw_CachePic: failed to load %s", path);

    SwapPic(dat);

    return dat;
}
#endif
