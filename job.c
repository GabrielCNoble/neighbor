#include "job.h"
#include "../dstuff/containers/stack_list.h"
#include <stdlib.h>


// struct stack_list_t job_pool;
// struct stack_list_t job_indices;
// struct job_t *job_pool;
// struct job_data_buffer_t *job_data_buffers;

void job_Init()
{
    // struct job_t *job;

    // // job_pool = create_stack_list(sizeof(struct job_t), MAX_JOBS);
    // job_indices = (uint32_t*)create_stack_list(sizeof(uint32_t ), MAX_JOBS);
    // job_pool = (struct job_t*)calloc(sizeof(struct job_t), MAX_JOBS);
    // job_data_buffers = (struct job_data_buffer_t*)calloc(sizeof(struct job_data_buffer_t ), MAX_JOBS);

    // for(uint32_t i = 0; i < MAX_JOBS; i++)
    // {
    //     job = (struct job_t*)get_stack_list_element(&job_pool, i);
    //     job->semaphore = SDL_CreateSemaphore(0);
    // }
}

int job_JobFunction(void *job_ptr)
{
    // struct job_t *job;
    
    // while(1)
    // {
    //    job = (struct job_t *)job_ptr;
    //    SDL_SemWait(job->semaphore);
    //    job->data.function(job->data.args);
    // }
    return 0;
}