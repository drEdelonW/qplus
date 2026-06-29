#pragma once
#include "types.h"

#if 1 /* TODO: swith to other type names */
/*
 * QTime - Time Domain Management
 * * Domains:
 * - Legacy: Compatibility layer for non-refactored code.
 * - RealWorld: Absolute system time (OS, Files, Network).
 * - SimWorld: Authoritative game logic progression (Physics, State).
 * - ViewRender: Visualization, interpolation, and animation.
 */

 // Legacy Time Domain
typedef double  LegacyTimeStamp_t;
typedef float   LegacyTimeDelta_t;

// RealWorld Time Domain
#if 0
typedef int64_t RealWorldTimeStamp_t;
typedef int64_t RealWorldTimeDelta_t;
#else
typedef LegacyTimeStamp_t RealWorldTimeStamp_t;
typedef LegacyTimeDelta_t RealWorldTimeDelta_t;
#endif

// SimWorld Time Domain
#if 0
typedef int32_t SimWorldTimeStamp_t;
typedef int32_t SimWorldTimeDelta_t;
#else
typedef LegacyTimeStamp_t SimWorldTimeStamp_t;
typedef LegacyTimeDelta_t SimWorldTimeDelta_t;
#endif

// ViewRender Time Domain
#if 0
typedef float   ViewRenderTimeStamp_t;
typedef float   ViewRenderTimeDelta_t;
#else
typedef LegacyTimeStamp_t   ViewRenderTimeStamp_t;
typedef LegacyTimeDelta_t   ViewRenderTimeDelta_t;
#endif

#else
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
typedef double  LegTime_t;
typedef float   LegDt_t;

/* Real - wall-clock */
#if 0
typedef int64_t RealTime_t;
typedef int64_t RealDt_t;
#else
typedef LegTime_t  RealTime_t;
typedef LegDt_t    RealDt_t;
#endif

/* Sim - game logic */
#if 0
typedef int32_t SimTime_t;
typedef int32_t SimDt_t;
#else
typedef LegTime_t  SimTime_t;
typedef LegDt_t    SimDt_t;
#endif

/* View - render/animation */
#if 0
typedef float   ViewTime_t;
typedef float   ViewDt_t;
#else
typedef LegTime_t  ViewTime_t;
typedef LegDt_t    ViewDt_t;
#endif

#endif