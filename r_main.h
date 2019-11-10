#ifndef R_MAIN_H
#define R_MAIN_H

#include "r_common.h"
#include "r_vk.h"


void r_InitRenderer();

/*
=================================================================
=================================================================
=================================================================
*/

struct r_alloc_handle_t r_Alloc(uint32_t size, uint32_t align, uint32_t index_alloc);

struct r_alloc_t *r_GetAllocPointer(struct r_alloc_handle_t handle);

void r_Free(struct r_alloc_handle_t handle);

void r_Memcpy(struct r_alloc_handle_t handle, void *data, uint32_t size);

/*
=================================================================
=================================================================
=================================================================
*/

// void r_SetProjectionMatrix(mat4_t *projection_matrix);

// void r_SetViewMatrix(mat4_t *view_matrix);

// void r_SetModelMatrix(mat4_t *model_matrix);

// void r_UpdateMatrices();

void r_SetViewProjectionMatrix(mat4_t *view_matrix, mat4_t *projection_matrix);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_texture_handle_t r_AllocTexture();

void r_FreeTexture(struct r_texture_handle_t handle);

struct r_texture_handle_t r_LoadTexture(char *file_name);

struct r_texture_t *r_GetTexturePointer(struct r_texture_handle_t handle);

void r_SetTexture(struct r_texture_handle_t handle, uint32_t sampler_index);


/*
=================================================================
=================================================================
=================================================================
*/

void *r_AllocCmdData(uint32_t size);

void r_QueueDrawCmd(mat4_t *model_matrix, struct r_material_handle_t material, struct r_alloc_handle_t alloc);

void r_QueueCmd(uint32_t type, void *data, uint32_t data_size);

struct r_cmd_t *r_NextCmd();

void r_AdvanceCmd();

void r_ExecuteCmds();

/*
=================================================================================
=================================================================================
=================================================================================
*/



#endif