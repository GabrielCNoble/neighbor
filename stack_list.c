#include "stack_list.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct stack_list_t create_stack_list(uint32_t elem_size, uint32_t buffer_size)
{
    struct stack_list_t stack_list;
    memset(&stack_list, 0, sizeof(struct stack_list_t));

    stack_list.buffer_size = buffer_size;
    stack_list.elem_size = elem_size;
    stack_list.free_stack_top = 0xffffffff;

    expand_stack_list(&stack_list, 1);

    return stack_list;
}

void expand_stack_list(struct stack_list_t *stack_list, uint32_t elem_count)
{
    void **buffers;
    uint32_t *free_stack;
    uint32_t buffer_count;
    uint32_t list_buffer_count;

    elem_count = (elem_count + stack_list->buffer_size - 1) & (~(stack_list->buffer_size - 1));

    if(elem_count % stack_list->buffer_size)
    {
        elem_count += stack_list->buffer_size - elem_count % stack_list->buffer_size;
    }

    buffer_count = elem_count / stack_list->buffer_size;
    list_buffer_count = stack_list->size / stack_list->buffer_size;

    stack_list->size += elem_count;

    buffers = calloc(stack_list->size, sizeof(void *));
    free_stack = calloc(stack_list->size, sizeof(uint32_t));

    if(stack_list->buffers)
    {
        memcpy(buffers, stack_list->buffers, sizeof(void *) * list_buffer_count);
        free(stack_list->buffers);
        free(stack_list->free_stack);
    }

    for(uint32_t i = 0; i < buffer_count; i++)
    {
        buffers[i + list_buffer_count] = calloc(stack_list->buffer_size, stack_list->elem_size);
    }

    stack_list->buffers = buffers;
    stack_list->free_stack = free_stack;
}

void *get_stack_list_element(struct stack_list_t *stack_list, uint32_t index)
{
    char *buffer;
    void *element = NULL;

    if(index < stack_list->size)
    {
        buffer = stack_list->buffers[index / stack_list->buffer_size];
        element = buffer + (index % stack_list->buffer_size) * stack_list->elem_size;
    }

    return element;
}

uint32_t add_stack_list_element(struct stack_list_t *stack_list, void *element)
{
    uint32_t index = 0xffffffff;
    char *buffer;

    if(stack_list->free_stack_top < 0xffffffff)
    {
        index = stack_list->free_stack[stack_list->free_stack_top];
        stack_list->free_stack_top--;
    }
    else
    {
        index = stack_list->cursor++;

        if(index >= stack_list->size)
        {
            /* this will add an extra buffer... */
            expand_stack_list(stack_list, 1);
        }
    }

    if(element && index < 0xffffffff)
    {
        buffer = stack_list->buffers[index / stack_list->buffer_size];
        memcpy(buffer + (index % stack_list->buffer_size) * stack_list->elem_size, element, stack_list->elem_size);
    }

    return index;
}

void remove_stack_list_element(struct stack_list_t *stack_list, uint32_t index)
{
    if(index < stack_list->cursor)
    {
        stack_list->free_stack_top++;
        stack_list->free_stack[stack_list->free_stack_top] = index;
    }
}