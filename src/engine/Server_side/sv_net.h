#pragma once
#include "server.h"

void SV_SendServerinfo(RmtClient_p client);
void SV_WriteEntitiesToClient(edict_p clent, sizebuf_p msg);
bool SV_SendClientDatagram(RmtClient_p client);
void SV_UpdateToReliableMessages();
void SV_CreateBaseline();
void SV_SendNop(RmtClient_p client);
void SV_SendReconnect();

uint8_p SV_FatPVS(vec3_t org);
