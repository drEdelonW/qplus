#include "Edict.h"
#include "host.h"


size_t  EdictSize = 0;      // in bytes

static inline EdIdx CheckEdictIdx(EdIdx idx) {
    if (
        (idx < EdictWorld) ||
        (idx >= EdictMax)
        )   Host_SysError("ED_GetEDictByIdx: bad index %i", idx);
        return idx;
}

edict_p ED_GetEDictByIdx(EdIdx idx) {
    CheckEdictIdx(idx);
    return (edict_p)((uint8_p)Edicts + (idx * GetEdictSize()));
}

EdIdx ED_GetEDictIdx(edict_p edict) {
    EdIdx idx = (EdIdx)((uint8_p)edict - (uint8_p)Edicts) / GetEdictSize();
    CheckEdictIdx(idx);
    return idx;
}

edict_p ED_GetEDictByOffs(int32_t offs) {
    return  ((edict_p)((uint8_p)Edicts + (uint32_t)(offs)));
}

int32_t ED_GetEDictOffs(edict_p ePtr) {
    return  ((int32_t)((uint8_p)(ePtr)-(uint8_p)Edicts));
}

edict_p ED_GetEDictFirst() {
    return ED_GetEDictNext(Edicts);  // first non word entity
}

edict_p ED_GetEDictNext(edict_p edict) {
    return  (edict_p)((uint8_p)edict + GetEdictSize());
}
