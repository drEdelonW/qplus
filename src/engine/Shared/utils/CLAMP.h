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

/*  =========================================================
    VS-Code Regular search — RUN PAIRED FIRST, SINGLES AFTER
    =========================================================

    --- PAIRED: CLAMP (4 variants) ---

    [A] X < min  THEN  X > max   (no else)
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n\s*if\s*\(\s*\1\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: CLAMP($2, &$1, $3);

    [B] X > max  THEN  X < min   (no else)
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n\s*if\s*\(\s*\1\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: CLAMP($3, &$1, $2);

    [C] X > max  ELSE IF  X < min
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*else\s+if\s*\(\s*\1\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: CLAMP($3, &$1, $2);

    [D] X < min  ELSE IF  X > max
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*else\s+if\s*\(\s*\1\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: CLAMP($2, &$1, $3);

    --- SINGLE / REVERSE: CLAMP_LESS, CLAMP_MORE (4 variants) ---

    [1] self CLAMP_LESS:  if (var < / <= val) var = val;
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*(?:\{\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*\}|\r?\n?\s*\1\s*=\s*\2\s*;)
    Replace: CLAMP_LESS(&$1, $2);

    [2] self CLAMP_MORE:  if (var > / >= val) var = val;
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*(?:\{\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*\}|\r?\n?\s*\1\s*=\s*\2\s*;)
    Replace: CLAMP_MORE(&$1, $2);

    [3] reverse CLAMP_MORE:  if (A < / <= B) B = A;   (B bounded above by A)
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*<=?\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*\)\s*(?:\{\s*\r?\n?\s*\2\s*=\s*\1\s*;\s*\r?\n?\s*\}|\r?\n?\s*\2\s*=\s*\1\s*;)
    Replace: CLAMP_MORE(&$2, $1);

    [4] reverse CLAMP_LESS:  if (A > / >= B) B = A;   (B bounded below by A)
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*(?<!-)>=?\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*\)\s*(?:\{\s*\r?\n?\s*\2\s*=\s*\1\s*;\s*\r?\n?\s*\}|\r?\n?\s*\2\s*=\s*\1\s*;)
    Replace: CLAMP_LESS(&$2, $1);
*/