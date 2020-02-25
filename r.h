#ifndef R_H
#define R_H

#include "r_common.h"
//#include "r_vk.h"


void r_InitRenderer();

void r_InitVulkan();



/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t properties);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_pipeline_handle_t r_CreatePipeline(struct r_pipeline_description_t* description);

void r_DestroyPipeline(struct r_pipeline_handle_t handle);

void r_BindPipeline(struct r_pipeline_handle_t handle);

struct r_pipeline_t *r_GetPipelinePointer(struct r_pipeline_handle_t handle);


/*
=================================================================
=================================================================
=================================================================
*/

void r_InitBuiltinTextures();

struct r_texture_handle_t r_AllocTexture();

void r_FreeTexture(struct r_texture_handle_t handle);

struct r_texture_handle_t r_CreateTexture(struct r_texture_description_t *description);

struct r_sampler_t *r_TextureSampler(struct r_sampler_params_t *params);

void r_SetImageLayout(VkImage image, uint32_t aspect_mask, uint32_t old_layout, uint32_t new_layout);

void r_SetTextureLayout(struct r_texture_handle_t handle, uint32_t layout);

void r_BlitImageToTexture(struct r_texture_handle_t handle, VkImage image);

void r_LoadTextureData(struct r_texture_handle_t handle, void *data);

struct r_texture_handle_t r_LoadTexture(char *file_name, char *texture_name);

struct r_texture_t *r_GetTexturePointer(struct r_texture_handle_t handle);

struct r_texture_description_t *r_GetTextureDescriptionPointer(struct r_texture_handle_t handle);

struct r_texture_t* r_GetDefaultTexturePointer();

struct r_texture_handle_t r_GetTextureHandle(char *name);


/*
=================================================================
=================================================================
=================================================================
*/

struct r_shader_handle_t r_CreateShader(struct r_shader_description_t *description);

void r_DestroyShader(struct r_shader_handle_t handle);

struct r_shader_t *r_GetShaderPointer(struct r_shader_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_render_pass_handle_t r_CreateRenderPass(struct r_render_pass_description_t *description);

void r_DestroyRenderPass(struct r_render_pass_handle_t handle);

struct r_render_pass_t *r_GetRenderPassPointer(struct r_render_pass_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_framebuffer_handle_t r_CreateFramebuffer(struct r_framebuffer_description_t *description);

void r_DestroyFramebuffer(struct r_framebuffer_handle_t handle);

struct r_framebuffer_t *r_GetFramebufferPointer(struct r_framebuffer_handle_t handle);

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

void r_DrawPoint(vec3_t* position, vec3_t* color);



#endif
