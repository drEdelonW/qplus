#pragma once

#include "rEntity_pre.h"
#include "EntityState.h"
#include "qTime.h"
#include "Model_pre.h"
#include "eFrag_pre.h"
#include "Node.h"
#include "qLight.h"
#include "pose.h"
#include "StateHistory.h"

#define MAX_TEMP_ENTITIES       64   /* lightning bolts, etc */
#define MAX_STATIC_ENTITIES     128   /* torches, etc */

// it was [entity_t] on render side
struct r_Entity_s {
    bool    forcelink;      // model changed
    int     update_type;
    EntityState_t baseline; // to fill in defaults in updates
    LegTime_t msgtime;      // time of last update

    pose_t  msgPoses[HistoryDepth]; // last two updates (Cur is newest)
    pose_t  pose;

    Model_p model;          // NULL = no model
    efrag_p efrag;          // linked list of efrags
    int     frame;
    float   syncbase;       // for client-side animations
    ColorMap_p pColorMap;
    EntityEffects_t effects;// light, particals, etc
    int     skinnum;        // for Alias models
    int     visframe;       // last frame this entity was found in an active leaf
    int     dlightframe;    // dynamic lighting
    int     dlightbits;

    // FIXME: could turn these into a union
    int     trivial_accept;
    mNode_p topnode;  // for bmodels, first world node that splits bmodel, or NULL if not split
};
