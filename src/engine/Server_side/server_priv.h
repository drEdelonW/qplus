#pragma once
#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

    void SV_StartParticle(vec3_t org, vec3_t dir, uint8_t color, uint8_t count);
    void SV_SetIdealPitch();
    void SV_MoveToGoal();
    bool SV_movestep(edict_p ent, vec3_t move, bool relink);
    int  SV_ModelIndex(cString name);

#ifdef __cplusplus
}
#endif