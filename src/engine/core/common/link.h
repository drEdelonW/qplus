#pragma once
#include <stddef.h>
#include "types.h"
//============================================================================

struct link_s;
typedef struct link_s link_t;
typedef link_t* link_p;
struct link_s {
    link_p prev, next;
};


void ClearLink(link_p l);
void RemoveLink(link_p l);
void InsertLinkBefore(link_p l, link_p before);
void InsertLinkAfter(link_p l, link_p after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
// #define STRUCT_FROM_LINK(l, t ,m) ((t *)((uint8_p)l - (int32_t) & (((t *)0)->m)))

#define STRUCT_FROM_LINK(ptr, type, member)   ((type *)((uint8_p)(ptr) - offsetof(type, member)))
// #define container_of(ptr, type, member)   ((type *)((uint8_p)(ptr) - offsetof(type, member)))


