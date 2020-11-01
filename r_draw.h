#ifndef R_DRAW_H
#define R_DRAW_H

#include "lib/dstuff/ds_matrix.h"
#include "lib/dstuff/ds_list.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_ringbuffer.h"
#include "lib/dstuff/ds_alloc.h"

#include "r_defs.h"
#include "r_nvkl.h"
#include "r.h"



struct r_begin_submission_info_t
{
    mat4_t inv_view_matrix;
    mat4_t projection_matrix;
    struct r_framebuffer_h framebuffer;
    uint32_t clear_framebuffer;
    VkViewport viewport;
    VkRect2D scissor;
};

struct r_submission_state_t
{
    struct stack_list_t draw_cmd_lists;
    struct ringbuffer_t ready_draw_cmd_lists;
    struct ds_heap_t immediate_draw_data_heap;
    uint8_t *immediate_draw_data_buffer;
    uint32_t index_cursor;
    uint32_t vertex_cursor;
};

enum R_DRAW_CMD_LIST_STATE
{
    R_DRAW_CMD_LIST_STATE_READY = 0,
    R_DRAW_CMD_LIST_STATE_PENDING,
    R_DRAW_CMD_LIST_STATE_FREE
};

struct r_draw_cmd_list_t
{
    struct r_begin_submission_info_t begin_info;
    mat4_t view_projection_matrix;
    struct list_t draw_cmds;
    
    struct list_t immediate_draw_cmds;
    
    /* portals that are "seen" by the view configuration specified in this list. */
    struct list_t portal_handles;
    uint32_t state;
};

struct r_draw_cmd_list_h
{
    uint32_t index;
};

struct r_draw_cmd_t
{
    uint32_t start;
    uint32_t count;
    uint32_t vertex_offset;
    mat4_t transform;
    struct r_material_t *material;
};

struct r_i_draw_cmd_t
{
    uint32_t type;
    struct ds_chunk_h data;
};

#define R_I_DRAW_DATA_SLOT_SIZE 64

struct r_i_draw_data_slot_t
{
    uint8_t data[R_I_DRAW_DATA_SLOT_SIZE];
};

struct r_i_upload_buffer_data_t
{
    uint32_t start;
    uint32_t count;
    uint32_t size;
    uint32_t index_data;
    void *data;
};

struct r_texture_state_binding_t
{
    uint32_t binding_index;
    struct r_texture_t *texture;
};

struct r_texture_state_t
{
    uint32_t texture_count;
    struct r_texture_state_binding_t *textures;
};

struct r_draw_state_t
{
    float point_size;
    float line_width;
    VkRect2D scissor;
    
    struct r_pipeline_state_t *pipeline_state;
    struct r_texture_state_t *texture_state;
};




//struct r_i_set_draw_state_data_t
//{
//    struct r_i_draw_state_t draw_state;
//};

struct r_i_submission_state_t
{
    struct list_t cmd_data;
    uint32_t current_pipeline;
    uint32_t vertex_cursor;
    uint32_t index_cursor;
};

enum R_I_DRAW_CMDS
{
    R_I_DRAW_CMD_DRAW,
    R_I_DRAW_CMD_DRAW_LAST,
    R_I_DRAW_CMD_SET_DRAW_STATE,
    R_I_DRAW_CMD_UPLOAD_DATA,
    R_I_DRAW_CMD_FLUSH,
    R_I_DRAW_CMD_LAST,
};

//struct r_i_draw_cmd_t
//{
//    uint32_t type;
//    void *data;
//};

struct r_i_draw_cmd_data_t
{
    mat4_t model_matrix;
    uint32_t index_start;
    uint32_t vertex_start;
    uint32_t count;
    uint32_t indexed;
};

#define R_MAX_SPRITE_LAYER 4

struct r_draw_uniform_data_t
{
    /* data that's written to the uniform buffer */
    mat4_t model_view_projection_matrix;
    vec4_t offset_size;
};

struct r_uniform_buffer_t
{
    struct r_buffer_h buffer;
    VkBuffer vk_buffer;
    void *memory;
    VkEvent event;
};

//struct r_draw_call_data_t
//{
//    uint32_t first;
//    uint32_t count;
//    uint32_t first_instance;
//    uint32_t instance_count;
//    struct r_texture_t *texture;
//};




/*
=================================================================
=================================================================
=================================================================
*/


void r_DrawInit();

void r_DrawInitImmediate();

void r_DrawShutdown();

void r_BeginFrame();

void r_EndFrame();

