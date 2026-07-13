#pragma once

typedef enum {
    MOVE_NORMAL     = 0u, // normal movement, collide with everything
    MOVE_NOMONSTERS = 1u, // ignore monsters
    MOVE_MISSILE    = 2u  // special missile movement rules
} phymovetype_t;