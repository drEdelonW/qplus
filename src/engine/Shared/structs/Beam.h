#pragma once

#include "types.h"
#include "vector.h"
#include "Model_st.h"
#include "qTime.h"

typedef struct {
    int32_t entity;
    Model_p model;
    LegacyTimeDelta_t   endtime;
    vec3_t  start;
    vec3_t  end;
} Beam_t;
typedef Beam_t* Beam_p;

#define MAX_BEAMS 24
extern Beam_t   cl_beams[MAX_BEAMS];
