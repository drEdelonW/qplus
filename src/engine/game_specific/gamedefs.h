#pragma once



// stock defines
typedef enum {
    IT_SHOTGUN          = (1 << 0),
    IT_SUPER_SHOTGUN    = (1 << 1),
    IT_NAILGUN          = (1 << 2),
    IT_SUPER_NAILGUN    = (1 << 3),
    IT_GRENADE_LAUNCHER = (1 << 4),
    IT_ROCKET_LAUNCHER  = (1 << 5),
    IT_LIGHTNING        = (1 << 6),
    IT_SUPER_LIGHTNING  = (1 << 7),

    IT_SHELLS           = (1 << 8),
    IT_NAILS            = (1 << 9),
    IT_ROCKETS          = (1 << 10),
    IT_CELLS            = (1 << 11),
    IT_AXE              = (1 << 12),

    IT_ARMOR1           = (1 << 13),
    IT_ARMOR2           = (1 << 14),
    IT_ARMOR3           = (1 << 15),

    IT_SUPERHEALTH      = (1 << 16),
    IT_KEY1             = (1 << 17),
    IT_KEY2             = (1 << 18),

    IT_INVISIBILITY     = (1 << 19),
    IT_INVULNERABILITY  = (1 << 20),
    IT_SUIT             = (1 << 21),
    IT_QUAD             = (1 << 22),

    IT_SIGIL1           = (1 << 28),
    IT_SIGIL2           = (1 << 29),
    IT_SIGIL3           = (1 << 30),
    IT_SIGIL4           = (1 << 31)
} item_bits_t;


//===========================================
//rogue changed and added defines

typedef enum {
    RIT_SHELLS             = (1 << 7),   // 128
    RIT_NAILS              = (1 << 8),   // 256
    RIT_ROCKETS            = (1 << 9),   // 512
    RIT_CELLS              = (1 << 10),  // 1024
    RIT_AXE                = (1 << 11),  // 2048

    RIT_LAVA_NAILGUN       = (1 << 12),  // 4096
    RIT_LAVA_SUPER_NAILGUN = (1 << 13),  // 8192
    RIT_MULTI_GRENADE      = (1 << 14),  // 16384
    RIT_MULTI_ROCKET       = (1 << 15),  // 32768
    RIT_PLASMA_GUN         = (1 << 16),  // 65536

    RIT_ARMOR1             = (1 << 23),  // 8388608
    RIT_ARMOR2             = (1 << 24),  // 16777216
    RIT_ARMOR3             = (1 << 25),  // 33554432

    RIT_LAVA_NAILS         = (1 << 26),  // 67108864
    RIT_PLASMA_AMMO        = (1 << 27),  // 134217728
    RIT_MULTI_ROCKETS      = (1 << 28),  // 268435456
    RIT_SHIELD             = (1 << 29),  // 536870912
    RIT_ANTIGRAV           = (1 << 30),  // 1073741824
    RIT_SUPERHEALTH        = (1u << 31)  // 2147483648
} rogue_item_bits_t;



//MED 01/04/97 added hipnotic defines
//===========================================
//hipnotic added defines

typedef enum {
    HIT_PROXIMITY_GUN_BIT   = 16,
    HIT_MJOLNIR_BIT         = 7,
    HIT_LASER_CANNON_BIT    = 23,
    HIT_WETSUIT_BIT         = 25, // (23 + 2)
    HIT_EMPATHY_SHIELDS_BIT = 26  // (23 + 3)
} hipnotic_item_bitpos_t;

typedef enum {
    HIT_PROXIMITY_GUN   = (1 << HIT_PROXIMITY_GUN_BIT),
    HIT_MJOLNIR         = (1 << HIT_MJOLNIR_BIT),
    HIT_LASER_CANNON    = (1 << HIT_LASER_CANNON_BIT),
    HIT_WETSUIT         = (1 << HIT_WETSUIT_BIT),
    HIT_EMPATHY_SHIELDS = (1 << HIT_EMPATHY_SHIELDS_BIT)
} hipnotic_item_bits_t;


#ifdef QUAKE2
    #define	GAMENAME	"id1"		/* directory to look in by default */
#else
    #define	GAMENAME	"id1"
#endif