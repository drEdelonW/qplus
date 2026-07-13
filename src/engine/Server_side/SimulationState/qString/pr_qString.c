#include "pr_qString.h"
#include "types.h"
#include "host.h"

static cString  _pr_strings;    // much more // should be static
void initProgString(progLump_t pl) {
    PR_ClearAppStrings();
    _pr_strings = GetPtrFromLump(pl);
}

#define PR_APPSTR_MAX 10
static cString  _appStrings[PR_APPSTR_MAX];
static uint8_t  _appStrings_num = 0;

void PR_ClearAppStrings() {
    for (qVmString_t i = 0; i < _appStrings_num; ++i)
        _appStrings[i] = NULL;
    _appStrings_num = 0;
}

// #include "console.h" // Con_DPrintf
qVmString_t PR_SetQString(cString str) {
    if (!str)   return 0;

    ptrdiff_t delta = str - _pr_strings;
    if ((delta >= 0) &&
        (delta <= INT32_MAX)
        )   return (qVmString_t)delta;  // return "Prog.dat space" string

    {
        for (qVmString_t i = 0; i < _appStrings_num; ++i)
            if (_appStrings[i] == str)
                return -(i + 1);    // return "app space" string from cache
    }
    {
        if (_appStrings_num >= PR_APPSTR_MAX)
            Host_SysError("PR_SetQString: app string table overflow");
        else {
            _appStrings[_appStrings_num++] = str; // Con_DPrintf("_appStrings_num %d\n", _appStrings_num);
            return -(_appStrings_num); // cache and return "app space" strings
        }
    }
}

cString PR_GetQString(qVmString_t offs) {
    if (offs >= 0)
        return _pr_strings + offs; // return "Prog.dat space" string

    int32_t idx = -(offs + 1);
    if ((idx < 0) ||
        (idx >= _appStrings_num)
        )   Host_SysError("PR_GetQString: bad app string index %d", idx);

    return _appStrings[idx]; // return "app space" string from cache
}
