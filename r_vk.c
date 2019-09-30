#include "r_vk.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct rBackend r_backend;


void r_InitBackend()
{
    r_backend.window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);

    r_InitVkBackend();
}

void r_InitVkDevice()
{
    VkInstanceCreateInfo instance_create_info = {};
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    VkDeviceCreateInfo device_create_info = {};
    VkDeviceQueueCreateInfo queue_create_info = {};
    VkPhysicalDeviceMemoryProperties memory_properties = {};
    VkPhysicalDevice physical_device;
    VkImageCreateInfo image_create_info;
    VkImageViewCreateInfo image_view_create_info;

    uint32_t physical_device_count = 1;

    VkQueueFamilyProperties *queue_families = NULL;
    uint32_t queue_families_count = 0;

    VkSurfaceCapabilitiesKHR surface_capabilites;
    VkSurfaceFormatKHR surface_format;
    uint32_t surface_format_count = 1;

    VkImage *swapchain_images = NULL;
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkMemoryRequirements alloc_req;
    VkMemoryAllocateInfo alloc_info;
    VkImageView *swapchain_image_views = NULL;
    uint32_t swapchain_images_count = 0;

    VkResult result;
    SDL_SysWMinfo wm_info;
    const char const *instance_extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    const char const *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = NULL;
    instance_create_info.enabledLayerCount = 0;
    instance_create_info.ppEnabledLayerNames = NULL;

    /* I find it a bit odd that surfaces are made available through an
    extension. If this is a graphics api, wouldn't having support for
    showing stuff on the screen be something required for it to fulfill
    its use? If this was a general purpose computing library, and doing
    graphics related operations was one of the things supported, I guess
    then it'd make sense. */
    instance_create_info.enabledExtensionCount = 2;
    instance_create_info.ppEnabledExtensionNames = instance_extensions;
    
    result = vkCreateInstance(&instance_create_info, NULL, &r_backend.vk_backend.instance);

    if(result != VK_SUCCESS)
    {
        printf("error creating instance!\n");
        return;
    }   

    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(r_backend.window, &wm_info);

    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = NULL;
    surface_create_info.flags = 0;
    surface_create_info.hinstance = wm_info.info.win.hinstance;
    surface_create_info.hwnd = wm_info.info.win.window;

    result = vkCreateWin32SurfaceKHR(r_backend.vk_backend.instance, &surface_create_info, NULL, &r_backend.vk_backend.surface);

    if(result != VK_SUCCESS)
    {
        printf("error creating surface!\n");
        return;
    }

    /* will the main gpu be the first on the list? I'm assuming it will for now... */
    result = vkEnumeratePhysicalDevices(r_backend.vk_backend.instance, &physical_device_count, &physical_device);

    if(result != VK_SUCCESS)
    {
        printf("error enumerating physical devices!\n");
    }

    vkGetPhysicalDeviceMemoryProperties(physical_device, &r_backend.vk_backend.memory_properties);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, NULL);

    if(queue_families_count)
    {
        queue_families = alloca(sizeof(VkQueueFamilyProperties) * queue_families_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_families);

        r_backend.vk_backend.graphics_queue_index = 0xffffffff;
        r_backend.vk_backend.present_queue_index = 0xffffffff;

        for(uint32_t i = 0; i < queue_families_count; i++)
        {
            if((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
                r_backend.vk_backend.graphics_queue_index == 0xffffffff)
            {
                r_backend.vk_backend.graphics_queue_index = i;
            }

            VkBool32 present_capable = 0;

            if(r_backend.vk_backend.present_queue_index == 0xffffffff)
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, r_backend.vk_backend.surface, &present_capable);
                if(present_capable)
                {
                    r_backend.vk_backend.present_queue_index = i;
                }
            }
        }
    }

    float *priorities = alloca(sizeof(float) * queue_families[r_backend.vk_backend.graphics_queue_index].queueCount);
    /* give all queues the same priority (0.0)... */
    memset(priorities, 0, sizeof(float) * queue_families[r_backend.vk_backend.graphics_queue_index].queueCount);

    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pNext = NULL;
    queue_create_info.flags = 0;
    queue_create_info.queueFamilyIndex = r_backend.vk_backend.graphics_queue_index;
    queue_create_info.queueCount = queue_families[r_backend.vk_backend.graphics_queue_index].queueCount;

    /* this is a bit odd. The tutorial declares an array with a single float inside,
    but the spec says this array should contain a value for each queue in the queue family.
    If there's only a single queue in the family, then it's fine. Otherwise, wouldn't 
    vulkan read garbage after the first element? */
    queue_create_info.pQueuePriorities = priorities;


    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = NULL;
    device_create_info.flags = 0;
    /* this will not always work. If presenting is not supported by the queue
    being used for graphics operation, we'll need two queues to get things going... */
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;

    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = NULL;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = device_extensions;
    device_create_info.pEnabledFeatures = NULL;

    result = vkCreateDevice(physical_device, &device_create_info, NULL, &r_backend.vk_backend.device);

    if(result != VK_SUCCESS)
    {
        printf("error creating device!\n");
        return;
    }

    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, r_backend.vk_backend.surface, &surface_format_count, &surface_format);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, r_backend.vk_backend.surface, &surface_capabilites);

    /* this pretty much works as the window backbuffer... */
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = r_backend.vk_backend.surface;
    swapchain_create_info.minImageCount = surface_capabilites.minImageCount;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = surface_capabilites.currentExtent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = NULL;
    swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; /* present the image as is */
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;   /* consider the image having alpha = 1.0 */
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;               /* queue images, present them during vertical blank */
    swapchain_create_info.clipped = VK_TRUE;                                    /* allow vulkan to discard pixels outside the visible area*/
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(r_backend.vk_backend.device, &swapchain_create_info, NULL, &r_backend.vk_backend.swapchain);

    if(result != VK_SUCCESS)
    {
        printf("error creating the swapchain!\n");
        return;
    }

    vkGetSwapchainImagesKHR(r_backend.vk_backend.device, r_backend.vk_backend.swapchain, &swapchain_images_count, NULL);

    if(swapchain_images_count)
    {
        swapchain_images = alloca(sizeof(VkImage) * swapchain_images_count);
        vkGetSwapchainImagesKHR(r_backend.vk_backend.device, r_backend.vk_backend.swapchain, &swapchain_images_count, swapchain_images);

        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = NULL;
        image_view_create_info.flags = 0;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = surface_format.format;
        
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        swapchain_image_views = alloca(sizeof(VkImageView) * swapchain_images_count);

        for(uint32_t i; i < swapchain_images_count; i++)
        {
            image_view_create_info.image = swapchain_images[i];
            vkCreateImageView(r_backend.vk_backend.device, &image_view_create_info, NULL, swapchain_image_views + i);
        }

        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = NULL;
        image_create_info.flags = 0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = VK_FORMAT_D16_UNORM;
        image_create_info.extent = surface_capabilites.currentExtent;
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = 1;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.queueFamilyIndexCount = 0;
        image_create_info.pQueueFamilyIndices = NULL;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        vkCreateImage(r_backend.vk_backend.device, &image_create_info, NULL, &depth_image);
        vkGetImageMemoryRequirements(device, depth_image, &alloc_req);

        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = NULL;
        alloc_info.allocationSize = alloc_req.size;
        alloc_info.memoryTypeIndex = r_MemoryTypeFromProperties(alloc_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
}

void r_InitVkBackend()
{
    r_InitVkDevice();
}

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t requirement)
{
    for(uint32_t i = 0; i < r_backend.vk_backend.memory_properties.memoryTypeCount; i++)
    {
        if(type_bits & 1)
        {
            if((r_backend.vk_backend.memory_properties.memoryTypes[i] & requirement) == requirement)
            {
                return i;
            }
        }

        type_bits >>= 1;
    }

    return 0xffffffff;
}





