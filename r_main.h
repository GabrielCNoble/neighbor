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

void r_SetZPlanes(float z_near, float z_far);

void r_SetFovY(float fov_y);

void r_RecomputeViewProjectionMatrix();

void r_SetViewMatrix(mat4_t *view_matrix);

void r_GetWindowSize(uint32_t *width, uint32_t *height);

/*
=================================================================
=================================================================
=================================================================
*/

void r_InitBuiltinTextures();

struct r_texture_handle_t r_AllocTexture();

void r_FreeTexture(struct r_texture_handle_t handle);

struct r_texture_handle_t r_LoadTexture(char *file_name, char *texture_name);

struct r_texture_t *r_GetTexturePointer(struct r_texture_handle_t handle);

struct r_texture_handle_t r_GetTextureHandle(char *name);

// void r_SetTexture(struct r_texture_handle_t handle, uint32_t sampler_index);


/*
=================================================================
=================================================================
=================================================================
*/

struct r_material_handle_t r_AllocMaterial();

void r_FreeMaterial(struct r_material_handle_t handle);

struct r_material_t *r_GetMaterialPointer(struct r_material_handle_t handle);

struct r_material_handle_t r_GetMaterialHandle(char *name);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_light_handle_t r_CreateLight(vec3_t *position, float radius, vec3_t *color);

struct r_light_t *r_GetLightPointer(struct r_light_handle_t handle);

void r_DestroyLight(struct r_light_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

void *r_AllocCmdData(uint32_t size);

void *r_AtomicAllocCmdData(uint32_t size);

void r_BeginSubmission(mat4_t *view_projection_matrix, mat4_t *view_matrix);

void r_SubmitDrawCmd(mat4_t *model_matrix, struct r_material_t *material, uint32_t start, uint32_t count);

void r_EndSubmission();

// void r_SortDrawCmds(struct r_udraw_cmd_buffer_t *udraw_cmd_buffer);

void r_QueueCmd(uint32_t type, void *data, uint32_t data_size);

struct r_cmd_t *r_NextCmd();

void r_AdvanceCmd();

int r_ExecuteCmds(void *data);

void r_WaitEmptyQueue();

void r_Draw(struct r_cmd_t *cmd);

/*
=================================================================================
=================================================================================
=================================================================================
*/



#endif