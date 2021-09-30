// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#include "SortedList.h"
#include <pthread.h>
#include <string.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
    if (list == NULL || element == NULL)
        return;

    SortedListElement_t* n = list->next;
    // If empty, n = list
    // Find a spot to place element such that the list is sorted,
    // so stop looping when the key of element we want to insert is <= n->key
    while (n != list){
        if (strcmp(element->key, n->key) <= 0)
            break;
        n = n->next;
    }
    if (opt_yield & INSERT_YIELD)
        sched_yield();
    // Also works for empty list
    element->next = n;
    element->prev = n->prev;
    n->prev->next = element;
    n->prev = element;
}

int SortedList_delete(SortedListElement_t *element){
    if (element == NULL)
        return 1;
    // Corrupted prev next pointers
    if (element->next->prev != element->prev->next)
        return 1;
    if (element->next->prev != element || element->prev->next != element)
        return 1;
    
    if (opt_yield & DELETE_YIELD)
        sched_yield();

    // Also works for empty list
    element->next->prev = element->prev;
    element->prev->next = element->next;
    return 0;
    
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
    if (list == NULL || key == NULL)
        return NULL;

    SortedListElement_t* p = list->next;
    while (p != list){
        if (!strcmp(key, p->key)) // strcmp returns 0 if equal
            return p;
        if (opt_yield & LOOKUP_YIELD)
            sched_yield();
        p = p->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list){
    if (list == NULL)
        return -1;
    
    int len = 0;
    SortedListElement_t* p = list->next;
    while (p != list){
        len++;
        if (opt_yield & LOOKUP_YIELD)
            sched_yield();
        p = p->next;
    }
    return len;
}
