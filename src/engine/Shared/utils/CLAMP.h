#pragma once

/* if (var_p < min_val)   var_p = min_val; */
#define CLAMP_MIN(var_p, min_val)         do{ if (*(var_p) < (min_val)) {*(var_p) = (min_val);} }while(0)
/* keep (var_p >= min_val) */
#define CLAMP_LESS(var_p, min_val)        CLAMP_MIN(var_p, min_val)


/* if (var_p > max_val)   var_p = max_val; */
#define CLAMP_MAX(var_p, max_val)         do{ if (*(var_p) > (max_val)) {*(var_p) = (max_val);} }while(0)
/* keep (var_p <= max_val) */
#define CLAMP_MORE(var_p, max_val)        CLAMP_MAX(var_p, max_val)

/* keep (min_val < var_p < max_val) */
#define CLAMP(min_val, var_p, max_val)    do{ if (*(var_p) < (min_val)) {*(var_p) = (min_val); }else CLAMP_MAX(var_p, max_val); }while(0)

