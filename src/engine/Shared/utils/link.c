#include "link.h"

// ClearLink is used for new headnodes
void ClearLink(link_p link) {
    link->prev = link->next = link;
}

void RemoveLink(link_p link) {
    link->next->prev = link->prev;
    link->prev->next = link->next;
}

void InsertLinkBefore(link_p link, link_p before) {
    link->next = before;
    link->prev = before->prev;
    link->prev->next = link;
    link->next->prev = link;
}
void InsertLinkAfter(link_p link, link_p after) {
    link->next = after->next;
    link->prev = after;
    link->prev->next = link;
    link->next->prev = link;
}


