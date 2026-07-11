#include "GameRule.h"
#include "server.h"
#include "q_tools.h"

static Skill_t _current_skill;  // skill level for currently loaded level (in case the user changes the cvar while the level is running, this reflects the level actually in use)

void GM_SetSkill(Skill_t skill) {
    ClampInRange(SkEasy, &skill, SkNightmare);
    _current_skill = skill;
}
Skill_t GM_GetSkill() { return _current_skill; }

int GetSvMaxClients() { return svs.maxClients; }
int GetSvMaxClientsLimit() { return svs.maxClientsLimit; }

bool isMultiplayer() { return GetSvMaxClients() > 1; }
bool isSingleGame() { return !isMultiplayer(); }
// svs.clients