#pragma once
#include "types.h"

#define MAX_SCOREBOARD      16
#define MAX_SCOREBOARDNAME  32

typedef enum {
    SkEasy      = 0,
    SkMedium    = 1,
    SkHard      = 2,
    SkNightmare = 3,
} Skill_t;

Skill_t GM_GetSkill();
void    GM_SetSkill(Skill_t);

#ifdef __cplusplus
extern "C" {
#endif

    int GetSvMaxClients();
    int GetSvMaxClientsLimit();
    bool isMultiplayer();
    bool isSingleGame();
    bool isSvPaused();

    bool isIntermission();

#ifdef __cplusplus
}
#endif