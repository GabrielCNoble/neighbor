

#define SDL_MAIN_HANDLED
#include "SDL/include/SDL2/SDL.h"
#include "SDL/include/SDL2/SDL_syswm.h"
#include "Windows.h"

#include "r_vk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    VkResult result;
    VkInstance instance;
    // VkInstanceCreateInfo instance_create_info;
    // VkApplicationInfo application_info;
    VkPhysicalDevice *physical_devices = NULL;
    uint32_t physical_devices_count = 0;
    VkQueueFamilyProperties *queue_family_properties = NULL;
    uint32_t queue_family_properties_count = 0;
    uint32_t graphics_queue_index = 0;
    uint32_t present_queue_index = 0;
    VkDeviceQueueCreateInfo queue_create_info;
    VkDeviceCreateInfo device_create_info;
    VkDevice device;
    VkCommandPoolCreateInfo command_pool_create_info;
    VkCommandPool command_pool;
    VkCommandBufferAllocateInfo command_buffer_allocate_info; 
    VkCommandBuffer command_buffer;
    SDL_Window *window;
    SDL_SysWMinfo info;
    HWND window_handle;
    HINSTANCE h_instance;

    VkWin32SurfaceCreateInfoKHR surface_create_info;
    VkSurfaceKHR surface;

    VkSwapchainCreateInfoKHR swap_chain_create_info;
    VkSwapchainKHR swap_chain;

    VkSurfaceFormatKHR *surface_formats;
    uint32_t surface_formats_count;

    VkSurfaceCapabilitiesKHR surface_capabilites;

    VkPresentModeKHR *present_modes;
    uint32_t present_modes_count;

    VkImageCreateInfo image_create_info;
    VkImageViewCreateInfo image_view_create_info;

    VkPhysicalDeviceMemoryProperties memory_properties;
    VkMemoryRequirements memory_requirements;
    VkMemoryAllocateInfo alloc_info;

    VkBufferCreateInfo buffer_create_info;
    VkBuffer buffer;
    VkDeviceMemory uniform_buffer_device_memory;

    VkImage *images;
    VkImage depth_buffer_image;
    VkImageView *image_views;
    VkImageView image_view;
    uint32_t images_count;

    VkDeviceMemory depth_buffer_device_memory;

    uint32_t host_visible_memory_property_index;

    uint32_t width = WIDTH;
    uint32_t height = HEIGHT;


    const char* device_extension_names[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const char* const* extensions;


    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL error. %s\n", SDL_GetError());
        return -1;
    }

    SDL_VERSION(&info.version);
    window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);

    SDL_GetWindowWMInfo(window, &info);
    window_handle = info.info.win.window;
    h_instance = info.info.win.hinstance;

    result = r_CreateInstance(&instance);

    if(result == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        printf("cannot find a compatible Vulkan ICD\n");
        exit(-1);
    }
    else if(result)
    {
        printf("unknown error\n");
        exit(-1);
    }
    else
    {
        printf("instance create succesfully!\n");
    }

    r_EnumeratePhysicalDevices(instance, &physical_devices, &physical_devices_count);

    if(physical_devices_count > 0)
    {
        printf("%d physical devices available!\n", physical_devices_count);
    
        r_GetPhysicalDeviceQueueFamilyProperties(physical_devices[0], &queue_family_properties, &queue_family_properties_count);
       
        if(queue_family_properties_count > 0)
        {
            printf("%d queue family properties for device 0\n", queue_family_properties_count);

            r_PrintQueueFamiliesProperties(queue_family_properties, queue_family_properties_count);

            graphics_queue_index = r_GetCapableQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queue_family_properties, queue_family_properties_count);

            if(graphics_queue_index < 0xffffffff)
            {
                printf("queue %d can do graphics!\n", graphics_queue_index);

                const float queue_priorities[] = {0.0};
                queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.pNext = NULL;
                queue_create_info.queueCount = queue_family_properties[graphics_queue_index].queueCount;
                queue_create_info.queueFamilyIndex = graphics_queue_index;
                queue_create_info.pQueuePriorities = queue_priorities;


                device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                device_create_info.pNext = NULL;
                device_create_info.queueCreateInfoCount = 1;
                device_create_info.pQueueCreateInfos = &queue_create_info;

                extensions = device_extension_names;

                device_create_info.enabledExtensionCount = 1;
                device_create_info.ppEnabledExtensionNames = extensions;
                device_create_info.enabledLayerCount = 0;
                device_create_info.ppEnabledLayerNames = NULL;
                device_create_info.pEnabledFeatures = NULL;
                device_create_info.flags = 0;
                
                result = vkCreateDevice(physical_devices[0], &device_create_info, NULL, &device);

                if(result == VK_SUCCESS)
                {
                    printf("device created successfully!\n");

                    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    command_pool_create_info.pNext = NULL;
                    command_pool_create_info.queueFamilyIndex = graphics_queue_index;
                    command_pool_create_info.flags = 0;

                    result = vkCreateCommandPool(device, &command_pool_create_info, NULL, &command_pool);

                    if(result == VK_SUCCESS)
                    {
                        printf("command pool created successfully!\n");

                        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                        command_buffer_allocate_info.pNext = NULL;
                        command_buffer_allocate_info.commandPool = command_pool;
                        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                        command_buffer_allocate_info.commandBufferCount = 1;

                        result = vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer);

                        if(result == VK_SUCCESS)
                        {
                            printf("command buffer created successfully!\n");

                            surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                            surface_create_info.pNext = NULL;
                            surface_create_info.flags = 0;
                            surface_create_info.hinstance = h_instance;
                            surface_create_info.hwnd = window_handle;

                            result = vkCreateWin32SurfaceKHR(instance, &surface_create_info, NULL, &surface);

                            if(result == VK_SUCCESS)
                            {
                                printf("surface created successfully!\n");

                                present_queue_index = r_GetPresentQueueFamilyIndex(physical_devices[0], surface);

                                if(present_queue_index < 0xffffffff)
                                {
                                    printf("queue %d can present!\n", present_queue_index);

                                    memset(&swap_chain_create_info, 0, sizeof(VkSwapchainCreateInfoKHR));

                                    swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                                    swap_chain_create_info.pNext = NULL;
                                    swap_chain_create_info.surface = surface;

                                    result = r_GetPhysicalDeviceSurfaceFormats(physical_devices[0], surface, &surface_formats, &surface_formats_count);

                                    if(result == VK_SUCCESS)
                                    {
                                        swap_chain_create_info.imageFormat = surface_formats[0].format;
                                        
                                        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_devices[0], surface, &surface_capabilites);

                                        if(width > surface_capabilites.maxImageExtent.width)
                                        {
                                            width = surface_capabilites.maxImageExtent.width;
                                        }

                                        if(height > surface_capabilites.maxImageExtent.height)
                                        {
                                            height = surface_capabilites.maxImageExtent.height;
                                        }

                                        swap_chain_create_info.minImageCount = surface_capabilites.minImageCount;
                                        swap_chain_create_info.preTransform = surface_capabilites.currentTransform;
                                        swap_chain_create_info.imageExtent.width = width;
                                        swap_chain_create_info.imageExtent.height = height;

                                        result = r_GetPhysicalDeviceSurfacePresentModes(physical_devices[0], surface, &present_modes, &present_modes_count);

                                        if(result == VK_SUCCESS)
                                        {
                                            if(present_modes_count)
                                            {
                                                printf("%d present modes available!\n", present_modes_count);

                                                r_PrintPresentModes(present_modes, present_modes_count);

                                                swap_chain_create_info.presentMode = present_modes[0];
                                                swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                                                swap_chain_create_info.queueFamilyIndexCount = 0;
                                                swap_chain_create_info.pQueueFamilyIndices = NULL;
                                                swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                                                swap_chain_create_info.imageArrayLayers = 1;
                                                swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;
                                                swap_chain_create_info.clipped = 1;
                                                swap_chain_create_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
                                                swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                                                result = vkCreateSwapchainKHR(device, &swap_chain_create_info, NULL, &swap_chain);

                                                if(result == VK_SUCCESS)
                                                {
                                                    printf("swapchain created successfully!\n");


                                                    result = r_GetSwapchainImages(device, swap_chain, &images, &images_count);

                                                    if(result == VK_SUCCESS)
                                                    {
                                                        printf("%d images created!\n", images_count);

                                                        image_views = calloc(sizeof(VkImageView), images_count);
                                                        for(uint32_t i; i < images_count; i++)
                                                        {
                                                            image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                                                            image_view_create_info.pNext = NULL;
                                                            image_view_create_info.flags = 0;
                                                            image_view_create_info.image = images[i]; 
                                                            image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                                                            image_view_create_info.format = surface_formats[0].format;
                                                            image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
                                                            image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
                                                            image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
                                                            image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
                                                            image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                                            image_view_create_info.subresourceRange.baseMipLevel = 0;
                                                            image_view_create_info.subresourceRange.levelCount = 1;
                                                            image_view_create_info.subresourceRange.baseArrayLayer = 0;
                                                            image_view_create_info.subresourceRange.layerCount = 1;
                                                            
                                                            result = vkCreateImageView(device, &image_view_create_info, NULL, image_views + i);

                                                            if(result != VK_SUCCESS)
                                                            {
                                                                printf("error creating image view for image %d\n", i);
                                                                break;
                                                            }
                                                        }

                                                        if(result == VK_SUCCESS)
                                                        {
                                                            printf("image views created successfully!\n");

                                                            image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                                                            image_create_info.pNext = NULL;
                                                            image_create_info.flags = 0;
                                                            image_create_info.imageType = VK_IMAGE_TYPE_2D;
                                                            image_create_info.format = VK_FORMAT_D16_UNORM;
                                                            image_create_info.extent.width = WIDTH;
                                                            image_create_info.extent.height = HEIGHT;
                                                            image_create_info.extent.depth = 1;
                                                            image_create_info.mipLevels = 1;
                                                            image_create_info.arrayLayers = 1;
                                                            image_create_info.samples = 1;
                                                            image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
                                                            image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                                                            image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                                                            image_create_info.queueFamilyIndexCount = 0;
                                                            image_create_info.pQueueFamilyIndices = NULL;
                                                            image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                                                            result = vkCreateImage(device, &image_create_info, NULL, &depth_buffer_image);

                                                            if(result == VK_SUCCESS)
                                                            {  
                                                                printf("depth buffer image created successfully!\n");

                                                                image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                                                                image_view_create_info.pNext = NULL;
                                                                image_view_create_info.flags = 0;
                                                                image_view_create_info.image = depth_buffer_image;
                                                                image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                                                                image_view_create_info.format = VK_FORMAT_D16_UNORM;
                                                                image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
                                                                image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
                                                                image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
                                                                image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
                                                                image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                                                                image_view_create_info.subresourceRange.baseMipLevel = 0;
                                                                image_view_create_info.subresourceRange.levelCount = 1;
                                                                image_view_create_info.subresourceRange.baseArrayLayer = 0;
                                                                image_view_create_info.subresourceRange.layerCount = 0;

                                                                result = vkCreateImageView(device, &image_view_create_info, NULL, &image_view);

                                                                if(result == VK_SUCCESS)
                                                                {
                                                                    printf("image view for depth buffer created successfully!\n");

                                                                    vkGetImageMemoryRequirements(device, depth_buffer_image, &memory_requirements);
                                                                    vkGetPhysicalDeviceMemoryProperties(physical_devices[0], &memory_properties);

                                                                    for(host_visible_memory_property_index = 0; host_visible_memory_property_index < memory_properties.memoryTypeCount; host_visible_memory_property_index++)
                                                                    {
                                                                        if(memory_properties.memoryTypes[host_visible_memory_property_index].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                                                                        {
                                                                            break;
                                                                        }
                                                                    }

                                                                    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                                                                    alloc_info.pNext = NULL;
                                                                    alloc_info.allocationSize = memory_requirements.size;
                                                                    alloc_info.memoryTypeIndex = host_visible_memory_property_index;

                                                                    result = vkAllocateMemory(device, &alloc_info, NULL, &depth_buffer_device_memory);

                                                                    if(result == VK_SUCCESS)
                                                                    {
                                                                        printf("memory for depth buffer allocated succesfully!\n");

                                                                        result = vkBindImageMemory(device, depth_buffer_image, depth_buffer_device_memory, 0);

                                                                        if(result == VK_SUCCESS)
                                                                        {
                                                                            printf("memory for depth buffer bound property!\n");

                                                                            buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                                                                            buffer_create_info.pNext = NULL;
                                                                            buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                                                                            buffer_create_info.size = sizeof(float[16]);
                                                                            buffer_create_info.queueFamilyIndexCount = 0;
                                                                            buffer_create_info.pQueueFamilyIndices = NULL;
                                                                            buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                                                                            buffer_create_info.flags = 0;

                                                                            result = vkCreateBuffer(device, &buffer_create_info, NULL, &buffer);

                                                                            if(result == VK_SUCCESS)
                                                                            {
                                                                                printf("buffer created sucessfully!\n");

                                                                                vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

                                                                                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                                                                                alloc_info.pNext = NULL;
                                                                                alloc_info.memoryTypeIndex = host_visible_memory_property_index;
                                                                                alloc_info.allocationSize = memory_requirements.size;

                                                                                result = vkAllocateMemory(device, &alloc_info, NULL, &uniform_buffer_device_memory);

                                                                                if(result == VK_SUCCESS)
                                                                                {
                                                                                    printf("uniform buffer memory allocated successfully!\n");

                                                                                    result = vkBindBufferMemory(device, buffer, uniform_buffer_device_memory, 0);

                                                                                    if(result == VK_SUCCESS)
                                                                                    {
                                                                                        printf("uniform buffer memory bound successfully!\n");

                                                                                        float mvp [] = {1.0, 0.0, 0.0, 0.0,
                                                                                                        0.0, 1.0, 0.0, 0.0, 
                                                                                                        0.0, 0.0, 1.0, 0.0,
                                                                                                        0.0, 0.0, 0.0, 1.0};

                                                                                        void *memory;

                                                                                        result = vkMapMemory(device, uniform_buffer_device_memory, 0, memory_requirements.size, 0, &memory);

                                                                                        if(result == VK_SUCCESS)
                                                                                        {
                                                                                            printf("uniform buffer memory mapped successfully!\n");
                                                                                            memcpy(memory, mvp, sizeof(mvp));
                                                                                            vkUnmapMemory(device, uniform_buffer_device_memory);
                                                                                        }

                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    printf("error creating swapchain!\n");
                                                }


                                            }
                                            else
                                            {
                                                printf("no present modes available...\n");
                                            }
                                            
                                        }
                                        else
                                        {
                                            printf("error querying surface present modes!\n");
                                        }
                                    }
                                    else
                                    {
                                        printf("error querying surface formats!\n");
                                    }
                                    
                                }
                                else
                                {
                                    printf("no queue capable of presenting is available...\n");
                                }
                            }
                            else
                            {
                                printf("error creating surface!\n");
                            }
                        }
                        else
                        {
                            printf("error creating command buffer!\n");
                        }
                    }
                    else
                    {
                        printf("error creating command pool!\n");
                    }
                }
                else
                {
                    printf("error creating device!\n");
                }
            }
            else
            {
                printf("no queue family capable of graphics found...\n");
            }
        }
    }
    else
    {
        printf("No physical devices available!\n");
    }

    vkDestroyInstance(instance, NULL);
}