#ifndef R_DEFS_H
#define R_DEFS_H

#include <stdint.h>
//#include "lib/SDL/include/SDL.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_list.h"
#include "lib/dstuff/ds_alloc.h"
#include "lib/dstuff/ds_ringbuffer.h"
#include "lib/dstuff/ds_vector.h"
#include "lib/dstuff/ds_matrix.h"
#include "lib/tinycthread/tinycthread.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "lib/vulkan/Include/vulkan/vulkan.h"
#include "lib/vulkan/Include/vulkan/vulkan_core.h"

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

struct r_framebuffer_h
{
    uint32_t index;
};

#define R_INVALID_FRAMEBUFFER_INDEX 0xffffffff
#define R_FRAMEBUFFER_HANDLE(index) (struct r_framebuffer_h){index}
#define R_INVALID_FRAMEBUFFER_HANDLE R_FRAMEBUFFER_HANDLE(R_INVALID_FRAMEBUFFER_INDEX)
#define R_DEFAULT_FRAMEBUFFER_HANDLE R_FRAMEBUFFER_HANDLE(0)


struct r_heap_h
{
    uint8_t index;
    uint8_t type;
};

#define R_INVALID_HEAP_INDEX 0xff
#define R_HEAP_HANDLE(index, type) (struct r_heap_h){index, type}
#define R_INVALID_HEAP_HANDLE R_HEAP_HANDLE(R_INVALID_HEAP_INDEX, R_HEAP_TYPE_BASE)
#define R_DEFAULT_VERTEX_HEAP R_HEAP_HANDLE(0, R_HEAP_TYPE_BUFFER)
#define R_DEFAULT_INDEX_HEAP R_HEAP_HANDLE(1, R_HEAP_TYPE_BUFFER)


struct r_chunk_h
{
    uint32_t index;
    struct r_heap_h heap;
};

#define R_INVALID_CHUNK_INDEX 0xffffffff
#define R_CHUNK_HANDLE(index, heap) (struct r_chunk_h){index, heap}
#define R_INVALID_CHUNK_HANDLE R_CHUNK_HANDLE(R_INVALID_CHUNK_INDEX, R_INVALID_HEAP_HANDLE)


struct r_shader_handle_t
{
    uint32_t index;
};

#define R_INVALID_SHADER_INDEX 0xffffffff
#define R_SHADER_HANDLE(index) (struct r_shader_handle_t){index}
#define R_INVALID_SHADER_HANDLE R_SHADER_HANDLE(R_INVALID_SHADER_INDEX)


struct r_buffer_h
{
    uint32_t index;
};

#define R_INVALID_BUFFER_INDEX 0xffffffff
#define R_BUFFER_HANDLE(index) (struct r_buffer_h){index}
#define R_INVALID_BUFFER_HANDLE R_BUFFER_HANDLE(R_INVALID_BUFFER_INDEX)


struct r_texture_h
{
    uint32_t index;
};

#define R_INVALID_TEXTURE_INDEX 0xffffffff
#define R_TEXTURE_HANDLE(index) (struct r_texture_h){index}
#define R_INVALID_TEXTURE_HANDLE R_TEXTURE_HANDLE(R_INVALID_TEXTURE_INDEX)
#define R_DIFFUSE_TEXTURE_BINDING 0
#define R_NORMAL_TEXTURE_BINDING 1


struct r_pipeline_h
{
    uint32_t index;
};

#define R_INVALID_PIPELINE_INDEX 0xffffffff
#define R_PIPELINE_HANDLE(index) (struct r_pipeline_h){index}
#define R_INVALID_PIPELINE_HANDLE R_PIPELINE_HANDLE(R_INVALID_PIPELINE_INDEX)


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

struct r_queue_t
{
    VkQueue queue;
    spnl_t spinlock;
    uint32_t max_command_buffers;
    VkCommandBuffer *command_buffers;
};

struct r_queue_h
{
    uint32_t index;
};


