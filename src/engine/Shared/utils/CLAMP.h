#pragma once

/* if (var_p < min_val)   var_p = min_val; */
#define CLAMP_MIN(var_p, min_val)         do{ if (*(var_p) < (min_val)) {*(var_p) = (min_val);} }while(0)
/* keep (var_p >= min_val) */
#define CLAMP_LESS(var_p, min_val)        CLAMP_MIN(var_p, min_val)
/*  VS-Code Regular search
    Find:    if\s*\(\s*([\w.\[\]]+)\s*<\s*([^)]+?)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;
    Replace: CLAMP_LESS(&$1, $2);
*/


/* if (var_p > max_val)   var_p = max_val; */
#define CLAMP_MAX(var_p, max_val)         do{ if (*(var_p) > (max_val)) {*(var_p) = (max_val);} }while(0)
/* keep (var_p <= max_val) */
#define CLAMP_MORE(var_p, max_val)        CLAMP_MAX(var_p, max_val)
/*  VS-Code Regular search
    Find:    if\s*\(\s*([\w.\[\]]+)\s*>\s*([^)]+?)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;
    Replace: CLAMP_MORE(&$1, $2);
*/


/* keep (min_val < var_p < max_val) */
#define CLAMP(min_val, var_p, max_val)    do{ if (*(var_p) < (min_val)) {*(var_p) = (min_val); }else CLAMP_MAX(var_p, max_val); }while(0)
/*  VS-Code Regular search
    [<] then [>], WithOUT [else]
    Find:    if\s*\(\s*([\w.\[\]]+)\s*<\s*([^)]+?)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n\s*if\s*\(\s*\1\s*>\s*([^)]+?)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: CLAMP($2, &$1, $3);

    [>] then [else if], [<]
    Find:    if\s*\(\s*([\w.\[\]]+)\s*>\s*([^)]+?)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n?\s*else\s+if\s*\(\s*\1\s*<\s*([^)]+?)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: CLAMP($3, &$1, $2);

    [>] then [<], WithOUT [else]
    Find:    if\s*\(\s*([\w.\[\]]+)\s*>\s*([^)]+?)\s*\)\s*\r?\n?\s*\1\s*=\s*\2\s*;\s*\r?\n\s*if\s*\(\s*\1\s*<\s*([^)]+?)\s*\)\s*\r?\n?\s*\1\s*=\s*\3\s*;
    Replace: CLAMP($3, &$1, $2);
*/

