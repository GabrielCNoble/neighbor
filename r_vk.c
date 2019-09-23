#include "r_vk.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


VkResult r_CreateInstance(VkInstance *instance)
{
    VkInstanceCreateInfo instance_create_info;
    VkApplicationInfo application_info;
    const char *instance_extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    const char* const* extensions = instance_extensions; 

    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = NULL;
    application_info.pEngineName = NULL;
    application_info.pApplicationName = NULL;
    application_info.engineVersion = 0;
    application_info.applicationVersion = 0;
    application_info.apiVersion = VK_API_VERSION_1_0;

    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.flags = 0;
    instance_create_info.ppEnabledLayerNames = NULL;
    instance_create_info.enabledLayerCount = 0;
    instance_create_info.enabledExtensionCount = 2;
    instance_create_info.ppEnabledExtensionNames = extensions;
    instance_create_info.pApplicationInfo = &application_info;

    return vkCreateInstance(&instance_create_info, NULL, instance);
}

VkResult r_EnumeratePhysicalDevices(VkInstance instance, VkPhysicalDevice **physical_devices, uint32_t *physical_devices_count)
{
    VkPhysicalDevice *devices = NULL;
    uint32_t devices_count = 0;
    VkResult result;

    result = vkEnumeratePhysicalDevices(instance, &devices_count, NULL);

    if(result == VK_SUCCESS)
    {
        if(devices_count)
        {
            devices = calloc(sizeof(VkPhysicalDevice), devices_count);
            vkEnumeratePhysicalDevices(instance, &devices_count, devices);
        }
    }

    
    *physical_devices = devices;
    *physical_devices_count = devices_count;

    return result;
}

void r_GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physical_device, VkQueueFamilyProperties **queue_families, uint32_t *queue_families_count)
{
    VkQueueFamilyProperties *families = NULL;
    uint32_t families_count = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &families_count, NULL);

    if(families_count)
    {
        families = calloc(sizeof(VkQueueFamilyProperties), families_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &families_count, families);
    }

    *queue_families = families;
    *queue_families_count = families_count;
}

void r_PrintQueueFamiliesProperties(VkQueueFamilyProperties *queue_families, uint32_t queue_families_count)
{
    uint32_t i;
    VkQueueFamilyProperties *queue_family;

    for(i = 0; i < queue_families_count; i++)
    {
        queue_family = queue_families + i;
        
        printf("***********************\n");
        printf("family index: %d\n", i);
        printf("queue count: %d\n", queue_family->queueCount);

        printf("GRAPHICS: %s\n", (queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "yes" : "no");
        printf("COMPUTE: %s\n", (queue_family->queueFlags & VK_QUEUE_COMPUTE_BIT) ? "yes" : "no");
        printf("TRANSFER: %s\n", (queue_family->queueFlags & VK_QUEUE_TRANSFER_BIT) ? "yes" : "no");
        printf("SPARSE BINDING: %s\n", (queue_family->queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? "yes" : "no");

        printf("***********************\n\n");
    }
}

VkDevice r_CreateDevice(VkPhysicalDevice physical_device)
{
    VkDevice device;
    VkDeviceCreateInfo device_create_info = {};
    VkDeviceQueueCreateInfo queue_create_info = {};


    const char const *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME}; 



    // queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    // queue_create_info.pNext = NULL;
    // queue_create_info.flags = 0;
    // queue_create_info.

    // device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // device_create_info.pNext = NULL;
    // device_create_info.flags = 0;
    // device_create_info.enabledLayerCount = 0; 
    // device_create_info.ppEnabledLayerNames = NULL;
    // device_create_info.enabledExtensionCount = 1;
    // device_create_info.ppEnabledExtensionNames = device_extensions;
    // device_create_info.

}

uint32_t r_GetCapableQueueFamilyIndex(uint32_t capability, VkQueueFamilyProperties *queue_families, uint32_t queue_families_count)
{
    uint32_t i;

    for(i = 0; i < queue_families_count; i++)
    {
        if((queue_families[i].queueFlags & capability) == capability)
        {
            return i;
        }
    }

    return 0xffffffff;
}

uint32_t r_GetPresentQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t families_count = 0;
    uint32_t i;
    VkBool32 supports;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &families_count, NULL);

    for(i = 0; i < families_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supports);

        if(supports)
        {
            return i;
        }
    }

    return 0xffffffff;
}

VkResult r_GetPhysicalDeviceSurfaceFormats(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkSurfaceFormatKHR **surface_formats, uint32_t *surface_formats_count)
{
    VkResult result;
    VkSurfaceFormatKHR *formats = NULL;
    uint32_t format_count = 0;

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);

    if(result == VK_SUCCESS)
    {
        if(format_count)
        {
            formats = calloc(sizeof(VkSurfaceFormatKHR), format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);
        }
    }

    *surface_formats = formats;
    *surface_formats_count = format_count;

    return result;
}

