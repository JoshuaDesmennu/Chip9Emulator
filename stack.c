#include "stack.h"

// get data before you pop of the stack
StackNode* pop(StackNode *stack) {
    StackNode *remaining = stack->next;
    free(stack);
    return remaining;
}

StackNode* push(StackNode *stack, uint16_t data) {
    StackNode *curr = malloc(sizeof(StackNode));
    if (curr == NULL) {
        fprintf(stderr, "Failed to allocate memory on stack\n");
        return stack;
    }
    curr->next = stack;
    curr->data = data;
    return curr;
}

void freeStack(StackNode *stack) {
    StackNode *tmp = stack;
    while (stack != NULL) {
        tmp = tmp->next;
        free(stack);
        stack = tmp;
    }
}

