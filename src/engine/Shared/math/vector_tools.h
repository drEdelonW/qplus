#pragma once

#include "vector.h"
#include "angle.h"
#include "msg.h"

static inline vec3_t MSG_ReadVector() {
    return (vec3_t){
        .x = MSG_ReadCoord(),
        .y = MSG_ReadCoord(),
        .z = MSG_ReadCoord()
    };
}

static inline ang3_t MSG_ReadAngles() {
    return (ang3_t){
        .pitch  = MSG_ReadAngle(),
        .yaw    = MSG_ReadAngle(),
        .roll   = MSG_ReadAngle()
    };
}

