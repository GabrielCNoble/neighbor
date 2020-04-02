#ifndef R_COMMON_H
#define R_COMMON_H

#include <stdint.h>
#include "SDL/include/SDL2/SDL.h"
#include "SDL/include/SDL2/SDL_vulkan.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/containers/list.h"
#include "dstuff/containers/ringbuffer.h"
#include "dstuff/math/vector.h"
#include "dstuff/math/matrix.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/Include/vulkan/vulkan.h"
#include "vulkan/Include/vulkan/vulkan_core.h"

enum R_SHADER_RESOURCE_TYPE
{
    R_SHADER_RESOURCE_TYPE_TEXTURE = 0,
    R_SHADER_RESOURCE_TYPE_UNIFORM,
//    R_SHADER_RESOURCE_TYPE_PUSH_CONSTANT,
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

struct r_vertex_attrib_t
{
    uint16_t offset;
    uint8_t format;
    uint8_t pad;
};

 struct r_vertex_binding_t
 {
     uint32_t attrib_count;
     uint32_t size;
     struct r_vertex_attrib_t *attribs;
 };

struct r_resource_binding_t
{
    uint16_t count;
    uint8_t descriptor_type;
};

struct r_push_constant_t
{
    uint16_t size;
    uint16_t offset;
};

struct r_shader_description_t
{
    uint32_t vertex_binding_count;
    struct r_vertex_binding_t *vertex_bindings;

    uint32_t resource_count;
    struct r_resource_binding_t *resources;

    uint32_t push_constant_count;
    struct r_push_constant_t *push_constants;

    uint32_t stage;
    uint32_t code_size;
    void *code;
};

struct r_shader_t
{
    /* necessary for the pipeline vertex input state */
    uint32_t vertex_binding_count;
    struct r_vertex_binding_t *vertex_bindings;

    VkShaderModule module;
    VkDescriptorSetLayout descriptor_set_layout;

    /* necessary for creating the pipeline layout */
    uint32_t push_constant_count;
    struct r_push_constant_t *push_constants;
    uint32_t stage;
};

struct r_shader_handle_t
{
    uint32_t index;
};

#define R_INVALID_SHADER_INDEX 0xffffffff
#define R_SHADER_HANDLE(index) (struct r_shader_handle_t){index}
#define R_INVALID_SHADER_HANDLE R_SHADER_HANDLE(R_INVALID_SHADER_INDEX)

/*
=================================================================
=================================================================
=================================================================
*/
enum R_COMMAND_BUFFER_STATE
{
    R_COMMAND_BUFFER_STATE_INITIAL = 0,
    R_COMMAND_BUFFER_STATE_RECORDING,
    R_COMMAND_BUFFER_STATE_EXECUTABLE,
    R_COMMAND_BUFFER_STATE_PENDING,
    R_COMMAND_BUFFER_STATE_INVALID,
};

struct r_command_buffer_t
{
    VkCommandBuffer command_buffer;
    uint8_t state;
};

/*
=================================================================
=================================================================
=================================================================
*/

struct r_sampler_params_t
{
    unsigned addr_mode_u : 3;
    unsigned addr_mode_v : 3;
    unsigned addr_mode_w : 3;
    unsigned mipmap_mode : 1;
    unsigned min_filter : 1;
    unsigned mag_filter : 1;
};

struct r_texture_description_t
{
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t layers;
    uint8_t type;
    uint8_t format;
    uint8_t mip_levels;
//    uint8_t aspect_mask;
    struct r_sampler_params_t sampler_params;
    char *name;
};

struct r_sampler_t
{
    VkSampler sampler;
    struct r_sampler_params_t params;
};

struct r_texture_t
{
    VkSampler sampler;
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
    uint8_t current_layout;
    uint8_t aspect_mask;
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
=================================================================================
=================================================================================
=================================================================================
*/

//struct r_pipeline_reference_t
//{
//    uint8_t index;
//};

struct r_pipeline_description_t
{
    /* this struct only exists to allow passing struct r_shader_t
    objects to the code that created a render pass. Having those is important
    because they already have descriptor set layouts created for them. Creating
    the pipeline layout from those is a ton easier. */
    struct r_shader_t **shaders;
    uint32_t shader_count;

