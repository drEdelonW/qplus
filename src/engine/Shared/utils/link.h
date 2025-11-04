#pragma once
//============================================================================

struct link_s;
typedef struct link_s link_t;
typedef link_t* link_p;
struct link_s {
    link_p prev, next;
};


void ClearLink(link_p link);
void RemoveLink(link_p link);
void InsertLinkBefore(link_p link, link_p before);
void InsertLinkAfter(link_p link, link_p after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,Entity_t,order)
// FIXME: remove this mess!
// #define STRUCT_FROM_LINK(link, t ,m) ((t *)((uint8_p)link - (int32_t) & (((t *)0)->m)))

#define STRUCT_FROM_LINK(ptr, type, member)   ((type *)((uint8_p)(ptr) - offsetof(type, member)))
// #define container_of(ptr, type, member)   ((type *)((uint8_p)(ptr) - offsetof(type, member)))


