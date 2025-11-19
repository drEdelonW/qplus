#pragma once

#include "types.h"

// stock defines
typedef enum {
    IT_SHOTGUN          = (1u << 0),
    IT_SUPER_SHOTGUN    = (1u << 1),
    IT_NAILGUN          = (1u << 2),
    IT_SUPER_NAILGUN    = (1u << 3),
    IT_GRENADE_LAUNCHER = (1u << 4),
    IT_ROCKET_LAUNCHER  = (1u << 5),
    IT_LIGHTNING        = (1u << 6),
    IT_SUPER_LIGHTNING  = (1u << 7),

    IT_SHELLS           = (1u << 8),
    IT_NAILS            = (1u << 9),
    IT_ROCKETS          = (1u << 10),
    IT_CELLS            = (1u << 11),
    IT_AXE              = (1u << 12),

    IT_ARMOR1           = (1u << 13),
    IT_ARMOR2           = (1u << 14),
    IT_ARMOR3           = (1u << 15),

    IT_SUPERHEALTH      = (1u << 16),
    IT_KEY1             = (1u << 17),
    IT_KEY2             = (1u << 18),

    IT_INVISIBILITY     = (1u << 19),
    IT_INVULNERABILITY  = (1u << 20),
    IT_SUIT             = (1u << 21),
    IT_QUAD             = (1u << 22),

    IT_SIGIL1           = (1u << 28),
    IT_SIGIL2           = (1u << 29),
    IT_SIGIL3           = (1u << 30),
    IT_SIGIL4           = (1u << 31)
} item_bits_t;
// #define IT_SIGIL4 (1u << 31)

//===========================================
//rogue changed and added defines

typedef enum {
    RIT_SHELLS             = (1u << 7),   // 128
    RIT_NAILS              = (1u << 8),   // 256
    RIT_ROCKETS            = (1u << 9),   // 512
    RIT_CELLS              = (1u << 10),  // 1024
    RIT_AXE                = (1u << 11),  // 2048

    RIT_LAVA_NAILGUN       = (1u << 12),  // 4096
    RIT_LAVA_SUPER_NAILGUN = (1u << 13),  // 8192
    RIT_MULTI_GRENADE      = (1u << 14),  // 16384
    RIT_MULTI_ROCKET       = (1u << 15),  // 32768
    RIT_PLASMA_GUN         = (1u << 16),  // 65536

    RIT_ARMOR1             = (1u << 23),  // 8388608
    RIT_ARMOR2             = (1u << 24),  // 16777216
    RIT_ARMOR3             = (1u << 25),  // 33554432

    RIT_LAVA_NAILS         = (1u << 26),  // 67108864
    RIT_PLASMA_AMMO        = (1u << 27),  // 134217728
    RIT_MULTI_ROCKETS      = (1u << 28),  // 268435456
    RIT_SHIELD             = (1u << 29),  // 536870912
    RIT_ANTIGRAV           = (1u << 30),  // 1073741824
    RIT_SUPERHEALTH        = (1u << 31)  // 2147483648
} rogue_item_bits_t;
// #define RIT_SUPERHEALTH (1u << 31)



//MED 01/04/97 added hipnotic defines
//===========================================
//hipnotic added defines

typedef enum {
    HIT_MJOLNIR_BIT         = 7,
    HIT_PROXIMITY_GUN_BIT   = 16,
    HIT_LASER_CANNON_BIT    = 23,
    HIT_WETSUIT_BIT         = 25, // (23 + 2)
    HIT_EMPATHY_SHIELDS_BIT = 26  // (23 + 3)
} hipnotic_item_bitpos_t;

typedef enum {
    HIT_PROXIMITY_GUN   = (1u << HIT_PROXIMITY_GUN_BIT),
    HIT_MJOLNIR         = (1u << HIT_MJOLNIR_BIT),
    HIT_LASER_CANNON    = (1u << HIT_LASER_CANNON_BIT),
    HIT_WETSUIT         = (1u << HIT_WETSUIT_BIT),
    HIT_EMPATHY_SHIELDS = (1u << HIT_EMPATHY_SHIELDS_BIT)
} hipnotic_item_bits_t;

extern bool noclip_anglehack;

#ifdef QUAKE2
    #define	GAMENAME	"id1"		/* directory to look in by default */
#else
    #define	GAMENAME	"id1"
#endif

extern bool     standard_quake;
extern bool     rogue;
extern bool     hipnotic;
extern int32_t  Registered;
#ifdef __cplusplus
extern "C" {
#endif

    void GM_GameInit();
    void GM_CheckRegistered();
    void GM_Quit();

#ifdef __cplusplus
}
#endif
