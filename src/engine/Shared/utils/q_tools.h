#pragma once
#include "types.h"

#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

    void Q_memset(TypeLess_ptr dest, int32_t fill, uint32_t count);
    void Q_memcpy(TypeLess_ptr dest, TypeLess_ptr src, int32_t count);
    int Q_memcmp(TypeLess_ptr m1, TypeLess_ptr m2, int32_t count);

    uint32_t Q_strlen(cStringRO str);
    void Q_strcpy(cString  dest, cStringRO src);
    void Q_strncpy(cString dest, cStringRO src, int32_t count);
    void Q_strcat(cString  dest, cStringRO src);
    cString Q_strrchr(cString s, char  c);
    int Q_strcmp(cStringRO      s1, cStringRO s2);
    int Q_strncmp(cStringRO     s1, cStringRO s2, uint32_t count);
    int Q_strcasecmp(cStringRO  s1, cStringRO s2);
    int Q_strncasecmp(cStringRO s1, cStringRO s2, int32_t n);

    int Q_atoi(cStringRO str);
    float Q_atof(cStringRO str);

#ifdef __cplusplus
}
#endif