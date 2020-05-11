#ifndef R_DRAW_H
#define R_DRAW_H

#include "r.h"
#include "lib/dstuff/math/matrix.h"
#include "lib/dstuff/containers/list.h"
#include "lib/dstuff/containers/stack_list.h"
#include "lib/dstuff/containers/ringbuffer.h"
#include "spr.h"

struct r_draw_state_t
{
    struct list_t draw_calls;
    struct stack_list_t draw_cmd_lists;
    struct ringbuffer_t pending_draw_cmd_lists;
    uint32_t current_draw_cmd_list;
    uint32_t draw_cmd_size;
};

struct r_view_t
{
    mat4_t projection_matrix;
    mat4_t inv_view_matrix;
    vec2_t position;
    uint32_t width;
    uint32_t height;
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

#define R_I_DRAW_DATA_SIZE 64

struct r_i_draw_data_t
{
    uint8_t data[R_I_DRAW_DATA_SIZE];
};

struct r_i_draw_line_data_t
{
    vec3_t from;
    vec3_t to;
    vec3_t color;
    float size;
};

enum R_I_DRAW_CMDS
{
    R_I_DRAW_CMD_DRAW_LINE,
    R_I_DRAW_CMD_DRAW_TRI_FILL,
    R_I_DRAW_CMD_DRAW_TRI_FILL_TEXTURED,
    R_I_DRAW_CMD_BEGIN,
    R_I_DRAW_CMD_END,
//    R_I_DRAW_CMD_TOGGLE_BLENDING,
//    R_I_DRAW_CMD_TOGGLE_STENCIL,
    R_I_DRAW_CMD_LAST,
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

/*
=================================================================
=================================================================
=================================================================
*/

void r_BeginSubmission();

void r_EndSubmission();

void r_DrawAnimationFrame(struct spr_animation_h animation, vec2_t *position, float scale, float rotation, uint32_t frame, uint32_t flipped, uint32_t layer);

void r_DrawSprite(struct spr_sprite_h sprite, vec2_t *position, float scale, float rotation, uint32_t frame, uint32_t flipped, uint32_t layer);

void r_DispatchPending(struct r_draw_state_t *draw_state);

struct r_uniform_buffer_t *r_AllocateUniformBuffer(union r_command_buffer_h command_buffer);

/*
=================================================================
=================================================================
=================================================================
*/

void r_i_BeginSubmission();

void r_i_EndSubmission();

void r_i_DispatchImmediate();

void r_i_DrawCmd(mat4_t *model_view_projection_matrix, uint32_t type, void *data, uint32_t data_size);

void r_i_DrawLine(mat4_t *model_view_projection_matrix, vec3_t *from, vec3_t *to, vec3_t *color, float size);




#endif // R_DRAW_H
