#include "GameRule.h"
#include "server.h"

int32_t current_skill;

bool isMultiplayer() { return svs.maxClients > 1; }
bool isSingleGame()  { return !isMultiplayer(); }
int GetSvMaxClients() { return svs.maxClients; }
