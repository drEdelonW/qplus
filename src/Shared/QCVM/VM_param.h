#pragma once

#define MAX_PARMS (8)

// VM global offsets; vectors occupy 3 float slots
typedef enum {
    OFS_NULL      = 0u,

    OFS_RETURN    = 1u,     // +3
    OFS_RETURN_X  = 1u,
    OFS_RETURN_Y  = 2u,
    OFS_RETURN_Z  = 3u,
    PARM_STRIDE   = 3,

    OFS_PARM0     = 4u,     // parm0..parm7: +3(PARM_STRIDE) per parm 
    OFS_PARM1     = 7u,
    OFS_PARM2     = 10u,
    OFS_PARM3     = 13u,
    OFS_PARM4     = 16u,
    OFS_PARM5     = 19u,
    OFS_PARM6     = 22u,
    OFS_PARM7     = 25u,
    RESERVED_OFS  = 28u
} PrOfs_e;
