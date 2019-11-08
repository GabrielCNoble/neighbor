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

void r_Free(struct r_alloc_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

void r_SetProjectionMatrix(mat4_t *projection_matrix);

void r_SetViewMatrix(mat4_t *view_matrix);

void r_SetModelMatrix(mat4_t *model_matrix);

void r_UpdateMatrices();

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

void r_QueueDrawCmd(struct r_cmd_t *draw_cmd);

struct r_cmd_t *r_NextCmd();

void r_AdvanceCmd();

void r_ExecuteDrawCmd(struct r_cmd_t *draw_cmd);

/*
=================================================================================
=================================================================================
=================================================================================
*/



#endif