    /* the const qualifier got dropped here because it was being
    a pain in the ass. We know what we're doing with those pointers.  */
    VkPipelineVertexInputStateCreateInfo *vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo *input_assembly_state;
    VkPipelineTessellationStateCreateInfo *tesselation_state;
    VkPipelineViewportStateCreateInfo *viewport_state;
    VkPipelineRasterizationStateCreateInfo *rasterization_state;
    VkPipelineMultisampleStateCreateInfo *multisample_state;
    VkPipelineDepthStencilStateCreateInfo *depth_stencil_state;
    VkPipelineColorBlendStateCreateInfo *color_blend_state;
    VkPipelineDynamicStateCreateInfo *dynamic_state;
};

struct r_subpass_description_t
{
    VkSubpassDescriptionFlags flags;
    VkPipelineBindPoint pipeline_bind_point;
    uint32_t input_attachment_count;
    VkAttachmentReference *input_attachments;
    uint32_t color_attachment_count;
    VkAttachmentReference *color_attachments;
    VkAttachmentReference *resolve_attachments;
    VkAttachmentReference *depth_stencil_attachment;
    uint32_t preserve_attachment_count;
    uint32_t *preserve_attachments;
    struct r_pipeline_description_t *pipeline_description;
};

struct r_render_pass_description_t
{
    VkAttachmentDescription *attachments;
    struct r_subpass_description_t *subpasses;
    uint8_t attachment_count;
    uint8_t subpass_count;
};

struct r_pipeline_t
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct r_render_pass_t
{
    VkRenderPass render_pass;

    union
    {
        /* since most of the time each render pass won't have
        more than a single sub pass, the pipeline handle is stored
        within the render pass struct. This is done to avoid tiny
        heap allocations. */
        struct r_pipeline_t *pipelines;
        struct r_pipeline_t pipeline;
    };

    uint8_t pipeline_count;
};

struct r_render_pass_handle_t
{
    uint32_t index;
};

#define R_INVALID_RENDER_PASS_INDEX 0xffffffff
#define R_RENDER_PASS_HANDLE(index) (struct r_render_pass_handle_t){index}
#define R_INVALID_RENDER_PASS_HANDLE R_RENDER_PASS_HANDLE(R_INVALID_RENDER_PASS_INDEX)


/*
=================================================================
=================================================================
=================================================================
*/


struct r_framebuffer_description_t
{
    VkAttachmentDescription *attachments;
    struct r_render_pass_t *render_pass;
    uint16_t width;
    uint16_t height;
    uint8_t attachment_count;
    uint8_t frame_count;
};

struct r_framebuffer_t
{
    VkFramebuffer *buffers; /* struct r_framebuffer_t object will group several VkFramebuffer objects.
    This is to allow handling several VkFramebuffer objects with a single higher level object. */
    struct r_texture_handle_t *textures;
    uint8_t texture_count;
    uint8_t next_buffer;
};

struct r_framebuffer_handle_t
{
    uint32_t index;
};

#define R_INVALID_FRAMEBUFFER_INDEX 0xffffffff
#define R_FRAMEBUFFER_HANDLE(index) (struct r_framebuffer_handle_t){index}
#define R_INVALID_FRAMEBUFFER_HANDLE R_FRAMEBUFFER_HANDLE(R_INVALID_FRAMEBUFFER_INDEX)

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

//    struct stack_list_t textures;
    struct stack_list_t materials;
    struct stack_list_t lights;
//    struct stack_list_t framebuffers;
//    struct stack_list_t render_passes;

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
    struct stack_list_t shaders;
};

#endif
