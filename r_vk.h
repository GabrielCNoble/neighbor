#ifndef R_VK_H
#define R_VK_H

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/Include/vulkan/vulkan.h"
#include "SDL\include\SDL2\SDL.h"
#include "SDL\include\SDL2\SDL_syswm.h"

#define WIDTH 800
#define HEIGHT 600

struct vk_image_t
{
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory image_memory;
};

struct vk_shader_t
{
    uint32_t size;
    uint32_t *shader;
}; 

struct rVkBackend
{
    VkInstance instance;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    uint32_t graphics_queue_index;
    uint32_t present_queue_index; 
    VkFramebuffer *framebuffers;
    VkPhysicalDeviceMemoryProperties memory_properties;
};

struct rBackend
{
    SDL_Window *window;
    struct rVkBackend vk_backend;
};


void r_InitBackend();

void r_InitVkBackend();

void r_InitVkDevice();

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t requirement);

// void r_GetPhysicalDeviceAggregateProperties(VkPhysicalDevice physical_device, struct rPhysicalDeviceAggregateProperties *properties);

// void r_GetPhysicalDeviceSurfaceProperties(VkPhysicalDevice physical_device, VkSurfaceKHR surface, struct rSurfaceProperties *surface_properties);

VkResult r_CreateInstance(VkInstance *instance);

VkResult r_EnumeratePhysicalDevices(VkInstance instance, VkPhysicalDevice **physical_devices, uint32_t *physical_devices_count);

void r_GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physical_device, VkQueueFamilyProperties **queue_families, uint32_t *queue_families_count);

void r_PrintQueueFamiliesProperties(VkQueueFamilyProperties *queue_families, uint32_t queue_families_count);

VkDevice r_CreateDevice(VkPhysicalDevice physical_device);

uint32_t r_GetCapableQueueFamilyIndex(uint32_t capability, VkQueueFamilyProperties *queue_families, uint32_t queue_families_count);

uint32_t r_GetPresentQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

VkResult r_GetPhysicalDeviceSurfaceFormats(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkSurfaceFormatKHR **surface_formats, uint32_t *surface_formats_count);

VkResult r_GetPhysicalDeviceSurfacePresentModes(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkPresentModeKHR **present_modes, uint32_t *present_modes_count);

void r_PrintPresentModes(VkPresentModeKHR *present_modes, uint32_t present_modes_count);

VkResult r_GetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, VkImage **images, uint32_t *images_count);

VkImageView r_CreateDepthBuffer(VkDevice device);

struct vk_shader_t r_LoadShader(const char *file_name);

uint32_t r_GetMemoryTypeFromProperties(VkPhysicalDeviceMemoryProperties *memory_properties, uint32_t type_bits, uint32_t mask);

#endif