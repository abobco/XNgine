#include "linked_list.h"

void list_append(ListNode **head, void *data, size_t data_size) {
    ListNode *new_node = (ListNode*) malloc(sizeof(ListNode));
    new_node->data = malloc(data_size);
    new_node->next = NULL;

    // copy each byte of memory
    for ( int i=0; i<data_size; i++ ) 
        *(char *)(new_node->data + i) = *(char *)(data + i); 

    if ( *head == NULL) {
        *head = new_node;
        return;
    }

    ListNode *last = *head;
    while ( last->next != NULL ) {
        last = last->next;
    }

    last->next = new_node;
}

void list_delete(ListNode **head,  ListNode* node) {
    ListNode *t = *head;
    ListNode *prev;

    if ( t != NULL && t == node ) {
        *head = t->next;
        free(t);
        return;
    }

    while ( t != NULL && t != node ) {
        prev = t;
        t = t->next;
    }

    if ( t == NULL ) return; // node wasn't in the list

    prev->next = t->next;

    free(t);
}