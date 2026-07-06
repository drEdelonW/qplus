#pragma once

#include "msg.h"
#include "vector.h"

static inline vec3_t MSG_ReadVector() {
    return (vec3_t){
        .x = MSG_ReadCoord(),
        .y = MSG_ReadCoord(),
        .z = MSG_ReadCoord()
    };
}
static inline vec3_t MSG_ReadMoveVec() {
    return (vec3_t){
        .x = (vec_t)MSG_ReadShort(),
        .y = (vec_t)MSG_ReadShort(),
        .z = (vec_t)MSG_ReadShort()
    };
}
static inline vec3_t MSG_ReadVecCoarse() {
    return (vec3_t){
        .x = fixed4_tof(MSG_ReadChar()),
        .y = fixed4_tof(MSG_ReadChar()),
        .z = fixed4_tof(MSG_ReadChar()),
    };
}


static inline void MSG_WriteVector(sizebuf_p msg, vec3_t v) {
    MSG_WriteCoord(msg, v.x);
    MSG_WriteCoord(msg, v.y);
    MSG_WriteCoord(msg, v.z);
}
static inline void MSG_WriteMoveVec(sizebuf_p msg, vec3_t v) {
    MSG_WriteShort(msg, (int16_t)v.x);
    MSG_WriteShort(msg, (int16_t)v.y);
    MSG_WriteShort(msg, (int16_t)v.z);
}
static inline void MSG_WriteVecCoarse(sizebuf_t *msg, vec3_t v) {
    MSG_WriteChar(msg, fixed4_fsat(v.x));
    MSG_WriteChar(msg, fixed4_fsat(v.y));
    MSG_WriteChar(msg, fixed4_fsat(v.z));
}

#include "endian_tools.h"
static inline vec3_t LittleVector(vec3_t v) {
    return (vec3_t) {
        .x = LittleFloat(v.x),
        .y = LittleFloat(v.y),
        .z = LittleFloat(v.z)
    };
}

#include "angle.h"

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
