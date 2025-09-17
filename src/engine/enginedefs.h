#pragma once

#include "mathlib.h"

//
// per-level limits
//
#define	MAX_EDICTS		600			// FIXME: ouch! ouch! ouch!
#define	MAX_LIGHTSTYLES	64
#define	MAX_MODELS		256			// these are sent over the net as bytes
#define	MAX_SOUNDS		256			// so they cannot be blindly increased

#define	MAX_SCOREBOARD		16
#define	MAX_SCOREBOARDNAME	32



#define	MAX_CL_STATS		32


typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	int		modelindex;
	int		frame;
	int		colormap;
	int		skin;
	int		effects;
} entity_state_t;