/*
=================================================================
=================================================================
=================================================================
*/

struct r_fence_t
{
    VkFence fence;
    uint32_t ref_count;
};

struct r_fence_h
{
    uint32_t index;
};

#define R_INVALID_FENCE_INDEX 0xffffffff
#define R_FENCE_HANDLE(index) (struct r_fence_h){index}
#define R_INVALID_FENCE_HANDLE R_FENCE_HANDLE(R_INVALID_FENCE_INDEX)

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


struct r_staging_buffer_t
{
    void *memory;
    uint32_t size;
    uint32_t offset;
    struct ds_chunk_h chunk;
    VkEvent complete_event;
};



enum R_HEAP_TYPE
{
    R_HEAP_TYPE_BASE = 0,
    R_HEAP_TYPE_BUFFER,
    R_HEAP_TYPE_IMAGE,
};

#define BASE_HEAP_FIELDS \
    struct stack_list_t alloc_chunks; \
    struct list_t free_chunks; \
    struct list_t move_chunks; \
    spnl_t spinlock; \
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
    void *mapped_memory;
};



struct r_chunk_t
{
    uint32_t size;
    uint32_t start;
    uint32_t align;
};

struct r_chunk_move_t
{
    struct r_chunk_h chunk;
    uint32_t new_start;
};

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
    uint32_t vertex_attrib_count;
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

/*
=================================================================
=================================================================
=================================================================
*/

struct r_buffer_t
{
    VkBuffer buffer;
    struct r_chunk_h memory;
};

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
    struct r_chunk_h memory;
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
    VkEvent event;
    char *name;
};

/*
=================================================================================
=================================================================================
=================================================================================
*/


struct r_descriptor_pool_t
{
    VkDescriptorPool descriptor_pool;
    /* TODO: investigate better command execution ordering. If two or more threads are
    sourcing descriptor sets from the same pool concurrently, and it gets depleted,
    the thread that first locks the spinlock will get to store its fence in the pool.
    However, the commands from the thread that stored its fence in the pool may finish
    executing before other threads that were also allocating descriptor sets from this pool.
    This means that the pool may get reset while the descriptor sets are being used by gpu. */

    VkEvent exhaustion_event;
    uint32_t set_count;
    uint32_t free_count;
};

struct r_descriptor_set_t
{
    VkDescriptorSet descriptor_set;

};

struct r_descriptor_pool_list_t
{
    spnl_t spinlock;
    struct list_t free_pools;
    struct list_t used_pools;
    uint32_t current_pool; /* current pool being used to allocate descriptor sets */
    uint32_t next_pool_check; /* next pool to be checked for recycling */
};

struct r_pipeline_description_t
{
    /* the const qualifier got dropped here because it was being
    a pain in the ass. We know what we're doing with those pointers.  */
    struct r_shader_t **shaders;
    uint32_t shader_count;
    VkPipelineVertexInputStateCreateInfo *vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo *input_assembly_state;
    VkPipelineTessellationStateCreateInfo *tesselation_state;
    VkPipelineViewportStateCreateInfo *viewport_state;
    VkPipelineRasterizationStateCreateInfo *rasterization_state;
    VkPipelineMultisampleStateCreateInfo *multisample_state;
    VkPipelineDepthStencilStateCreateInfo *depth_stencil_state;
    VkPipelineColorBlendStateCreateInfo *color_blend_state;
    VkPipelineDynamicStateCreateInfo *dynamic_state;
    VkRenderPass render_pass;
    uint32_t subpass_index;
};

struct r_pipeline_state_t
{
    struct
    {
        uint32_t test_enable: 1;
        uint32_t write_enable: 1;
        uint32_t compare_op: 3;
    }depth_state;

    struct
    {
        uint32_t test_enable: 1;
        struct
        {
            uint32_t fail_op: 3;
            uint32_t pass_op: 3;
            uint32_t depth_fail_op: 3;
            uint32_t compare_op: 3;
        }front, back;

    }stencil_state;
    
