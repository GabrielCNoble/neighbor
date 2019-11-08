#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h>

struct ringbuffer_t
{
    void *buffer;
    uint32_t elem_size;
    uint32_t buffer_size;
    uint32_t free_slots;
    uint32_t next_in;
    uint32_t next_out;
};


struct ringbuffer_t create_ringbuffer(uint32_t elem_size, uint32_t buffer_size);

uint32_t add_ringbuffer_element(struct ringbuffer_t *ringbuffer, void *element);

void *peek_ringbuffer_element(struct ringbuffer_t *ringbuffer);

void *get_ringbuffer_element(struct ringbuffer_t *ringbuffer);


#endif