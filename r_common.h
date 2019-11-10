#ifndef R_COMMON_H
#define R_COMMON_H

#include <stdint.h>
#include "SDL/include/SDL2/SDL.h"
#include "stack_list.h"
#include "ringbuffer.h"
#include "list.h"
#include "vector.h"
#include "matrix.h"


struct vertex_t
{
    vec4_t position;
    vec4_t color;
    vec4_t tex_coords; 
};

struct view_def_t
{
    float fov_y;
    float z_near;
    float z_far;
};

/*
=================================================================
=================================================================
=================================================================
*/

struct r_alloc_t
{
    uint32_t size;
    uint32_t start;
    uint32_t align;
};

struct r_alloc_handle_t
{
    unsigned alloc_index : 31;
    unsigned is_index : 1;
};

#define R_INVALID_ALLOC_INDEX 0x7fffffff
#define R_ALLOC_HANDLE(alloc_index, is_index_alloc) (struct r_alloc_handle_t){alloc_index, is_index_alloc}
#define R_INVALID_ALLOC_HANDLE R_ALLOC_HANDLE(R_INVALID_ALLOC_INDEX, 1)


/*
=================================================================
=================================================================
=================================================================
*/

struct r_shader_handle_t
{
    uint32_t index;
};

struct r_shader_t
{
    
};

#define R_INVALID_SHADER_INDEX 0xffffffff
#define R_SHADER_HANDLE(index) (struct r_shader_handle_t){index}
#define R_INVALID_SHADER_HANDLE R_SHADER_HANDLE(R_INVALID_SHADER_INDEX)

/*
=================================================================
=================================================================
=================================================================
*/

enum R_TEXTURE_TYPE
{
    R_TEXTURE_TYPE_INVALID = 0,
    R_TEXTURE_TYPE_NONE,
    R_TEXTURE_TYPE_1D,
    R_TEXTURE_TYPE_1D_ARRAY,
    R_TEXTURE_TYPE_2D,
    R_TEXTURE_TYPE_2D_ARRAY,
    R_TEXTURE_TYPE_3D,
    R_TEXTURE_TYPE_3D_ARRAY,
    R_TEXTURE_TYPE_CUBEMAP,
    R_TEXTYRE_TYPE_CUBEMAP_ARRAY
};

struct r_texture_t
{
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t type;
    /* NOTE: add texture format here? Creating an enum for it will be an 
    awful amount of work... */
};

struct r_texture_handle_t
{
    uint32_t index;
};

#define R_INVALID_TEXTURE_INDEX 0xffffffff
#define R_TEXTURE_HANDLE(index) (struct r_texture_handle_t){index}
#define R_INVALID_TEXTURE_HANDLE R_TEXTURE_HANDLE(R_INVALID_TEXTURE_INDEX)

/*
=================================================================
=================================================================
=================================================================
*/


enum R_MATERIAL_FLAGS
{
    R_MATERIAL_FLAG_INVALID = 1,
    R_MATERIAL_FLAG_USE_DIFFUSE_TEXTURE = 1 << 1,
    R_MATERIAL_FLAG_USE_NORMAL_TEXTURE = 1 << 2,
};

struct r_material_handle_t
{
    uint32_t index;
};

struct r_material_t
{
    uint32_t flags;
    struct r_texture_handle_t diffuse_texture;
    struct r_texture_handle_t normal_texture;
};

#define R_INVALID_MATERIAL_INDEX 0xffffffff
#define R_MATERIAL_HANDLE(index) (struct r_material_handle_t){index}
#define R_INVALID_MATERIAL_HANDLE R_MATERIAL_HANDLE(R_INVALID_MATERIAL_INDEX)

/*
=================================================================
=================================================================
=================================================================
*/

/* 64 MB */
#define R_HEAP_SIZE 67108864

#define R_SAMPLER_COUNT 4
#define R_MAX_TEMP_DRAW_BATCH_SIZE 1024

enum R_CMD_TYPE
{
    R_CMD_TYPE_BEGIN_FRAME,
    R_CMD_TYPE_DRAW,
    R_CMD_TYPE_END_FRAME,
};

struct r_cmd_t
{
    uint32_t cmd_type;
    void *data;
};

struct r_draw_range_t
{
    uint32_t start;
    uint32_t count;
};
struct r_draw_cmd_t
{
    mat4_t model_matrix;
    struct r_draw_range_t range;
};

struct r_draw_cmd_buffer_t
{
    mat4_t view_projection_matrix;
    struct r_material_handle_t material;
    uint32_t draw_cmd_count;
    struct r_draw_cmd_t draw_cmds[1];
};


#define R_CMD_DATA_ELEM_SIZE 8

struct r_renderer_t
{
    SDL_Window *window;

    float z_near;
    float z_far;

    uint32_t width;
    uint32_t height;

    struct stack_list_t allocd_blocks[2];
    struct list_t free_blocks[2];

    struct stack_list_t textures;
    struct stack_list_t materials;

    SDL_SpinLock cmd_buffer_lock;
    struct ringbuffer_t cmd_buffer;
    struct ringbuffer_t cmd_buffer_data;

    mat4_t projection_matrix;
    mat4_t view_matrix;
    mat4_t model_matrix;
    mat4_t view_projection_matrix;
    mat4_t model_view_projection_matrix;
    uint32_t outdated_view_projection_matrix;

    struct r_material_handle_t active_material;

    struct r_draw_cmd_buffer_t *draw_cmd_buffer;
};

/*
=================================================================================
=================================================================================
=================================================================================
*/


enum R_COMPARE_OP
{
    R_COMPARE_OP_NONE = 0,
    R_COMPARE_OP_ALWAYS,
    R_COMPARE_OP_LESS,
    R_COMPARE_OP_LEQUAL,
    R_COMPARE_OP_EQUAL,
    R_COMPARE_OP_GEQUAL,
    R_COMPARE_OP_GREATER,
    R_COMPARE_OP_NEVER,
    R_COMPARE_OP_INVALID
};

enum R_POLYGON_MODE
{
    R_POLYGON_MODE_NONE = 0,
    R_POLYGON_MODE_LINE,
    R_POLYGON_MODE_POINT,
    R_POLYGON_MODE_FILL,
    R_POLYGON_MODE_INVALID,
};

enum R_POLYGON_FACE
{
    R_POLYGON_FACE_FRONT = 1,
    R_POLYGON_FACE_BACK = 1 << 1,
    R_POLYGON_FACE_FRONT_AND_BACK = R_POLYGON_FACE_FRONT | R_POLYGON_FACE_BACK
};

struct r_pipeline_state_t
{
    struct
    {
        float depth_clear_value;
        uint8_t depth_test_enabled;
        uint8_t depth_compare_op;
        uint8_t depth_write_mask;

        uint8_t stencil_test_enabled;
        uint8_t stencil_fail_op;
        uint8_t stencil_pass_op;
        uint8_t depth_fail_op;
        uint8_t stencil_write_mask;
        uint8_t stencil_clear_value;
        uint8_t stencil_ref_value;
    }depth_stencil_state;

    struct
    {
        float color_clear_value[4];
        struct
        {
            unsigned r : 1;
            unsigned g : 1;
            unsigned b : 1;
            unsigned a : 1;
        }color_write_mask;

    }color_state;

    struct
    {
        uint8_t front_face_polygon_mode;
        uint8_t back_face_polygon_mode;
        uint8_t face_cull_enabled;
        uint8_t cull_face_winding;

    }rasterizer_state;
};

#endif