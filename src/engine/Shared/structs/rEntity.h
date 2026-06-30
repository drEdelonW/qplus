#pragma once

#include "rEntity_pre.h"
#include "EntityState.h"
#include "qTime.h"
#include "Model_pre.h"
#include "eFrag_pre.h"
#include "Node.h"

// it was [entity_t] on render side
struct r_Entity_s {
    bool    forcelink;      // model changed
    int     update_type;
    EntityState_t baseline; // to fill in defaults in updates
    LegTime_t msgtime;// time of last update
    vec3_t  msg_origins[2]; // last two updates(0 is newest)
    vec3_t  origin;
    vec3_t  msg_angles[2];  // last two updates(0 is newest)
    vec3_t  angles;
    Model_p model;          // NULL = no model
    efrag_p efrag;          // linked list of efrags
    int     frame;
    float   syncbase;       // for client-side animations
    uint8_p colormap;
    EntityEffects_t effects;// light, particals, etc
    int     skinnum;        // for Alias models
    int     visframe;       // last frame this entity was found in an active leaf
    int     dlightframe;    // dynamic lighting
    int     dlightbits;

    // FIXME: could turn these into a union
    int     trivial_accept;
    mNode_p topnode;  // for bmodels, first world node that splits bmodel, or NULL if not split
};
