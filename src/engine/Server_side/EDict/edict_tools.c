#include "Edict.h"
#include "host.h"
#include "server.h"

edict_p ED_GetEDictByIdx(uint32_t idx) {
    if (
        // (idx < 0) ||
        (idx >= sv.max_edicts))
        Host_SysError("ED_GetEDictByIdx: bad index %i", idx);

    return (edict_p)((uint8_p)sv.edicts + ((idx) * pr_edict_size));
}

uint32_t ED_GetEDictIdx(edict_p edict) {
    uint32_t idx = (uint32_t)((uint8_p)edict - (uint8_p)sv.edicts) / pr_edict_size;

    if (
        // (idx < 0) ||
        (idx >= sv.num_edicts))
        Host_SysError("ED_GetEDictIdx: bad pointer");

    return idx;
}

edict_p ED_GetEDictByOffs(int32_t offs) {
    return  ((edict_p)((uint8_p)sv.edicts + (uint32_t)(offs)));
}
int32_t ED_GetEDictOffs(edict_p ePtr) {
    return  ((int32_t)((uint8_p)(ePtr) - (uint8_p)sv.edicts));
}

edict_p ED_GetEDictFirst() {
    return ED_GetEDictNext(sv.edicts);  // first non word entity
}
edict_p ED_GetEDictNext(edict_p edict) {
    return  ((edict_p)((uint8_p)(edict) + pr_edict_size));
}
