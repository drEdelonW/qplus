#include "link.h"

// ClearLink is used for new headnodes
void ClearLink(link_p l) {
    l->prev = l->next = l;
}

void RemoveLink(link_p l) {
    l->next->prev = l->prev;
    l->prev->next = l->next;
}

void InsertLinkBefore(link_p l, link_p before) {
    l->next = before;
    l->prev = before->prev;
    l->prev->next = l;
    l->next->prev = l;
}
void InsertLinkAfter(link_p l, link_p after) {
    l->next = after->next;
    l->prev = after;
    l->prev->next = l;
    l->next->prev = l;
}


