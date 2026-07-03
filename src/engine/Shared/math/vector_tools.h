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

static inline void MSG_WriteVector(sizebuf_p msg, vec3_t v) {
    MSG_WriteCoord(msg, v.x);
    MSG_WriteCoord(msg, v.y);
    MSG_WriteCoord(msg, v.z);
}


static inline ang3_t MSG_ReadAngles() {
    return (ang3_t){
        .pitch  = MSG_ReadAngle(),
        .yaw    = MSG_ReadAngle(),
        .roll   = MSG_ReadAngle()
    };
}

static inline void MSG_WriteAngles(sizebuf_p msg, ang3_t a) {
    MSG_WriteAngle(msg, a.pitch);
    MSG_WriteAngle(msg, a.yaw);
    MSG_WriteAngle(msg, a.roll);
}
