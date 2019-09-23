#ifndef R_VK_H
#define R_VK_H

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/Include/vulkan/vulkan.h"

#define WIDTH 800
#define HEIGHT 600


/*
    The workflow to create an image (a texture), is as follows:

    (1). create a VkImage. This represents an image, but no memory has been 
    allocated for it yet. 

    the next two steps can be swaped, since they are independent.

    (2). create a VkImageView. This contains information about how the texture bytes 
    will be interpreted (how to 'view' it, hence the name). The view keeps the 
    image handle.

    (3). alloc memory to back the image, by calling vkAllocateMemory.

    (4). bind the memory to the texture, by calling vkBindImageMemory. 

    allocating memory is only necessary for images that are being created
    manually. Whenever a swapchain gets created, it also creates the
    number of images requested (possibily less), allocates and binds 
    memory for them. However, creating an image view is still necessary.
    To do that, it's necessary to call vkGetSwapchainImages. This will
    return an list of handles to images. 
*/


/* just group those fuckers for now... */
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

#endif