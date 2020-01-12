#ifndef MDL_H
#define MDL_H

#include "r_common.h"

struct model_batch_t
{
    struct r_material_t *material;
    struct r_draw_range_t range;
};
struct model_t
{
    char* name;
    struct model_batch_t* batches;
    uint32_t batch_count;
    uint32_t lod_count;
    struct r_alloc_handle_t alloc; 
    vec3_t max;
    vec3_t min;
};

struct model_handle_t
{
    uint32_t model_index;
};

#define MDL_INVALID_LOD_COUNT 0xffffffff
#define MDL_INVALID_MODEL_INDEX 0xffffffff
#define MDL_MODEL_HANDLE(index) (struct model_handle_t){index}
#define MDL_INVALID_MODEL_HANDLE MDL_MODEL_HANDLE(MDL_INVALID_MODEL_INDEX)

void mdl_Init();

void mdl_Shutdown();

struct model_handle_t mdl_AllocModel();

struct model_handle_t mdl_GetModelHandle(char* name);

struct model_t *mdl_GetModelPointer(struct model_handle_t handle);

struct model_batch_t *mdl_GetModelLod(struct model_t *model, uint32_t lod);

void mdl_FreeModel(struct model_handle_t handle);

struct model_handle_t mdl_LoadModel(char *file_name);


#endif