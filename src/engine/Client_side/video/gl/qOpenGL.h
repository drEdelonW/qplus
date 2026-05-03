#pragma once

#include "enginedefs.h"
#include "types.h"
#include "vid.h"
#include "vector.h"
#include "Surface.h"
#include "Leaf.h"
#include "render.h"
#include "Texture.h"
#include "cvar.h"

#include "glquake.h"

extern uint8_t	d_15to8table[65536];

bool VID_Is8bit(void);