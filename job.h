#ifndef JOB_H
#define JOB_H

#include "SDL/include/SDL2/SDL_mutex.h"
#include <stdint.h>

#define MAX_JOBS 16
#define JOB_DATA_BUFFER_SIZE 64

struct job_data_buffer_t
{
    char data[JOB_DATA_BUFFER_SIZE];
};

struct job_data_t
{
    void *args;
    void (*function)(void *args);
};

struct job_t
{
    SDL_sem *semaphore;
    struct job_data_t data;
};



void job_Init();

int job_JobFunction(void *job_ptr);

void job_DispatchJob(void (*function)(void *args), void *args, uint32_t args_size);



#endif