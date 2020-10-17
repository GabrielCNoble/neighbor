#ifndef MDL_H
#define MDL_H

#include "r_defs.h"
#include <stdint.h>

struct mdl_batch_t
{
    struct r_material_t *material;
    uint32_t start;
    uint32_t count;
};
struct mdl_model_t
{
    char* name;
    struct vec3_t* vertices;
    struct mdl_batch_t *batches;
    uint32_t batch_count;
    uint32_t lod_count;
    struct r_chunk_h chunk;
    vec3_t max;
    vec3_t min;
};

struct mdl_model_h
{
    uint32_t model_index;
};

#define MDL_INVALID_LOD_COUNT 0xffffffff
#define MDL_INVALID_MODEL_INDEX 0xffffffff
#define MDL_MODEL_HANDLE(index) (struct mdl_model_h){index}
#define MDL_INVALID_MODEL_HANDLE MDL_MODEL_HANDLE(MDL_INVALID_MODEL_INDEX)

void mdl_Init();

void mdl_Shutdown();

struct mdl_model_h mdl_CreateModel();

void mdl_DestroyModel(struct mdl_model_h handle);

struct mdl_model_h mdl_GetModelHandle(char* name);

struct mdl_model_t *mdl_GetModelPointer(struct mdl_model_h handle);

struct mdl_batch_t *mdl_GetModelLod(struct mdl_model_t *model, uint32_t lod);

struct mdl_model_h mdl_LoadModel(char *file_name, char *model_name);


#endif
