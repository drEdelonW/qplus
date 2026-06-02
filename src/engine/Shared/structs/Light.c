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
        dLight_p dl = cl_dlights;
        for (int i = 0; i < MAX_DLIGHTS; i++, dl++) {
            if (dl->key == key) {
                memset(dl, 0, sizeof(*dl));
                dl->key = key;
                return dl;
            }
        }
    }

    // then look for anything else
    {
        dLight_p dl = cl_dlights;
        for (int i = 0; i < MAX_DLIGHTS; i++, dl++) {
            if (dl->die < cl.time) {
                memset(dl, 0, sizeof(*dl));
                dl->key = key;
                return dl;
            }
        }
    }

    dLight_p dl = &cl_dlights[0];
    memset(dl, 0, sizeof(*dl));
    dl->key = key;
    return dl;
}