    struct 
    {
        uint32_t polygon_mode: 2;
        uint32_t cull_mode: 2;
        uint32_t front_face: 1;
    }rasterizer_state;
    
    struct
    {
        uint32_t topology: 4;
    }input_assembly_state;
    
    struct
    {
        uint32_t test_enable: 1;
        uint32_t src_color_blend_factor: 5;
        uint32_t dst_color_blend_factor: 5;
        uint32_t color_blend_op: 3;
        uint32_t src_alpha_blend_factor: 5;
        uint32_t dst_alpha_blend_factor: 5;
        uint32_t alpha_blend_op: 3;
        uint32_t color_write_mask: 4;
        float blend_constants[4];
    }color_blend_state;
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
    struct r_pipeline_state_t state;
    struct r_descriptor_pool_list_t pool_lists[2];
};

/*
=================================================================================
=================================================================================
=================================================================================
*/

struct r_attachment_d
{
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkImageTiling tiling;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op; 
    VkAttachmentLoadOp stencil_load_op;
    VkAttachmentStoreOp stencil_store_op;
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
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
    uint32_t pipeline_description_count;
    struct r_pipeline_description_t *pipeline_descriptions;
};

struct r_render_pass_description_t
{
    struct r_attachment_d *attachments;
    struct r_subpass_description_t *subpasses;
    uint8_t attachment_count;
    uint8_t subpass_count;
};

struct r_subpass_t
{
    uint32_t pipeline_count;
    struct r_pipeline_h *pipelines;
};

struct r_render_pass_t
{
    VkRenderPass render_pass;
    struct r_pipeline_h *pipelines;
    struct r_subpass_t *subpasses;
    uint32_t pipeline_count;
    uint8_t subpass_count;
    struct r_attachment_d *attachments;
    uint32_t attachment_count;
};

struct r_render_pass_begin_info_t
{
    struct r_render_pass_handle_t render_pass;
    struct r_framebuffer_h framebuffer;
    VkRect2D render_area;
    uint32_t clear_value_count;
    VkClearValue *clear_values;
};

/*
=================================================================
=================================================================
=================================================================
*/


struct r_framebuffer_description_t
{
//    VkAttachmentDescription *attachments;
    struct r_attachment_d *attachments;
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
    struct r_texture_h *textures;
    uint8_t texture_count;
    uint8_t current_buffer;
};

/*
=================================================================
=================================================================
=================================================================
*/

struct r_swapchain_t
{
    VkSwapchainKHR swapchain;
    struct r_image_handle_t *images;
    uint32_t image_count;
    uint32_t current_image;
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

struct r_command_buffer_t
{
    VkCommandBuffer command_buffer;
    struct r_fence_h submit_fence;
    struct r_render_pass_handle_t render_pass;
    struct r_framebuffer_h framebuffer;
    struct list_t events;
};

union r_command_buffer_h
{
    VkCommandBuffer command_buffer;
    uint32_t index;
};

#define R_INVALID_COMMAND_BUFFER_INDEX 0xffffffff
#define R_COMMAND_BUFFER_HANDLE(index) (union r_command_buffer_h){index}
#define R_INVALID_COMMAND_BUFFER_HANDLE R_COMMAND_BUFFER_HANDLE(R_INVALID_BUFFER_INDEX)

struct r_submit_info_t
{
    VkStructureType s_type;
    const void *next;
    uint32_t wait_semaphore_count;
    const VkSemaphore *wait_semaphores;
    const VkPipelineStageFlags *wait_dst_stage_mask;
    uint32_t command_buffer_count;
    union r_command_buffer_h *command_buffers;
    uint32_t signal_semaphore_count;
    const VkSemaphore *signal_semaphores;
};

/*
=================================================================
=================================================================
=================================================================
*/

#define R_DEFAULT_WIDTH 1300
#define R_DEFAULT_HEIGHT 760

#endif
