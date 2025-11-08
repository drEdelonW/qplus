#pragma once

#define NAME_LENGTH         64

//
// per-level limits
//
#define MAX_EDICTS          600   // FIXME: ouch! ouch! ouch!
#define MAX_LIGHTSTYLES     64
#define MAX_MODELS          256   // these are sent over the net as bytes
#define MAX_SOUNDS          256   // so they cannot be blindly increased

#define MAX_SCOREBOARD      16
#define MAX_SCOREBOARDNAME  32

#define MAX_FILES_IN_PACK   2048

#define MIPLEVELS           4
#define MAXLIGHTMAPS        4

#define SAVEGAME_COMMENT_LENGTH 39

#define MAX_PARTICLES           2048 /* default max # of particles at one time */
#define ABSOLUTE_MIN_PARTICLES  512  // no fewer than this no matter what's on the command line

#define MAX_EFRAGS              640

#define MAX_MAPSTRING           2048
#define MAX_DEMOS               8
#define MAX_DEMONAME            16

#define MAX_TEMP_ENTITIES       64   // lightning bolts, etc
#define MAX_STATIC_ENTITIES     128   // torches, etc

#define MAX_VISEDICTS           256

#define MAX_DLIGHTS             32