// void r_GetPhysicalDeviceAggregateProperties(VkPhysicalDevice physical_device, struct rPhysicalDeviceAggregateProperties *properties)
// {
//     vkResult result;

//     memset(properties, 0, sizeof(struct rPhysicalDeviceAggregateProperties));

//     vkGetPhysicalDeviceMemoryProperties(physical_device, &properties->memory_properties);
//     vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &properties->queue_families_count, NULL);

//     if(properties->queue_families_count)
//     {
//         properties = calloc(sizeof(VkQueueFamilyProperties), properties->queue_families_count);
//         vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &properties->queue_families_count, properties->queue_families);
//     }
// }

// void r_GetPhysicalDeviceSurfaceProperties(VkPhysicalDevice physical_device, VkSurfaceKHR surface, struct rSurfaceProperties *surface_properties)
// {
//     memset(surface_properties, 0, sizeof(struct rSurfaceProperties));

//     vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_properties->surface_capabilites);

//     vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &surface_properties->present_modes_count, NULL);
//     if(surface_properties->present_modes_count)
//     {
//         surface_properties->present_modes = calloc(sizeof(VkPresentModeKHR), surface_properties->present_modes);
//         vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, &surface_properties->present)
//     }
// }

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
            devices = calloc(sizeof(VkPhysicalDevice) * 10, devices_count);
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
        families = calloc(sizeof(VkQueueFamilyProperties) * 10, families_count);
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
            formats = calloc(sizeof(VkSurfaceFormatKHR) * 10, format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);

            // for(uint32_t i = 0; i < format_count; i++)
            // {
            //     if(formats[i].format == VK_FORMAT_UNDEFINED)
            //     {
            //         formats[i].format = VK_FORMAT_UNDEFINED;
            //     }
            // }
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
            modes = calloc(sizeof(VkPresentModeKHR) * 10, modes_count);
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
            imgs = calloc(sizeof(VkImage) * 10, count);
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

uint32_t r_GetMemoryTypeFromProperties(VkPhysicalDeviceMemoryProperties *memory_properties, uint32_t type_bits, uint32_t mask)
{
    for(uint32_t i = 0; i < memory_properties->memoryTypeCount; i++)
    {
        if(type_bits & 1)
        {
            /* the physical device has this type of memory... */
            if((memory_properties->memoryTypes[i].propertyFlags & mask) == mask)
            {
                return i;
            }
        }

        type_bits >>= 1;
    }

    return 0xffffffff;
}