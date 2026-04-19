#include "Edict.h"
#include "host.h"
#include "server.h"

edict_p ED_GetEDictByIdx(uint32_t idx) {
    if (
        // (idx < 0) ||
        (idx >= EdictsMax))
        Host_SysError("ED_GetEDictByIdx: bad index %i", idx);

    return (edict_p)((uint8_p)Edicts + ((idx) * pr_edict_size));
}

uint32_t ED_GetEDictIdx(edict_p edict) {
    uint32_t idx = (uint32_t)((uint8_p)edict - (uint8_p)Edicts) / pr_edict_size;

    if (
        // (idx < 0) ||
        (idx >= EdictsNum))
        Host_SysError("ED_GetEDictIdx: bad pointer");

    return idx;
}

edict_p ED_GetEDictByOffs(int32_t offs) {
    return  ((edict_p)((uint8_p)Edicts + (uint32_t)(offs)));
}
int32_t ED_GetEDictOffs(edict_p ePtr) {
    return  ((int32_t)((uint8_p)(ePtr) - (uint8_p)Edicts));
}

edict_p ED_GetEDictFirst() {
    return ED_GetEDictNext(Edicts);  // first non word entity
}
edict_p ED_GetEDictNext(edict_p edict) {
    return  ((edict_p)((uint8_p)(edict) + pr_edict_size));
}
