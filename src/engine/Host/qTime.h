#pragma once
#include "types.h"

/*
 * QTime - time domain types
 *
 * Domains:
 *   Real  - wall-clock; OS, files, network
 *   Sim   - authoritative game logic: physics, state
 *   View  - interpolation, animation, render
 *
 * *Time_t  - absolute timestamp
 * *Dt_t    - time delta
 *
 * All domains currently aliased to legacy float/double pending migration.
 */

 /* legacy backing types */
typedef double      LegTime_t;
typedef float       LegDt_t;
typedef LegTime_t*  LegTime_p;
typedef LegDt_t*    LegDt_p;

/* Real - wall-clock */
#if 0
typedef int64_t     RealTime_t;
typedef int64_t     RealDt_t;
#else
typedef LegTime_t   RealTime_t;
typedef LegDt_t     RealDt_t;
#endif
typedef RealTime_t* RealTime_p;
typedef RealDt_t*   RealDt_p;

/* Sim - game logic */
#if 0
typedef int32_t     SimTime_t;
typedef int32_t     SimDt_t;
#else
typedef LegTime_t   SimTime_t;
typedef LegDt_t     SimDt_t;
#endif
typedef SimTime_t*  SimTime_p;
typedef SimDt_t*    SimDt_p;

/* View - render/animation */
#if 0
typedef float       ViewTime_t;
typedef float       ViewDt_t;
#else
typedef LegTime_t   ViewTime_t;
typedef LegDt_t     ViewDt_t;
#endif
typedef ViewTime_t* ViewTime_p;
typedef ViewDt_t*   ViewDt_p;