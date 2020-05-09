#ifndef R_DRAW_H
#define R_DRAW_H

#include "r.h"
#include "lib/dstuff/math/matrix.h"
#include "spr.h"

struct vertex_t
{
    vec4_t position;
    vec4_t normal;
    vec4_t tex_coords;
};

struct r_draw_cmd_t
{
    vec2_t position;
    struct spr_sprite_h sprite;
    float scale;
    float rotation;
    uint32_t frame;
    uint32_t flipped;
};

#define R_DRAW_CMD_BUFFER_DRAW_CMDS 65536

struct r_draw_cmd_list_t
{
    mat4_t view_projection_matrix;
    mat4_t view_matrix;
    uint32_t draw_cmd_count;
    struct r_draw_cmd_t draw_cmds[R_DRAW_CMD_BUFFER_DRAW_CMDS];
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

void r_DrawShutdown();

void r_BeginFrame();

void r_EndFrame();

//void r_SetViewMatrix(mat4_t *view_matrix);

//void r_SetProjectionMatrix(mat4_t *projection_matrix);

/*
=================================================================
=================================================================
=================================================================
*/

void r_BeginSubmission(mat4_t *view_matrix, mat4_t *projection_matrix);

void r_DrawAnimationFrame(struct spr_animation_h animation, vec2_t *position, float scale, float rotation, uint32_t frame, uint32_t flipped);

void r_DrawSprite(struct spr_sprite_h sprite, vec2_t *position, float scale, float rotation, uint32_t frame, uint32_t flipped);

void r_EndSubmission();

void r_DispatchPending();

struct r_uniform_buffer_t *r_AllocateUniformBuffer(union r_command_buffer_h command_buffer);

/*
=================================================================
=================================================================
=================================================================
*/

void r_DrawPoint(vec3_t *position, vec3_t *color, float size);




#endif // R_DRAW_H
