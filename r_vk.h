#ifndef R_VK_H
#define R_VK_H

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/Include/vulkan/vulkan.h"
#include "r_common.h"

// #define WIDTH 800
// #define HEIGHT 600

// struct vk_image_t
// {
//     VkImage image;
//     VkImageView image_view;
//     VkDeviceMemory image_memory;
// };

#define R_VK_AVAILABLE_CMD_BUFFERS 128
// #define R_VK_CMD_BUFFER_MAX_DRAW_CMDS 128

struct rVkShader
{
    uint32_t size;
    uint32_t *shader;
}; 

// struct r_vk_shader_t
// {
    
// };

struct r_vk_buffer_t
{
    VkBuffer buffer;
    VkDeviceMemory memory;
};

// struct rVkImage
// {
//     VkImage image;
//     VkImageView image_view;
//     VkDeviceMemory memory;
// };

struct r_vk_texture_t
{
    struct r_texture_t base;
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
};

struct r_vk_swapchain_t
{
    VkSwapchainKHR swapchain;
    uint32_t image_count;
    VkImage *images;
    VkImageView *image_views;
};

struct r_vk_renderer_t
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface; 
    struct r_vk_swapchain_t swapchain;
    // VkSwapchainKHR swapchain;
    uint32_t graphics_queue_index;
    uint32_t present_queue_index; 
    VkFramebuffer *framebuffers;
    VkPhysicalDeviceMemoryProperties memory_properties;
    
    // VkCommandBuffer command_buffer;
    VkDescriptorSet descriptor_sets[2];
    VkDescriptorPool descriptor_pool;
    VkRenderPass render_pass;
    VkQueue queue;
    
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    struct r_vk_buffer_t uniform_buffer;
    // struct rVkBuffer uniform_buffer;
    struct r_vk_buffer_t vertex_buffer;
    struct r_vk_buffer_t index_buffer;
    
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkSemaphore image_aquire_semaphore;
    VkFence submit_fence;

    // struct rVkImage texture;
    VkSampler samplers[R_SAMPLER_COUNT];
    uint32_t current_image;

    struct
    {
        VkCommandPool command_pool;
        VkCommandBuffer *command_buffers;
        // uint32_t command_buffer_count;
        uint32_t current_command_buffer;
        uint32_t current_command_buffer_draw_cmds;
        uint32_t max_command_buffer_draw_cmds;    
    }command_state;

    // struct stack_list_t textures;
};

struct r_vk_render_pass_t
{
    VkRenderPass render_pass;
};

// struct rBackend
// {
//     SDL_Window *window;

//     float z_near;
//     float z_far;

//     uint32_t width;
//     uint32_t height;

//     struct rVkBackend vk_backend;
// };


// void r_InitBackend();

// void r_InitVkBackend();

void r_InitVkRenderer();

void r_InitVkDevice();

void r_InitVkSwapchain();

void r_InitVkUniformBuffer();

void r_InitVkHeap();

void r_InitVkDescriptorSets();

void r_InitVkCommandPool();

void r_InitVkPipeline();

/*
=================================================================
=================================================================
=================================================================
*/

VkResult r_vk_CreateBuffer(struct r_vk_buffer_t *buffer, uint32_t size, uint32_t usage);

void *r_vk_MapAlloc(struct r_alloc_handle_t handle, struct r_alloc_t *alloc);

void r_vk_UnmapAlloc(struct r_alloc_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_InitalizeTexture(struct r_vk_texture_t *texture, unsigned char *pixels);

void r_vk_SetTexture(struct r_vk_texture_t *texture, uint32_t sampler_index);

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_BeginFrame();

void r_vk_Draw(struct r_cmd_t *cmd);

void r_vk_EndFrame();

/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_AcquireNextImage();

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t requirement);

struct rVkShader r_LoadShader(const char *file_name);

void r_vk_SetProjectionMatrix(mat4_t *projection_matrix);


#endif