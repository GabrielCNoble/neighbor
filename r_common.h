#ifndef R_COMMON_H
#define R_COMMON_H

#include <stdint.h>
#include "SDL/include/SDL2/SDL.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/containers/list.h"
#include "dstuff/containers/ringbuffer.h"
#include "dstuff/math/vector.h"
#include "dstuff/math/matrix.h"

enum R_STENCIL_OP
{
    R_STENCIL_OP_KEEP = 0,
    R_STENCIL_OP_ZERO,
    R_STENCIL_OP_REPLACE,
    R_STENCIL_OP_INC_CLAMP,
    R_STENCIL_OP_DEC_CLAMP,
    R_STENCIL_OP_INVERT,
    R_STENCIL_OP_INC_WRAP,
    R_STENCIL_OP_DEC_WRAP,
    R_STENCIL_OP_LAST,
};

enum R_COMPARE_OP
{
    R_COMPARE_OP_NONE = 0,
    R_COMPARE_OP_ALWAYS,
    R_COMPARE_OP_LESS,
    R_COMPARE_OP_LEQUAL,
    R_COMPARE_OP_EQUAL,
    R_COMPARE_OP_NEQUAL,
    R_COMPARE_OP_GEQUAL,
    R_COMPARE_OP_GREATER,
    R_COMPARE_OP_NEVER,
    R_COMPARE_OP_LAST
};

enum R_POLYGON_MODE
{
    R_POLYGON_MODE_LINE = 0,
    R_POLYGON_MODE_POINT,
    R_POLYGON_MODE_FILL,
    R_POLYGON_MODE_LAST,
};

enum R_BLEND_OP
{
    R_BLEND_OP_ADD = 0,
    R_BLEND_OP_SUB,
    R_BLEND_OP_REV_SUB,
    R_BLEND_OP_MIN,
    R_BLEND_OP_MAX,
    R_BLEND_OP_LAST,
};

enum R_BLEND_FACTOR
{
    R_BLEND_FACTOR_ZERO = 0,
    R_BLEND_FACTOR_ONE,
    R_BLEND_FACTOR_SRC_COLOR,
    R_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    R_BLEND_FACTOR_DST_COLOR,
    R_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    R_BLEND_FACTOR_SRC_ALPHA,
    R_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    R_BLEND_FACTOR_DST_ALPHA,
    R_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    R_BLEND_FACTOR_LAST,
};

enum R_POLYGON_FACE
{
    R_POLYGON_FACE_FRONT = 0,
    R_POLYGON_FACE_BACK,
    R_POLYGON_FACE_FRONT_AND_BACK,
    R_POLYGON_FACE_LAST,
};

enum R_CULL_MODE
{
    R_CULL_MODE_NONE = 0,
    R_CULL_MODE_BACK,
    R_CULL_MODE_FRONT,
    R_CULL_MODE_FRONT_AND_BACK,
    R_CULL_MODE_LAST,
};

enum R_FRONT_FACE
{
    R_FRONT_FACE_CW = 0,
    R_FRONT_FACE_CCW,
    R_FRONT_FACE_LAST
};

enum R_FORMAT
{
    R_FORMAT_R32G32B32A32_SFLOAT = 0,
    R_FORMAT_LAST,
};

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
    R_TEXTURE_TYPE_CUBEMAP_ARRAY,
    R_TEXTURE_TYPE_LAST
};

enum R_TEXTURE_FILTER
{
    R_TEXTURE_FILTER_NEAREST,
    R_TEXTURE_FILTER_LINEAR,
    R_TEXTURE_FILTER_LAST
};

enum R_TEXTURE_ADDRESS_MODE
{
    R_TEXTURE_ADDRESS_MODE_CLAMP_TO_EDGE,
    R_TEXTURE_ADDRESS_MODE_CLAMP_TO_BORDER,
    R_TEXTURE_ADDRESS_MODE_REPEAT,
    // R_TEXTURE_ADDRESS_MODE_MIRROR,
    R_TEXTURE_ADDRESS_MODE_LAST,
    // R_TEXTURE_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
};

