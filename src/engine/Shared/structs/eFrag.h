#pragma once

#include "eFrag_pre.h"
#include "rEntity_pre.h"
#include "Leaf_pre.h"
struct efrag_s {
    mLeaf_p     leaf;
    // struct mLeaf_s* leaf;  // TODO: fix include collision issue
    efrag_p     leafnext;
    r_Entity_p  entity;
    efrag_p     entnext;
};