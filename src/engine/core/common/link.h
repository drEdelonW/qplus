#pragma once
#include <stddef.h>
//============================================================================

typedef struct link_s{
	struct link_s	*prev, *next;
} link_t;


void ClearLink(link_t *l);
void RemoveLink(link_t *l);
void InsertLinkBefore(link_t *l, link_t *before);
void InsertLinkAfter( link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
// #define	STRUCT_FROM_LINK(l, t ,m) ((t *)((uint8_t *)l - (int) & (((t *)0)->m)))

#define STRUCT_FROM_LINK(ptr, type, member)   ((type *)((uint8_t *)(ptr) - offsetof(type, member)))
// #define container_of(ptr, type, member)   ((type *)((uint8_t *)(ptr) - offsetof(type, member)))


