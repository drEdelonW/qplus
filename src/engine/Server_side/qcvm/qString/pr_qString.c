#include "progs.h"
#include "types.h"
#include "host.h"

static cString  _pr_strings;         // much more // should be static

void initProgSrting(dprograms_p progs){
    PR_ClearAppStrings();
    _pr_strings = (cString)((uint8_p)progs + progs->strings.ofs);
}

typedef int32_t qVmString_t;

#define PR_APPSTR_MAX 4096

static cString      _pr_appstrings[PR_APPSTR_MAX];
static qVmString_t  _pr_appstrings_num = 0;

void PR_ClearAppStrings() {
    for (qVmString_t i = 0; i < _pr_appstrings_num; ++i) {
        _pr_appstrings[i] = NULL;
    }

    _pr_appstrings_num = 0;
}

qVmString_t PR_SetQString(cString str) {
    if (!str)   return 0;

    ptrdiff_t delta = str - _pr_strings;

    if ((delta >= 0) &&
        (delta <= INT32_MAX))
        return (qVmString_t)delta;

    for (qVmString_t i = 0; i < _pr_appstrings_num; ++i)
        if (_pr_appstrings[i] == str)
            return -(i + 1);

    if (_pr_appstrings_num >= PR_APPSTR_MAX)
        Host_SysError("PR_SetQString: app string table overflow");

    qVmString_t idx = _pr_appstrings_num;

    _pr_appstrings[_pr_appstrings_num] = str;
    _pr_appstrings_num++;

    return -(idx + 1);
}

cString PR_GetQString(qVmString_t offs) {
    int32_t idx;

    if (offs >= 0)
        return _pr_strings + offs;

    idx = -offs - 1;

    if ((idx < 0) || (idx >= _pr_appstrings_num))
        Host_SysError("PR_GetQString: bad app string index %d", idx);


    return _pr_appstrings[idx];
}
