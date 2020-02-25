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

struct r_vk_pipeline_t
{
    struct r_pipeline_t base;
    VkPipelineLayout layout;
    VkPipeline pipeline;
};
struct r_vk_texture_t
{
    struct r_texture_t base;
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
    uint32_t sampler_index;
};

struct r_vk_shader_t
{
    struct r_shader_t base;
    VkShaderModule module;
    VkDescriptorSetLayout descriptor_set_layout;
};

struct r_vk_framebuffer_t
{
    struct r_framebuffer_t base;
    VkFramebuffer *framebuffers;
};

struct r_vk_render_pass_t
{
    struct r_render_pass_t base;
    VkRenderPass render_pass;
};

struct r_vk_swapchain_t
{
    VkSwapchainKHR swapchain;
    uint32_t image_count;
    VkImage *images;
    VkImageView *image_views;
};

//union r_vk_sampler_tag_t
//{
//    struct r_texture_sampler_params_t params;
//    uint16_t tag;
//};

//struct r_vk_sampler_t
//{
//    VkSampler sampler;
//    union r_vk_sampler_tag_t tag;
//};

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
    // VkSampler samplers[R_SAMPLER_COUNT];
    struct list_t samplers;
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
};


void r_vk_InitRenderer();

void r_vk_InitDevice();

void r_vk_InitSwapchain();

void r_vk_InitUniformBuffer();

void r_vk_InitHeap();

void r_vk_InitDescriptorSets();

void r_vk_InitCommandPool();

void r_vk_InitPipeline();


// void r_vk_InitExtensions();

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_CreatePipeline(struct r_pipeline_t* pipeline);

void r_vk_DestroyPipeline(struct r_pipeline_t *pipeline);

void r_vk_BindPipeline(struct r_pipeline_t *pipeline);

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_CreateShader(struct r_shader_t *shader);

void r_vk_DestroyShader(struct r_shader_t *shader);

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_CreateRenderPass(struct r_render_pass_t *render_pass);

void r_vk_DestroyRenderPass(struct r_render_pass_t *render_pass);

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_CreateFramebuffer(struct r_framebuffer_t *framebuffer);

void r_vk_DestroyFramebuffer(struct r_framebuffer_t *framebuffer);

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

void r_vk_CreateTexture(struct r_texture_t *texture);

void r_vk_DestroyTexture(struct r_texture_t *texture);

void r_vk_InitWithDefaultTexture(struct r_vk_texture_t *texture);

void r_vk_LoadTextureData(struct r_vk_texture_t *texture, unsigned char *pixels);

void r_vk_TextureSamplerIndex(struct r_vk_texture_t *texture);

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_SetMaterial(struct r_material_t *material);

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_BeginFrame();

void r_vk_PushViewMatrix(mat4_t *view_matrix);

void r_vk_Draw(struct r_material_t *material, mat4_t *view_projection_matrix, struct r_draw_cmd_t *draw_cmds, uint32_t count);

void r_vk_EndFrame();

uint32_t r_AcquireNextImage();

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_DrawPoint(vec3_t* position, vec3_t* color);

/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t requirement);

char *r_vk_ResultString(VkResult error);

struct rVkShader r_LoadShader(const char *file_name);

void r_vk_AdjustProjectionMatrix(mat4_t *projection_matrix);


#endif
