#include "VA.h"

va_list argptr;

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
cString va(cStringRO format, ...) {
    static char string[1024]; 
    VA_EXPAND(string, format);
    return string;
}