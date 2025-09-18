#pragma once

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


void    Q_memset(void* dest, int   fill, int count);
void    Q_memcpy(void* dest, void* src,  int count);
int     Q_memcmp(void* m1,   void* m2,   int count);

int     Q_strlen(char*  str);
void    Q_strcpy(char*  dest, char* src);
void    Q_strncpy(char* dest, char* src, int count);
void    Q_strcat(char*  dest, char* src);
char*   Q_strrchr(char* s,  char  c);
int     Q_strcmp(char*      s1, char* s2);
int     Q_strncmp(char*     s1, char* s2, int count);
int     Q_strcasecmp(char*  s1, char* s2);
int     Q_strncasecmp(char* s1, char* s2, int n);

int	    Q_atoi(char* str);
float   Q_atof(char* str);