enum R_TEXTURE_ANISOTROPY
{
    R_TEXTURE_ANISOTROPY_OFF,
    R_TEXTURE_ANISOTROPY_1X,
    R_TEXTURE_ANISOTROPY_2X,
    R_TEXTURE_ANISOTROPY_4X,
    R_TEXTURE_ANISOTROPY_8X,
};

/*
=================================================================
=================================================================
=================================================================
*/
struct vertex_t
{
    vec4_t position;
    vec4_t normal;
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


struct r_texture_sampler_params_t
{
    unsigned min_filter : 1;
    unsigned mag_filter : 1;
    unsigned mip_filter : 1;
    unsigned addr_mode_u : 2;
    unsigned addr_mode_v : 2;
    unsigned addr_mode_w : 2;
    // unsigned anisotropy : 3;
};

struct r_texture_t
{
    char *name;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t type;
    
    struct r_texture_sampler_params_t sampler_params;

    /* NOTE: add texture format here? Creating an enum for it will be an 
    awful amount of work... */
};

struct r_texture_handle_t
{
    uint32_t index;
};

#define R_DEFAULT_TEXTURE_INDEX 0x00000000
#define R_MISSING_TEXTURE_INDEX 0x00000001
#define R_INVALID_TEXTURE_INDEX 0xffffffff
#define R_TEXTURE_HANDLE(index) (struct r_texture_handle_t){index}
#define R_INVALID_TEXTURE_HANDLE R_TEXTURE_HANDLE(R_INVALID_TEXTURE_INDEX)
#define R_DIFFUSE_TEXTURE_BINDING 0
#define R_NORMAL_TEXTURE_BINDING 1

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
    char *name;
    vec4_t base_color;
    struct r_texture_t *diffuse_texture;
    struct r_texture_t *normal_texture;
    // struct r_texture_handle_t diffuse_texture;
    // struct r_texture_handle_t normal_texture;
};

#define R_INVALID_MATERIAL_INDEX 0xffffffff
#define R_MATERIAL_HANDLE(index) (struct r_material_handle_t){index}
#define R_INVALID_MATERIAL_HANDLE R_MATERIAL_HANDLE(R_INVALID_MATERIAL_INDEX)

/*
=================================================================
=================================================================
=================================================================
*/

struct r_light_handle_t
{
    uint32_t index;
};
struct r_light_t
{
    vec4_t pos_radius;
    vec4_t color;
};

#define R_INVALID_LIGHT_INDEX 0xffffffff
#define R_LIGHT_HANDLE(index) (struct r_light_handle_t){index}
#define R_INVALID_LIGHT_HANDLE R_LIGHT_HANDLE(R_INVALID_LIGHT_INDEX)

/*
=================================================================
=================================================================
=================================================================
*/

/* 64 MB */
#define R_HEAP_SIZE 67108864

#define R_SAMPLER_COUNT 4
// #define R_MAX_DRAW_CMDS 64
// #define R_MAX_UNORDERED_DRAW_CMDS 1024
#define R_DRAW_CMD_BUFFER_DRAW_CMDS 1024
#define R_CMD_DATA_ELEM_SIZE 64

