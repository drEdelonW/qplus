#pragma once

#include "link.h"
#include "pr_def.h"    // defs shared with qcc
#include "progdefs.h"       //for entvars_t
#include "EntityState.h"
#include "vmValue.h"


// edict->movetype values
typedef enum {
    MOVETYPE_NONE           = 0u,  // never moves
    MOVETYPE_ANGLENOCLIP    = 1u,
    MOVETYPE_ANGLECLIP      = 2u,
    MOVETYPE_WALK           = 3u,  // gravity
    MOVETYPE_STEP           = 4u,  // gravity, special edge handling
    MOVETYPE_FLY            = 5u,
    MOVETYPE_TOSS           = 6u,  // gravity
    MOVETYPE_PUSH           = 7u,  // no clip to world, push and crush
    MOVETYPE_NOCLIP         = 8u,
    MOVETYPE_FLYMISSILE     = 9u,  // extra size to monsters
    MOVETYPE_BOUNCE         = 10u,
#ifdef QUAKE2
    MOVETYPE_BOUNCEMISSILE  = 11u, // bounce w/o gravity
    MOVETYPE_FOLLOW         = 12u, // track movement of aiment
#endif
} movetype_t;   // sv_phys.c


// edict->solid values
typedef enum {
    SOLID_NOT       = 0u, // no interaction with other objects
    SOLID_TRIGGER   = 1u, // touch on edge, but not blocking
    SOLID_BBOX      = 2u, // touch on edge, block
    SOLID_SLIDEBOX  = 3u, // touch on edge, but not an onground
    SOLID_BSP       = 4u  // BSP clip, touch on edge, block
} solid_t;      // world.c

// edict->deadflag values
typedef enum {
    DEAD_NO     = 0u, // alive
    DEAD_DYING  = 1u, // in the process of dying
    DEAD_DEAD   = 2u  // fully dead
} deadflag_t;

typedef enum {
    DAMAGE_NO   = 0u, // does not take damage
    DAMAGE_YES  = 1u, // always takes damage
    DAMAGE_AIM  = 2u  // takes damage only with aim
} damage_t;


// edict->flags
typedef enum {
    FL_FLY              = 1u << 0,   // 0000...0001
    FL_SWIM             = 1u << 1,   // 0000...0010
    // FL_GLIMPSE        = 1u << 1,
    FL_CONVEYOR         = 1u << 2,   // 0000...0100
    FL_CLIENT           = 1u << 3,   // 0000...1000
    FL_INWATER          = 1u << 4,   // 0001...0000
    FL_MONSTER          = 1u << 5,   // 0010...0000
    FL_GODMODE          = 1u << 6,
    FL_NOTARGET         = 1u << 7,
    FL_ITEM             = 1u << 8,
    FL_ONGROUND         = 1u << 9,
    FL_PARTIALGROUND    = 1u << 10,  // not all corners are valid
    FL_WATERJUMP        = 1u << 11,  // player jumping out of water
    FL_JUMPRELEASED     = 1u << 12,  // for jump debouncing
#ifdef QUAKE2
    FL_FLASHLIGHT       = 1u << 13,
    FL_ARCHIVE_OVERRIDE = 1u << 20
#endif
} EntityFlags_t;   // sv_phys.c


typedef enum {
    SPAWNFLAG_NOT_EASY          = 1u << 8,  // 0x0100
    SPAWNFLAG_NOT_MEDIUM        = 1u << 9,  // 0x0200
    SPAWNFLAG_NOT_HARD          = 1u << 10, // 0x0400
    SPAWNFLAG_NOT_DEATHMATCH    = 1u << 11  // 0x0800
} SpawnFlags_t;

#ifdef QUAKE2
// server flags
typedef enum {
    SFL_EPISODE_1       = 1u << 0,   // 0x0001
    SFL_EPISODE_2       = 1u << 1,   // 0x0002
    SFL_EPISODE_3       = 1u << 2,   // 0x0004
    SFL_EPISODE_4       = 1u << 3,   // 0x0008
    SFL_NEW_UNIT        = 1u << 4,   // 0x0010
    SFL_NEW_EPISODE     = 1u << 5,   // 0x0020
    SFL_CROSS_TRIGGERS  = 0xFF00u    // 65280, covers multiple bits
} spawnlevel_flags_t;
#endif


#define MAX_ENT_LEAFS (16)
typedef struct edict_s {
    bool            free;
    link_t          area;       // linked to a division node or leaf
    int32_t         num_leafs;
    int16_t         leafnums[MAX_ENT_LEAFS];
    EntityState_t   baseline;
    float           freetime;   // sv.time when the object was freed
    entvars_t       v;          // C exported fields from progs
    // other fields from progs come immediately after
} edict_t;
typedef edict_t* edict_p;
#define PROG_HEADER_SIZE (sizeof(edict_t) - sizeof(entvars_t))

#define EDICT_FROM_AREA(l)  STRUCT_FROM_LINK(l, edict_t, area)
#define G_EDICT(o)          ED_GetEDictByOffs((uint32_t)G_INT((o)))
#define G_EDICTNUM(o)       ED_GetEDictIdx(G_EDICT((o)))

#ifdef __cplusplus
extern "C" {
#endif
    void ED_Init();

    edict_p ED_Alloc();
    void ED_Free(edict_p ed);

    cString ED_NewString(cString string);   // returns a copy of the string allocated from the server's string heap

    void ED_Print(edict_p ed);
    void ED_PrintEdicts();
    void ED_PrintNum(uint32_t ent);

    void ED_Write(FILE* f, edict_p ed);

    cString ED_ParseEdict(cString data, edict_p ent);

    void ED_LoadFromFile(cString data);
    bool ED_ParseEpair(TypeLess_ptr base, dDef_p key, cString s);

    edict_p ED_GetEDictByIdx(uint32_t idx);
    uint32_t ED_GetEDictIdx(edict_p edict);

    edict_p ED_GetEDictByOffs(int32_t offs);
    int32_t ED_GetEDictOffs(edict_p edict);

    edict_p ED_GetEDictFirst();
    edict_p ED_GetEDictNext(edict_p edict);

    eval_p GetEdictFieldValue(edict_p ed, cString field);

    edict_p FindViewthing();
#ifdef __cplusplus
}
#endif

extern uint32_t EdictSize;  // in bytes
extern edict_p  Edicts;
extern uint32_t EdictsMax;
extern uint32_t EdictsNum;
