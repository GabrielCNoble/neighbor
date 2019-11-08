#ifndef LIST_H
#define LIST_H

#include <stdint.h>

struct list_t
{
    void **buffers;
    uint32_t buffer_size;
    uint32_t elem_size;
    uint32_t cursor;
    uint32_t size;
};


struct list_t create_list(uint32_t elem_size, uint32_t buffer_size);

void destroy_list(struct list_t *list);

void expand_list(struct list_t *list, uint32_t elem_count);

void *get_list_element(struct list_t *list, uint32_t index);

uint32_t add_list_element(struct list_t *list, void *element);

void remove_list_element(struct list_t *list, uint32_t index);



#endif