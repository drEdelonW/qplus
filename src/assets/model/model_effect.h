#pragma once

typedef enum {
    RT_ROCKET   = 0,
    RT_GRENADE  = 1,
    RT_GIB      = 2,
    RT_TRACER   = 3,
    RT_ZOMGIB   = 4,
    RT_TRACER2  = 5,
    RT_TRACER3  = 6
} rocket_trail_type;

/* Bit flags for entity effects */
typedef enum entity_effects {
    EF_ROCKET  = 1 << RT_ROCKET,   /* leave a trail */
    EF_GRENADE = 1 << RT_GRENADE,   /* leave a trail */
    EF_GIB     = 1 << RT_GIB,   /* leave a trail */
    EF_ROTATE  = 1 << RT_TRACER,   /* rotate (bonus items) */
    EF_TRACER  = 1 << RT_ZOMGIB,   /* green split trail */
    EF_ZOMGIB  = 1 << RT_TRACER2,   /* small blood trail */
    EF_TRACER2 = 1 << RT_TRACER3,   /* orange split trail + rotate */
    EF_TRACER3 = 1 << 7    /* purple trail */
} entity_effects;