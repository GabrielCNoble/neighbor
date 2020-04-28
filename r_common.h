#ifndef R_COMMON_H
#define R_COMMON_H

#include <stdint.h>
#include "SDL/include/SDL2/SDL.h"
//#include "SDL/include/SDL2/SDL_vulkan.h"
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

struct r_queue_t
{
    VkQueue queue;
    SDL_SpinLock spinlock;
};

/*
=================================================================
=================================================================
=================================================================
*/

struct r_viewport_t
{
    VkViewport viewport;
    VkRect2D scissor;
};

/*
=================================================================
=================================================================
=================================================================
*/


enum R_HEAP_TYPE
{
    R_HEAP_TYPE_BASE = 0,
    R_HEAP_TYPE_BUFFER,
    R_HEAP_TYPE_IMAGE,
};

#define BASE_HEAP_FIELDS \
    struct stack_list_t alloc_chunks; \
    struct list_t free_chunks; \
    SDL_SpinLock spinlock; \
    uint32_t size; \
    uint32_t type

struct r_heap_t
{
    BASE_HEAP_FIELDS;
};

struct r_image_heap_t
{
    BASE_HEAP_FIELDS;
    VkDeviceMemory memory;
    VkFormat *supported_formats;
    uint32_t supported_format_count;
};

struct r_buffer_heap_t
{
    BASE_HEAP_FIELDS;
    VkDeviceMemory memory;
    VkBuffer buffer;
};

struct r_heap_handle_t
{
    uint8_t index;
    uint8_t type;
};

#define R_INVALID_HEAP_INDEX 0xffff
#define R_HEAP_HANDLE(index, type) (struct r_heap_handle_t){index, type}
#define R_INVALID_HEAP_HANDLE R_HEAP_HANDLE(R_INVALID_HEAP_INDEX, R_HEAP_TYPE_BASE)

struct r_chunk_t
{
    uint32_t size;
    uint32_t start;
    uint32_t align;
};

struct r_chunk_handle_t
{
    uint32_t index;
    struct r_heap_handle_t heap;
};

#define R_INVALID_CHUNK_INDEX 0xffffffff
#define R_CHUNK_HANDLE(index, heap) (struct r_chunk_handle_t){index, heap}
#define R_INVALID_CHUNK_HANDLE R_CHUNK_HANDLE(R_INVALID_CHUNK_INDEX, R_INVALID_HEAP_HANDLE)

//
//struct r_alloc_t
//{
//    uint32_t size;
//    uint32_t start;
//    uint32_t align;
//};
//
//struct r_alloc_handle_t
//{
//    unsigned alloc_index : 31;
//    unsigned is_index : 1;
//};
//
//#define R_INVALID_ALLOC_INDEX 0x7fffffff
//#define R_ALLOC_HANDLE(alloc_index, is_index_alloc) (struct r_alloc_handle_t){alloc_index, is_index_alloc}
//#define R_INVALID_ALLOC_HANDLE R_ALLOC_HANDLE(R_INVALID_ALLOC_INDEX, 1)


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

    /* necessary for estimating the number of descriptors
    to be allocated from the pipeline descriptor pool */
    uint32_t resource_count;
    struct r_resource_binding_t *resources;

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

struct r_buffer_t
{
    VkBuffer buffer;
    struct r_chunk_handle_t memory;
};

struct r_buffer_handle_t
{
    uint32_t index;
};

struct r_staging_buffer_t
{
    VkBuffer buffer;
    VkDeviceMemory staging_memory;
    void *staging_pointer;
};

#define R_INVALID_BUFFER_INDEX 0xffffffff
#define R_BUFFER_HANDLE(index) (struct r_buffer_handle_t){index}
#define R_INVALID_BUFFER_HANDLE R_BUFFER_HANDLE(R_INVALID_BUFFER_INDEX)

/*
=================================================================
=================================================================
=================================================================
*/

/*
    modify r_CreateImage to set the struct r_image_t VkImage to the
    default texture VkImage, so the data upload can happen in a
    separate thread.
*/

struct r_image_t
{
    VkImage image;
    struct r_chunk_handle_t memory;
    uint32_t current_layout;
    uint8_t aspect_mask;
};

