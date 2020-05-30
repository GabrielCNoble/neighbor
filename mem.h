#ifndef MEM_H
#define MEM_H

#include <stdint.h>

#define MEM_GUARD_POINTERS 24

struct mem_header_t
{
    void *guard[MEM_GUARD_POINTERS];
    size_t size;
    uint32_t line;
    char file[256];
    struct mem_header_t *next;
    struct mem_header_t *prev;
};

struct mem_tail_t
{
    void *guard[MEM_GUARD_POINTERS];
};

#define INSTRUMENT_MEMORY

struct mem_header_t *mem_GetAllocHeader(void *memory);

struct mem_tail_t *mem_GetAllocTail(void *memory);

void *mem_InitHeaderAndTail(void *memory, uint32_t size, uint32_t line, char *file);

void mem_CheckGuardImp(void *memory);

void mem_CheckGuardsImp();

void *mem_MallocImp(size_t size, uint32_t line, char *file);

void *mem_CallocImp(size_t num, size_t size, uint32_t line, char *file);

void mem_FreeImp(void *memory, uint32_t line, char *file);


#ifdef INSTRUMENT_MEMORY

#define mem_CheckGuard(memory) mem_CheckGuardImp(memory)

#define mem_CheckGuards mem_CheckGuardsImp

#define mem_Malloc(size) mem_MallocImp(size, __LINE__, __FILE__)

#define mem_Calloc(num, count) mem_CallocImp(num, count, __LINE__, __FILE__)

#define mem_Free(memory) mem_FreeImp(memory, __LINE__, __FILE__)

#else

#define mem_CheckGuard(memory)

#define mem_CheckGuards

#define mem_Malloc(size) malloc(size)

#define mem_Calloc(num, count) calloc(num, count)

#define mem_Free(memory) free(memory)

#endif

#ifdef DSTUFF_MEMORY_MEMORY_IMPLEMENTATION


#endif

#endif

