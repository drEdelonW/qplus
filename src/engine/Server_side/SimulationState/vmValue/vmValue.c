#include "vmValue.h"
#include "common.h"
#include "Edict.h"



int type_size[ev_LAST] = {
    1,                          // ev_void,
    sizeof(string_t) / 4,       // ev_string,
    1,                          // ev_float,
    3,                          // ev_vector,
    1,                          // ev_entity,
    1,                          // ev_field,
    sizeof(func_t) / 4,         // ev_function,
    sizeof(TypeLess_ptr) / 4    // ev_pointer
};

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
static char _line[256];
cString PR_ValueString(etype_t type, eval_p val) {
    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
        case ev_string:     snprintf(_line, sizeof(_line), "%s", PR_GetQString(val->string));                                       break;
        case ev_entity:     snprintf(_line, sizeof(_line), "entity %i", ED_GetEDictIdx(ED_GetEDictByOffs(val->edict)));             break;
        case ev_function:   snprintf(_line, sizeof(_line), "%s()", PR_GetQString((pr_functions + val->function)->s_name));          break;
        case ev_field:      snprintf(_line, sizeof(_line), ".%s", PR_GetQString(ED_FieldAtOfs(val->_int)->s_name));                 break;
        case ev_void:       snprintf(_line, sizeof(_line), "void");                                                                 break;
        case ev_float:      snprintf(_line, sizeof(_line), "%5.1f", val->_float);                                                   break;
        case ev_vector:     snprintf(_line, sizeof(_line), "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);  break;
        case ev_pointer:    snprintf(_line, sizeof(_line), "pointer");                                                              break;
        default:            snprintf(_line, sizeof(_line), "bad type %i", type);                                                    break;
    }

    return _line;
}


/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
cString PR_UglyValueString(etype_t type, eval_p val) {
    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
        case ev_string:     snprintf(_line, sizeof(_line), "%s", PR_GetQString(val->string));                           break;
        case ev_entity:     snprintf(_line, sizeof(_line), "%i", ED_GetEDictIdx(ED_GetEDictByOffs(val->edict)));             break;
        case ev_function:   snprintf(_line, sizeof(_line), "%s", PR_GetQString((pr_functions + val->function)->s_name));break;
        case ev_field:      snprintf(_line, sizeof(_line), "%s", PR_GetQString(ED_FieldAtOfs(val->_int)->s_name));      break;
        case ev_void:       snprintf(_line, sizeof(_line), "void");                                                     break;
        case ev_float:      snprintf(_line, sizeof(_line), "%f", val->_float);                                          break;
        case ev_vector:     snprintf(_line, sizeof(_line), "%f %f %f", val->vector[0], val->vector[1], val->vector[2]); break;
        default:            snprintf(_line, sizeof(_line), "bad type %i", type);                                        break;
    }

    return _line;
}

