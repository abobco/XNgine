#include <stdio.h>
#include <stdlib.h>

typedef struct ListNode {
    void *data;
    struct ListNode *next;
} ListNode;

void list_append(ListNode **head, void *data, size_t data_size);

void list_delete(ListNode **head,  ListNode* node);