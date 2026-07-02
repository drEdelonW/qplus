#pragma once
#include "types.h"

extern int current_skill;  // skill level for currently loaded level (in case the user changes the cvar while the level is running, this reflects the level actually in use)

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