VkResult r_GetPhysicalDeviceSurfacePresentModes(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkPresentModeKHR **present_modes, uint32_t *present_modes_count)
{
    VkResult result;
    VkPresentModeKHR *modes = NULL;
    uint32_t modes_count = 0;

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &modes_count, NULL);

    if(result == VK_SUCCESS)
    {
        if(modes_count)
        {
            modes = calloc(sizeof(VkPresentModeKHR), modes_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &modes_count, modes);
        }
    }

    *present_modes = modes;
    *present_modes_count = modes_count;

    return result;
}

void r_PrintPresentModes(VkPresentModeKHR *present_modes, uint32_t present_modes_count)
{
    uint32_t i;
    VkPresentModeKHR *present_mode;
    printf("*************************\n");

    for(i = 0; i < present_modes_count; i++)
    {
        present_mode = present_modes + i;

        switch(*present_mode)
        {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                printf("VK_PRESENT_MODE_IMMEDIATE_KHR\n");
            break;

            case VK_PRESENT_MODE_MAILBOX_KHR:
                printf("VK_PRESENT_MODE_MAILBOX_KHR\n");
            break;

            case VK_PRESENT_MODE_FIFO_KHR:
                printf("VK_PRESENT_MODE_FIFO_KHR\n");
            break;

            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                printf("VK_PRESENT_MODE_FIFO_RELAXED_KHR\n");
            break;

            case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
                printf("VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR\n");
            break;

            case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
                printf("VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR\n");
            break;

            case VK_PRESENT_MODE_RANGE_SIZE_KHR:
                printf("VK_PRESENT_MODE_RANGE_SIZE_KHR\n");
            break;
        }
    }
    printf("*************************\n\n");
}

VkResult r_GetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, VkImage **images, uint32_t *images_count)
{
    VkResult result;
    VkImage *imgs = NULL;
    uint32_t count = 0;

    result = vkGetSwapchainImagesKHR(device, swapchain, &count, NULL);

    if(result == VK_SUCCESS)
    {
        if(count)
        {
            imgs = calloc(sizeof(VkImage), count);
            vkGetSwapchainImagesKHR(device, swapchain, &count, imgs);
        }
    }

    *images = imgs;
    *images_count = count;

    return result;
}

VkImageView r_CreateDepthBuffer(VkDevice device)
{
    // VkImage image;
    // VkImageCreateInfo image_create_info = {};
    // VkImageView image_view;
    // VkImageViewCreateInfo image_view_create_info = {};
    // VkMemoryRequirements memory_requirements;
    // VkMemoryAllocateInfo alloc_info;
    // VkPhysicalDeviceMemoryProperties memory_properties;
    // VkResult result;

    // image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    // image_create_info.pNext = NULL;
    // image_create_info.flags = 0;
    // image_create_info.imageType = VK_IMAGE_TYPE_2D;
    // image_create_info.format = VK_FORMAT_D16_UNORM;
    // image_create_info.extent.width = WIDTH;
    // image_create_info.extent.height = HEIGTH;
    // image_create_info.extent.depth = 1;
    // image_create_info.mipLevels = 1;
    // image_create_info.arrayLayers = 1;
    // image_create_info.samples = 1;
    // image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    // image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // image_create_info.queueFamilyIndexCount = 0;
    // image_create_info.pQueueFamilyIndices = NULL;
    // image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // result = vkCreateImage(device, &image_create_info, NULL, &image);

    // if(result == VK_SUCCESS)
    // {  
    //     image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    //     image_view_create_info.pNext = NULL;
    //     image_view_create_info.flags = 0;
    //     image_view_create_info.image = image;
    //     image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    //     image_view_create_info.format = VK_FORMAT_D16_UNORM;
    //     image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    //     image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    //     image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    //     image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    //     image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    //     image_view_create_info.subresourceRange.baseMipLevel = 0;
    //     image_view_create_info.subresourceRange.levelCount = 1;
    //     image_view_create_info.subresourceRange.baseArrayLayer = 0;
    //     image_view_create_info.subresourceRange.layerCount = 0;

    //     result = vkCreateImageView(device, &image_view_create_info, NULL, &image_view);

    //     if(result == VK_SUCCESS)
    //     {
    //         vkGetImageMemoryRequirements(device, image, &memory_requirements);

    //         alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    //         alloc_info.pNext = NULL;
    //         alloc_info.allocationSize = memory_requirements.size;
    //     }
    // }
}

struct vk_shader_t r_LoadShader(const char *file_name)
{
    struct vk_shader_t shader = {};
    FILE *file;

    file = fopen(file_name, "rb");

    if(file)
    {
        fseek(file, 0, SEEK_END);
        shader.size = ftell(file);
        rewind(file);

        shader.shader = calloc(1, shader.size + 1);

        fread(shader.shader, 1, shader.size, file);
        fclose(file);
    }
    else
    {
        printf("couldn't find file %s!\n", file_name);    
    }
    

    return shader;
}