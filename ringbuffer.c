#include "ringbuffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ringbuffer_t create_ringbuffer(uint32_t elem_size, uint32_t buffer_size)
{
    struct ringbuffer_t ringbuffer;
    memset(&ringbuffer, 0, sizeof(struct ringbuffer_t));

    ringbuffer.buffer_size = buffer_size;
    ringbuffer.elem_size = elem_size;
    ringbuffer.free_slots = buffer_size;
    ringbuffer.buffer = calloc(elem_size, buffer_size);

    return ringbuffer;
}

uint32_t add_ringbuffer_element(struct ringbuffer_t *ringbuffer, void *element)
{
    uint32_t index = 0xffffffff;

    if(ringbuffer->free_slots)
    {
        index = ringbuffer->next_in;

        if(element)
        {
            memcpy((char *)ringbuffer->buffer + index * ringbuffer->elem_size, element, ringbuffer->elem_size);
        }

        ringbuffer->next_in = (ringbuffer->next_in + 1) % ringbuffer->buffer_size;
        ringbuffer->free_slots--; 
    }

    return index;
}

void *peek_ringbuffer_element(struct ringbuffer_t *ringbuffer)
{
    return (char *)ringbuffer->buffer + ringbuffer->next_out * ringbuffer->elem_size;
}

void *get_ringbuffer_element(struct ringbuffer_t *ringbuffer)
{
    void *element = NULL;

    if(ringbuffer->free_slots < ringbuffer->buffer_size)
    {
        element = (char *)ringbuffer->buffer + ringbuffer->next_out * ringbuffer->elem_size;
        ringbuffer->next_out = (ringbuffer->next_out + 1) % ringbuffer->buffer_size;
        ringbuffer->free_slots++;
    }

    return element;
}