enum R_CMD_TYPE
{
    R_CMD_TYPE_BEGIN_FRAME,
    R_CMD_TYPE_DRAW,
    R_CMD_TYPE_SORT_DRAW_CMDS,
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

/* unordered draw cmds */
// struct r_udraw_cmd_t
// {
//     mat4_t model_matrix;
//     struct r_material_t *material;
//     struct r_draw_range_t range;
// };
// struct r_udraw_cmd_buffer_t
// {
//     mat4_t view_projection_matrix;
//     mat4_t view_matrix;
//     uint32_t draw_cmd_count;
//     struct r_udraw_cmd_t draw_cmds[1];
// };


/* ordered draw cmds, ready for consumption by the renderer */
struct r_draw_cmd_t
{
    mat4_t model_matrix;
    struct r_material_t *material;  /* necessary for sorting... */
    struct r_draw_range_t range;
};

struct r_draw_cmd_buffer_t
{
    mat4_t view_projection_matrix;
    mat4_t view_matrix;
    uint32_t draw_cmd_count;
    struct r_draw_cmd_t draw_cmds[R_DRAW_CMD_BUFFER_DRAW_CMDS];
};


/*
=================================================================================
=================================================================================
=================================================================================
*/





struct r_renderpass_t
{

};

struct r_vertex_attrib_t
{
    uint32_t location;
    uint32_t format;
    uint32_t offset;
};

struct r_vertex_binding_t
{
    uint32_t binding;
    uint32_t stride;
    uint32_t attrib_count;
    struct r_vertex_attrib_t* attribs;
};
struct r_pipeline_description_t
{
    struct
    {
        unsigned compare_op 4;
        unsigned test_enabled: 1;
        unsigned write_enabled: 1;
    }depth_state;
    
    struct
    {
        unsigned compare_op: 4;
        unsigned fail_op: 3;
        unsigned depth_fail_op: 3;
        unsigned pass_op: 3;
        unsigned test_enabled: 1;
    }stencil_state;

    struct
    {
        unsigned blend_factor: 4;
        unsigned blend_op: 3;
    }blend_state;

    struct
    {
        unsigned polygon_mode: 2;
        unsigned front_face: 2;
        unsigned cull_face: 2;
    }rasterizer_state;

    struct
    {
        struct r_vertex_binding_t *bindings;
        uint32_t binding_count;
    }input_state;

    struct
    {

    }blend_state;
};

struct r_pipeline_t
{
    struct r_pipeline_description_t description;
};

struct r_pipeline_handle_t
{
    uint32_t index;
};

#define R_INVALID_PIPELINE_INDEX 0xffffffff
#define R_PIPELINE_HANDLE(index) (struct r_pipeline_handle_t){index}
#define R_INVALID_PIPELINE_HANDLE R_PIPELINE_HANDLE(R_INVALID_PIPELINE_INDEX)


struct r_renderer_t
{
    SDL_Window *window;

    float z_near;
    float z_far;
    float fov_y;

    uint32_t width;
    uint32_t height;

    struct stack_list_t allocd_blocks[2];
    struct list_t free_blocks[2];

    struct stack_list_t textures;
    struct stack_list_t materials;
    struct stack_list_t lights;

    SDL_SpinLock cmd_buffer_lock;
    struct ringbuffer_t cmd_buffer;
    struct ringbuffer_t cmd_buffer_data;

    mat4_t projection_matrix;
    mat4_t view_matrix;
    mat4_t model_matrix;
    mat4_t view_projection_matrix;
    mat4_t model_view_projection_matrix;
    uint32_t outdated_view_projection_matrix;

    struct r_draw_cmd_buffer_t *submiting_draw_cmd_buffer;
    
    struct stack_list_t pipelines;
};



// struct r_pipeline_state_t
// {
//     struct
//     {
//         float depth_clear_value;
//         uint8_t depth_test_enabled;
//         uint8_t depth_compare_op;
//         uint8_t depth_write_mask;

//         uint8_t stencil_test_enabled;
//         uint8_t stencil_fail_op;
//         uint8_t stencil_pass_op;
//         uint8_t depth_fail_op;
//         uint8_t stencil_write_mask;
//         uint8_t stencil_clear_value;
//         uint8_t stencil_ref_value;
//     }depth_stencil_state;

//     struct
//     {
//         float color_clear_value[4];
//         struct
//         {
//             unsigned r : 1;
//             unsigned g : 1;
//             unsigned b : 1;
//             unsigned a : 1;
//         }color_write_mask;

//     }color_state;

//     struct
//     {
//         uint8_t front_face_polygon_mode;
//         uint8_t back_face_polygon_mode;
//         uint8_t face_cull_enabled;
//         uint8_t cull_face_winding;

//     }rasterizer_state;
// };

#endif