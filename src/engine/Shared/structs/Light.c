#include "Light.h"
#include "client.h"
#include "mem_placement.h"
#include <string.h>

dLight_t    cl_dlights[MAX_DLIGHTS] PLACE_TO_SDRAM;

/*
===============
CL_AllocDlight

===============
*/
dLight_p CL_AllocDlight(int32_t key) {
    // first look for an exact key match
    if (key) {
        for (int i = 0; i < MAX_DLIGHTS; i++)
            if (cl_dlights[i].key == key) {
                cl_dlights[i] = (dLight_t){ // should clear all fields
                     .key = key
                };
                return &cl_dlights[i];
            }
    }

    // then look for anything else
    for (int i = 0; i < MAX_DLIGHTS; i++)
        if (cl_dlights[i].die < cl.time) {
            cl_dlights[i] = (dLight_t){ // should clear all fields
                 .key = key
            };
            return &cl_dlights[i];
        }

    cl_dlights[0] = (dLight_t){ // should clear all fields
        .key = key
    };
    return &cl_dlights[0];
}


/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights() {
    LegDt_t time = (LegDt_t)(cl.time - cl.oldtime);

    for (int i = 0; i < MAX_DLIGHTS; i++) {
        if ((cl_dlights[i].die < cl.time) ||
            (cl_dlights[i].radius == 0.0f)
            )   continue;

        cl_dlights[i].radius -= time * cl_dlights[i].decay;
        if (cl_dlights[i].radius < 0.0f)
            cl_dlights[i].radius = 0.0f;
    }
}