struct r_image_handle_t
{
    uint32_t index;
};

struct r_staging_image_t
{
    struct r_image_handle_t image;
    VkFormat format;
};

#define R_INVALID_IMAGE_INDEX 0xffffffff
#define R_IMAGE_HANDLE(index) (struct r_image_handle_t){index}
#define R_INVALID_IMAGE_HANDLE R_IMAGE_HANDLE(R_INVALID_IMAGE_INDEX)
#define R_IS_VALID_IMAGE_HANDLE(handle) (handle.index != R_INVALID_IMAGE_INDEX)

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
//    uint16_t width;
//    uint16_t height;
//    uint16_t depth;
//    uint16_t layers;
//    uint8_t type;
//    uint8_t format;
//    uint8_t mip_levels;
//    uint8_t aspect_mask;
    VkStructureType s_type;
    const void *next;
    VkImageCreateFlags flags;
    VkImageType image_type;
    VkFormat  format;
    VkExtent3D  extent;
    uint32_t mip_levels;
    uint32_t array_layers;
    VkSampleCountFlagBits samples;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkSharingMode sharing_mode;
    uint32_t queueFamilyIndexCount;
    uint32_t *pQueueFamilyIndices;
    VkImageLayout initial_layout;
    /* ======================================= */
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
    VkImageView image_view;
    struct r_image_handle_t image;
    char *name;
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

struct r_descriptor_pool_t
{
    VkDescriptorPool descriptor_pool;
    /* TODO: investigate better command execution ordering. If two or more threads are
    sourcing descriptor sets from the same pool concurrently, and it gets depleted,
    the thread that first locks the spinlock will get to store its fence in the pool.
    However, the commands from the thread that stored its fence in the pool may finish
    executing before other threads that were also allocating descriptor sets from this pool.
    This means that the pool may get reset while the descriptor sets are being used by gpu. */

    VkFence submission_fence; /* once a pool gets depleted, it gets put in an used
    pools list. To know when it's safe to recycle it, the fence used in the submission
    that depleted this pool gets stored here. A pool can be used in several submissions,
    but only the one that depleted it is enough to tell whether it's safe to recycle it. */
    uint32_t set_count;
    uint32_t free_count;
};

struct r_descriptor_set_t
{
    VkDescriptorSet descriptor_set;

};

struct r_descriptor_pool_list_t
{
    SDL_SpinLock spinlock;
    struct list_t free_pools;
    struct list_t used_pools;
    uint32_t current_pool; /* current pool being used to allocate descriptor sets */
    uint32_t next_pool_check; /* next pool to be checked for recycling */
};

struct r_pipeline_description_t
{
    /* the const qualifier got dropped here because it was being
    a pain in the ass. We know what we're doing with those pointers.  */
    VkStructureType type;
    const void *next;
    VkPipelineCreateFlags flags;
    uint32_t stage_count;
    VkPipelineShaderStageCreateInfo *stages;
    VkPipelineVertexInputStateCreateInfo *vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo *input_assembly_state;
    VkPipelineTessellationStateCreateInfo *tesselation_state;
    VkPipelineViewportStateCreateInfo *viewport_state;
    VkPipelineRasterizationStateCreateInfo *rasterization_state;
    VkPipelineMultisampleStateCreateInfo *multisample_state;
    VkPipelineDepthStencilStateCreateInfo *depth_stencil_state;
    VkPipelineColorBlendStateCreateInfo *color_blend_state;
    VkPipelineDynamicStateCreateInfo *dynamic_state;
    /*=====================================================*/
    /* this struct only exists to allow passing struct r_shader_t
    objects to the code that created a render pass. Having those is important
    because they already have descriptor set layouts created for them. Creating
    the pipeline layout from those is a ton easier. */
    struct r_shader_t **shaders;
    uint32_t shader_count;
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
    /* ====================================================== */
    struct r_pipeline_description_t *pipeline_description;
};

struct r_render_pass_description_t
{
    VkAttachmentDescription *attachments;
    struct r_subpass_description_t *subpasses;
    uint8_t attachment_count;
    uint8_t subpass_count;
};


