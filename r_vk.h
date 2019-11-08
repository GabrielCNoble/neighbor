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
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
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

void r_InitVkPipeline();

/*
=================================================================
=================================================================
=================================================================
*/

VkResult r_vk_CreateBuffer(struct r_vk_buffer_t *buffer, uint32_t size, uint32_t usage);

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_InitalizeTexture(struct r_vk_texture_t *texture, unsigned char *pixels);

// VkResult r_LoadImage(struct rVkImage *image, const char *file_name);

void r_vk_SetTexture(struct r_vk_texture_t *texture, uint32_t sampler_index);

/*
=================================================================
=================================================================
=================================================================
*/

VkResult r_BeginCommandBuffer();

VkResult r_EndCommandBuffer();

uint32_t r_AcquireNextImage();

void r_UploadData();

void r_BeginRenderpass();

void r_Draw();

void r_EndRenderpass();

void r_Present();

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t requirement);

struct rVkShader r_LoadShader(const char *file_name);

void r_vk_SetProjectionMatrix(mat4_t *projection_matrix);

// void r_vk_BindTexture()

// void r_vk_SetViewMatrix(mat4_t *view_matrix);

// void r_vk_SetModelMatrix(mat4_t *model_matrix);

void r_vk_UpdateModelViewProjectionMatrix(mat4_t *m);

#endif