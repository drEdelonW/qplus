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


void    Q_memset(typeless_ptr dest, int   fill, int32_t count);
void    Q_memcpy(typeless_ptr dest, typeless_ptr src, int32_t count);
int     Q_memcmp(typeless_ptr m1, typeless_ptr m2, int32_t count);

int     Q_strlen(cstring  str);
void    Q_strcpy(cstring  dest, cstring src);
void    Q_strncpy(cstring dest, cstring src, int32_t count);
void    Q_strcat(cstring  dest, cstring src);
cstring   Q_strrchr(cstring s, char  c);
int     Q_strcmp(cstring      s1, cstring s2);
int     Q_strncmp(cstring     s1, cstring s2, int32_t count);
int     Q_strcasecmp(cstring  s1, cstring s2);
int     Q_strncasecmp(cstring s1, cstring s2, int32_t n);

int	    Q_atoi(cstring str);
float   Q_atof(cstring str);
