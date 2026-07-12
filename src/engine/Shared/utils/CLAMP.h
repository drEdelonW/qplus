#pragma once

/* if (var_p < min_val)   var_p = min_val; */
#define ClampLessThen(var_p, min_val)        do{ if (*(var_p) < (min_val)) {*(var_p) = (min_val);} }while(0)
/* keep (var_p >= min_val) */

/* if (var_p > max_val)   var_p = max_val; */
#define ClampMoreThen(var_p, max_val)        do{ if (*(var_p) > (max_val)) {*(var_p) = (max_val);} }while(0)
/* keep (var_p <= max_val) */

/* keep (min_val < var_p < max_val) */
#define ClampInRange(min_val, var_p, max_val)    do{ if (*(var_p) < (min_val)) {*(var_p) = (min_val); }else ClampMoreThen(var_p, max_val); }while(0)

/*  =========================================================
    VS-Code Regular search — RUN PAIRED FIRST, SINGLES AFTER
    =========================================================

    --- PAIRED: ClampInRange (4 variants) ---

    [A] X < min  THEN  X > max   (no else)
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n\s*if\s*\(\s*\1\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: ClampInRange($2, &$1, $3);

    [B] X > max  THEN  X < min   (no else)
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n\s*if\s*\(\s*\1\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: ClampInRange($3, &$1, $2);

    [C] X > max  ELSE IF  X < min
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*else\s+if\s*\(\s*\1\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: ClampInRange($3, &$1, $2);

    [D] X < min  ELSE IF  X > max
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*else\s+if\s*\(\s*\1\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: ClampInRange($2, &$1, $3);

    --- SINGLE / REVERSE: ClampLessThen, ClampMoreThen (4 variants) ---

    [1] self ClampLessThen:  if (var < / <= val) var = val;
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*<=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*(?:\{\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*\}|\r?\n?\s*\1\s*=\s*\2\s*;)
    Replace: ClampLessThen(&$1, $2);

    [2] self ClampMoreThen:  if (var > / >= val) var = val;
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*(?<!-)>=?\s*([^()]+(?:\([^()]*\)[^()]*)*)\s*\)\s*(?:\{\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*\}|\r?\n?\s*\1\s*=\s*\2\s*;)
    Replace: ClampMoreThen(&$1, $2);

    [3] reverse ClampMoreThen:  if (A < / <= B) B = A;   (B bounded above by A)
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*<=?\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*\)\s*(?:\{\s*\r?\n?\s*\2\s*=\s*\1\s*;\s*\r?\n?\s*\}|\r?\n?\s*\2\s*=\s*\1\s*;)
    Replace: ClampMoreThen(&$2, $1);

    [4] reverse ClampLessThen:  if (A > / >= B) B = A;   (B bounded below by A)
    Find:    if\s*\(\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*(?<!-)>=?\s*([\w.\[\]]+(?:->[\w.\[\]]+)*)\s*\)\s*(?:\{\s*\r?\n?\s*\2\s*=\s*\1\s*;\s*\r?\n?\s*\}|\r?\n?\s*\2\s*=\s*\1\s*;)
    Replace: ClampLessThen(&$2, $1);
*/

#define SmallerOf(a, b)    (((a) <= (b)) ? (a) : (b))
/* --- SmallerOf ---
    Find:    \(\s*([^()?]+?)\s*<=?\s*([^()?]+?)\s*\)\s*\?\s*\1\s*:\s*\2
    Replace: SmallerOf($1, $2)
*/
#define LargerOf(a, b)     (((a) >= (b)) ? (a) : (b))
/* --- LargerOf ---
    Find:    \(\s*([^()?]+?)\s*(?<!-)>=?\s*([^()?]+?)\s*\)\s*\?\s*\1\s*:\s*\2
    Replace: LargerOf($1, $2)
*/