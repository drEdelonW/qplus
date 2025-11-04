#pragma once

typedef struct r_Entity_s r_Entity_t;
typedef r_Entity_t* r_Entity_p;

// struct efrag_s;
typedef struct efrag_s efrag_t;
typedef efrag_t* efrag_p;
struct efrag_s {
    // mLeaf_p leaf;
    struct mLeaf_s* leaf;  // TODO: fix include collision issue
    efrag_p     leafnext;
    r_Entity_p  entity;
    efrag_p     entnext;
};