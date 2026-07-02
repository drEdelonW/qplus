#include "GameRule.h"
#include "server.h"

int32_t current_skill;

int GetSvMaxClients() { return svs.maxClients; }
int GetSvMaxClientsLimit() { return svs.maxClientsLimit; }

bool isMultiplayer() { return GetSvMaxClients() > 1; }
bool isSingleGame()  { return !isMultiplayer(); }
// svs.clients