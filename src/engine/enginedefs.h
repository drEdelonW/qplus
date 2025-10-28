#pragma once



//
// per-level limits
//
#define MAX_EDICTS      600   // FIXME: ouch! ouch! ouch!
#define MAX_LIGHTSTYLES 64
#define MAX_MODELS      256   // these are sent over the net as bytes
#define MAX_SOUNDS      256   // so they cannot be blindly increased

#define MAX_SCOREBOARD      16
#define MAX_SCOREBOARDNAME  32

#define MIPLEVELS       4
#define MAXLIGHTMAPS    4

#define SAVEGAME_COMMENT_LENGTH 39




#include <stdbool.h>
extern bool noclip_anglehack;
