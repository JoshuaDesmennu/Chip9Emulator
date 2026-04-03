#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct stacknode {
    uint16_t data;
    struct stacknode *next;
} StackNode;

StackNode* pop(StackNode *stack);
StackNode* push(StackNode *stack, uint16_t data);
void freeStack(StackNode *stack);