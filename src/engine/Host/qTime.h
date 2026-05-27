#pragma once
#include "types.h"

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