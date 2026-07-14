#pragma once
//============================================================================

typedef struct link_s link_t;
typedef link_t* link_p;
struct link_s {
    link_p prev;
    link_p next;
};


void ClearLink(link_p link);
void RemoveLink(link_p link);
void InsertLinkBefore(link_p link, link_p before);
void InsertLinkAfter(link_p link, link_p after);

#define STRUCT_FROM_LINK(ptr, type, member)   ((type *)((uint8_p)(ptr) - offsetof(type, member)))

