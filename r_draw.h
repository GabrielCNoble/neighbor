#ifndef R_DRAW_H
#define R_DRAW_H

#include "r.h"
#include "lib/dstuff/ds_matrix.h"
#include "lib/dstuff/ds_list.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_ringbuffer.h"
#include "spr.h"

struct r_submission_state_t
{
    struct stack_list_t draw_cmd_lists;
    struct list_t pending_draw_cmd_lists;
    uint32_t current_draw_cmd_list;
    uint32_t draw_cmd_size;
};

struct r_m_submission_state_t
{
    struct r_submission_state_t base;
    struct list_t draw_calls;
};

struct r_i_submission_state_t
{
    struct r_submission_state_t base;
    struct list_t cmd_data;
    uint32_t current_pipeline;
};

struct r_view_t
{
    mat4_t projection_matrix;
    mat4_t inv_view_matrix;
    VkViewport viewport;
    VkRect2D scissor;
    vec2_t position;
//    uint32_t width;
//    uint32_t height;
    float z_near;
    float z_far;
    float zoom;
};

struct r_i_vertex_t
{
    vec4_t position;
    vec4_t tex_coords;
    vec4_t color;
};

struct r_vertex_t
{
    vec4_t position;
    vec4_t normal;
    vec4_t tex_coords;
};

#define R_I_DRAW_DATA_SLOT_SIZE 64

struct r_i_draw_data_slot_t
{
    uint8_t data[R_I_DRAW_DATA_SLOT_SIZE];
};

struct r_i_draw_data_t
{
    uint32_t indexed;
};

struct r_i_draw_line_data_t
{
    float size;
    uint32_t vert_count;
    struct r_i_vertex_t *verts;
};

struct r_i_draw_tri_data_t
{
    uint32_t indice_count;
    uint32_t *indices;
    uint32_t vert_count;
    struct r_i_vertex_t *verts;
};

struct r_i_set_pipeline_data_t
{
    uint32_t pipeline_index;
};

enum R_I_DRAW_CMDS
{
    R_I_DRAW_CMD_DRAW_LINE_LIST,
    R_I_DRAW_CMD_DRAW_LINE_STRIP,
    R_I_DRAW_CMD_DRAW_TRI_LINE,
    R_I_DRAW_CMD_DRAW_TRI_FILL,
    R_I_DRAW_CMD_DRAW_TRI_TEX,
    R_I_DRAW_CMD_SET_PIPELINE,
    R_I_DRAW_CMD_LAST,
};

enum R_I_PIPELINES
{
    R_I_PIPELINE_LINE_LIST,
    R_I_PIPELINE_LINE_STRIP,
    R_I_PIPELINE_TRI_LINE,
    R_I_PIPELINE_TRI_FILL,
    R_I_PIPELINE_TRI_TEX,
    R_I_PIPELINE_LAST,
};

struct r_i_draw_cmd_t
{
    uint32_t type;
    struct r_i_draw_data_t *data;
};

#define R_MAX_SPRITE_LAYER 4

struct r_draw_cmd_t
{
    vec2_t position;
    struct spr_sprite_h sprite;
    float scale;
    float rotation;
    uint32_t layer;
    uint32_t frame;
    uint32_t flipped;
};

struct r_draw_cmd_list_t
{
    mat4_t view_projection_matrix;
    mat4_t inv_view_matrix;
    struct list_t draw_cmds;
};

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

struct r_draw_call_data_t
{
    uint32_t first;
    uint32_t count;
    uint32_t first_instance;
    uint32_t instance_count;
    struct r_texture_handle_t texture;
};

void r_DrawInit();

void r_DrawInitImmediate();

void r_DrawShutdown();

void r_BeginFrame();

void r_EndFrame();

//void r_SetViewMatrix(mat4_t *view_matrix);

//void r_SetProjectionMatrix(mat4_t *projection_matrix);

void r_SetViewPosition(vec2_t *position);

void r_TranslateView(vec2_t *translation);

void r_SetViewZoom(float zoom);

void r_RecomputeInvViewMatrix();

void r_RecomputeProjectionMatrix();

struct r_view_t *r_GetViewPointer();

/*
=================================================================
=================================================================
=================================================================
*/

void r_BeginDrawCmdSubmission(struct r_submission_state_t *submission_state);

uint32_t r_EndDrawCmdSubmission(struct r_submission_state_t *submission_state);

void r_BeginSubmission();

void r_EndSubmission();

void r_DrawAnimationFrame(struct spr_animation_h animation, vec2_t *position, float scale, float rotation, uint32_t frame, uint32_t flipped, uint32_t layer);

void r_DrawSprite(struct spr_sprite_h sprite, vec2_t *position, float scale, float rotation, uint32_t frame, uint32_t flipped, uint32_t layer);

void r_DispatchPending();

struct r_uniform_buffer_t *r_AllocateUniformBuffer(union r_command_buffer_h command_buffer);

/*
=================================================================
=================================================================
=================================================================
*/

void r_i_BeginSubmission();

void r_i_EndSubmission();

void r_i_DispatchPending();

void *r_i_AllocateDrawCmdData(uint32_t size);

void r_i_DrawCmd(uint32_t type, void *data);

void r_i_SetPipeline(uint32_t pipeline_index);

void r_i_DrawLines(struct r_i_vertex_t *verts, uint32_t vert_count, float size, uint32_t line_strip);

void r_i_DrawLine(struct r_i_vertex_t *from, struct r_i_vertex_t *to, float size);

void r_i_DrawLineStrip(struct r_i_vertex_t *verts, uint32_t vert_count, float size);

void r_i_DrawTris(struct r_i_vertex_t *verts, uint32_t vert_count, uint32_t *indices, uint32_t indice_count, uint32_t fill);

void r_i_DrawTriLine(struct r_i_vertex_t *verts);

void r_i_DrawTriFill(struct r_i_vertex_t *verts);



#endif // R_DRAW_H