void r_GetWindowSize(vec2_t *size);

void r_RecomputeInvViewMatrix(struct r_view_t *view);

void r_RecomputeProjectionMatrix(struct r_view_t *view);

struct r_view_t *r_GetViewPointer();

void r_SetWindowSize(uint32_t width, uint32_t height);

void r_Fullscreen(uint32_t enable);

struct r_framebuffer_h r_CreateDrawableFramebuffer(uint32_t width, uint32_t height);

struct r_framebuffer_h r_GetBackbufferHandle();



/*
=================================================================
=================================================================
=================================================================
*/

//void r_BeginDrawCmdSubmission(struct r_submission_state_t *submission_state, struct r_begin_submission_info_t *begin_info);
//
//uint32_t r_EndDrawCmdSubmission(struct r_submission_state_t *submission_state);




//struct r_draw_cmd_list_t *r_GetDrawCmdListPointer(struct r_draw_cmd_list_h handle);
//
//struct r_draw_cmd_list_h r_BeginDrawCmdList(struct r_begin_submission_info_t *begin_info);

//void r_EndDrawCmdList(struct r_draw_cmd_list_h handle);

//void r_ResetDrawCmdList(struct r_draw_cmd_list_h handle);

void r_BeginCommandBuffer(union r_command_buffer_h command_buffer, struct r_view_t *view);

void r_EndCommandBuffer(union r_command_buffer_h command_buffer);

void r_BeginRenderPass(union r_command_buffer_h command_buffer, VkRect2D *render_area, struct r_framebuffer_h framebuffer);

void r_EndRenderPass(union r_command_buffer_h command_buffer);

void r_SubmitCommandBuffers(uint32_t command_buffer_count, union r_command_buffer_h *command_buffers, struct r_fence_h fence);

void r_ClearAttachments(union r_command_buffer_h command_buffer, uint32_t clear_color, uint32_t clear_depth);

void r_Draw(union r_command_buffer_h command_buffer, uint32_t start, uint32_t count);

void r_DrawIndexed(union r_command_buffer_h command_buffer, uint32_t start, uint32_t count, uint32_t vertex_offset);

//void r_BindTexture(union r_command_buffer_h command_buffer, uint32_t index, struct r_texture_h texture);

void r_PushConstants(union r_command_buffer_h command_buffer, VkShaderStageFlagBits stage, uint32_t size, uint32_t offset, void *data);

void r_ImageBarrier(union r_command_buffer_h command_buffer, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

uint32_t r_FillTempVertices(void *data, uint32_t size, uint32_t count);

uint32_t r_FillTempIndices(void *data, uint32_t size, uint32_t count);

void r_SetDrawState(union r_command_buffer_h command_buffer, struct r_draw_state_t *draw_state);

//void r_DispatchPending();

//struct r_uniform_buffer_t *r_AllocateUniformBuffer(union r_command_buffer_h command_buffer);

/*
=================================================================
=================================================================
=================================================================
*/


//void r_i_BeginSubmission(struct r_begin_submission_info_t *begin_info);
//
//void r_i_EndSubmission();
//
//void r_i_DispatchPending();

//struct ds_chunk_h r_i_AllocateDrawCmdData(uint32_t size);

//void r_i_SubmitCmd(struct r_draw_cmd_list_h handle, uint32_t type, struct ds_chunk_h data);

//uint32_t r_i_UploadVertices(struct r_draw_cmd_list_h handle, struct r_i_vertex_t *vertices, uint32_t vert_count);

//uint32_t r_i_UploadIndices(struct r_draw_cmd_list_h handle, uint32_t *indices, uint32_t count);

//uint32_t r_i_UploadBufferData(struct r_draw_cmd_list_h handle, void *data, uint32_t size, uint32_t count, uint32_t index_data);

//void r_i_Draw(struct r_draw_cmd_list_h handle, uint32_t vertex_start, uint32_t index_start, uint32_t count, uint32_t indexed, mat4_t *transform);

//void r_i_DrawImmediate(struct r_draw_cmd_list_h handle, struct r_i_vertex_t *vertices, uint32_t vertex_count, uint32_t *indices, uint32_t index_count, mat4_t *transform);



/*
=================================================================
=================================================================
=================================================================
*/

void r_DrawVisibleEntities(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list);

void r_DrawVisibleLights(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list);

void r_DrawVisiblePortals(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list);

void r_DrawVisibleWorld(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list);

void r_DrawVisible(struct r_view_t *view, struct r_framebuffer_h framebuffer);



#endif // R_DRAW_H
