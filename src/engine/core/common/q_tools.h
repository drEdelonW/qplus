#pragma once
#include "types.h"

// if (var < min_val)   var = min_val;
#define CLAMP_MIN(var, min_val)         do{ if ((var) < (min_val)) {(var) = (min_val);} }while(0)
// keep (var >= min_val)
#define CLAMP_LESS(var, min_val)        CLAMP_MIN(var, min_val)


// if (var > max_val)   var = max_val;
#define CLAMP_MAX(var, max_val)         do{ if ((var) > (max_val)) {(var) = (max_val);} }while(0)
// keep (var <= max_val)
#define CLAMP_MORE(var, max_val)        CLAMP_MAX(var, max_val)

// keep (min_val < var < max_val)
#define CLAMP(min_val, var, max_val)    do{ if ((var) < (min_val)) {(var) = (min_val); }else CLAMP_MAX(var, max_val); }while(0)

#ifdef __cplusplus
extern "C" {
#endif

void Q_memset(TypeLess_ptr dest, int32_t fill, int32_t count);
void Q_memcpy(TypeLess_ptr dest, TypeLess_ptr src, int32_t count);
int Q_memcmp(TypeLess_ptr m1, TypeLess_ptr m2, int32_t count);

int Q_strlen(cStringRO str);
void Q_strcpy(cString  dest, cString src);
void Q_strncpy(cString dest, cString src, int32_t count);
void Q_strcat(cString  dest, cString src);
cString Q_strrchr(cString s, char  c);
int Q_strcmp(cStringRO     s1, cStringRO s2);
int Q_strncmp(cString     s1, cString s2, int32_t count);
int Q_strcasecmp(cString  s1, cString s2);
int Q_strncasecmp(cString s1, cString s2, int32_t n);

int	Q_atoi(cString str);
float Q_atof(cString str);

#ifdef __cplusplus
}
#endif