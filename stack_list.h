#ifndef STACK_LIST_H
#define STACK_LIST_H

#include <stdint.h>

struct stack_list_t
{
    void **buffers;
    uint32_t buffer_size;
    uint32_t elem_size;
    uint32_t cursor;
    uint32_t size;

    uint32_t *free_stack;
    uint32_t free_stack_top;
};

struct stack_list_t create_stack_list(uint32_t elem_size, uint32_t buffer_size);

void expand_stack_list(struct stack_list_t *stack_list, uint32_t elem_count);

void *get_stack_list_element(struct stack_list_t *stack_list, uint32_t index);

uint32_t add_stack_list_element(struct stack_list_t *stack_list, void *element);

void remove_stack_list_element(struct stack_list_t *stack_list, uint32_t index);

#endif