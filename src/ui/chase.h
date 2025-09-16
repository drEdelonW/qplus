#pragma once
#include "cvar.h"
//
// chase
//

CVAR_EXTERN(chase_active);

void Chase_Init (void);
void Chase_Reset (void);
void Chase_Update (void);