/* each pipeline will have a list of command pools,
and all command pools will use the same descriptor set layout.
The amount of descriptors available will be a multiple of the
number of descriptors used across all shader stages. Once a
descriptor pool runs out of descriptors, it will receive the
VkFence of the submission it happened. Next time a descriptor
set is allocated, the status of this fence will be tested. If
it's signaled, the command pool gets reset and added to the free
list once more. */
struct r_pipeline_t
{
    struct r_shader_t *vertex_shader;
    struct r_shader_t *fragment_shader;

    VkPipeline pipeline;
    VkPipelineLayout layout;
    struct r_descriptor_pool_list_t pool_lists[2];
};

struct r_render_pass_t
{
    VkRenderPass render_pass;

    union
    {
        struct r_pipeline_t *pipelines;
        struct r_pipeline_t pipeline;
    };

    uint8_t subpass_count;
    uint8_t current_subpass;
};

struct r_render_pass_handle_t
{
    uint32_t index;
};

//struct r_render_pass_begin_info_t
//{
//    VkStructureType type;
//    const void *next;
//    VkRenderPass render_pass;
//    VkFramebuffer framebuffer;
//    VkRect2D render_area;
//    uint32_t clear_value_count;
//    VkClearValue *clear_values;
//    /* ============================================ */
//    struct r_render_pass_handle_t render_pass_handle;
//    struct r_framebuffer_handle_t framebuffer_handle;
//};

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
    uint8_t current_buffer;
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

struct r_swapchain_t
{
    VkSwapchainKHR swapchain;
    struct r_image_handle_t *images;
    uint8_t image_count;
    uint8_t current_image;
};

struct r_swapchain_handle_t
{
    uint32_t index;
};

#define R_INVALID_SWAPCHAIN_INDEX 0xffffffff
#define R_SWAPCHAIN_HANDLE(index) (struct r_swapchain_handle_t){index}
#define R_INVALID_SWAPCHAIN_HANDLE R_SWAPCHAIN_HANDLE(R_INVALID_SWAPCHAIN_INDEX)

/*
=================================================================
=================================================================
=================================================================
*/


//
//enum R_MATERIAL_FLAGS
//{
//    R_MATERIAL_FLAG_INVALID = 1,
//    R_MATERIAL_FLAG_USE_DIFFUSE_TEXTURE = 1 << 1,
//    R_MATERIAL_FLAG_USE_NORMAL_TEXTURE = 1 << 2,
//};
//
//struct r_material_handle_t
//{
//    uint32_t index;
//};
//
//struct r_material_t
//{
//    uint32_t flags;
//    char *name;
//    vec4_t base_color;
//    struct r_texture_t *diffuse_texture;
//    struct r_texture_t *normal_texture;
//    // struct r_texture_handle_t diffuse_texture;
//    // struct r_texture_handle_t normal_texture;
//};
//
//#define R_INVALID_MATERIAL_INDEX 0xffffffff
//#define R_MATERIAL_HANDLE(index) (struct r_material_handle_t){index}
//#define R_INVALID_MATERIAL_HANDLE R_MATERIAL_HANDLE(R_INVALID_MATERIAL_INDEX)

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
//#define R_HEAP_SIZE 67108864

//#define R_SAMPLER_COUNT 4
// #define R_MAX_DRAW_CMDS 64
// #define R_MAX_UNORDERED_DRAW_CMDS 1024
//#define R_DRAW_CMD_BUFFER_DRAW_CMDS 1024
//#define R_CMD_DATA_ELEM_SIZE 64

//enum R_CMD_TYPE
//{
//    R_CMD_TYPE_BEGIN_FRAME,
//    R_CMD_TYPE_DRAW,
//    R_CMD_TYPE_SORT_DRAW_CMDS,
//    R_CMD_TYPE_END_FRAME,
//};

//struct r_cmd_t
//{
//    uint32_t cmd_type;
//    void *data;
//};
//struct r_draw_range_t
//{
//    uint32_t start;
//    uint32_t count;
//};

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
//struct r_draw_cmd_t
//{
//    mat4_t model_matrix;
////    struct r_material_t *material;  /* necessary for sorting... */
////    struct r_draw_range_t range;
//};



#define R_DEFAULT_WIDTH 800
#define R_DEFAULT_HEIGHT 600

#endif
