#pragma once
// particle effects
typedef enum {
    RT_ROCKET   = 0u,
    RT_GRENADE  = 1u,
    RT_GIB      = 2u,
    RT_ROTATE   = 3u,   // not used
    RT_TRACER   = 4u,
    RT_ZOMGIB   = 5u,
    RT_TRACER2  = 6u,
    RT_TRACER3  = 7u
} RocketTrailType;

/* Bit flags for entity effects */
typedef enum entity_effects {
    EF_ROCKET  = 1u << RT_ROCKET,   /* leave a trail */
    EF_GRENADE = 1u << RT_GRENADE,  /* leave a trail */
    EF_GIB     = 1u << RT_GIB,      /* leave a trail */
    EF_ROTATE  = 1u << RT_ROTATE,   /* rotate (bonus items) */ // TODO: seems like mistake. investigate it
    EF_TRACER  = 1u << RT_TRACER,   /* green split trail */
    EF_ZOMGIB  = 1u << RT_ZOMGIB,   /* small blood trail */
    EF_TRACER2 = 1u << RT_TRACER2,  /* orange split trail + rotate */
    EF_TRACER3 = 1u << RT_TRACER3   /* purple trail */
} entity_effects;