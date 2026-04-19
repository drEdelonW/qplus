#pragma once
#include "server.h"


#ifdef __cplusplus
extern "C" {
#endif

    void SV_StartParticle(vec3_t org, vec3_t dir, int color, size_t count);
    void SV_SetIdealPitch();
    void SV_MoveToGoal();
    bool SV_movestep(edict_p ent, vec3_t move, bool relink);
    bool SV_CheckBottom(edict_p ent);
    int  SV_ModelIndex(cString name);
    void SV_ClearWorld();

#ifdef __cplusplus
}
#endif