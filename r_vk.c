#include "r_vk.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct rBackend r_backend;


struct vertex_t vertices[] = 
{
    {{-0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
    {{-0.5, -0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
    {{0.5, -0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},

    {{0.5, -0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
    {{0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},
    {{-0.5, 0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}}
};


#define UNIFORM_BUFFER_SIZE (sizeof(struct vertex_t) * 900)


void r_InitBackend()
{
    r_backend.window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
    r_InitVkBackend();
}

// #define PHYSICAL_DEVICES_ENUMERATION_ORDER

void r_InitVkDevice()
{
    VkInstanceCreateInfo instance_create_info = {};
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    VkDeviceCreateInfo device_create_info = {};
    VkDeviceQueueCreateInfo queue_create_info = {};
    VkPhysicalDeviceMemoryProperties memory_properties = {};
    // VkPhysicalDevice physical_device;
    VkPhysicalDevice *physical_devices = NULL;

    VkImageCreateInfo image_create_info;
    VkImageViewCreateInfo image_view_create_info;

    uint32_t physical_device_count = 1;

    VkQueueFamilyProperties *queue_families = NULL;
    uint32_t queue_families_count = 0;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSurfaceFormatKHR surface_format;
    uint32_t surface_format_count = 1;

    VkImage *swapchain_images = NULL;
    VkImage depth_image;
    VkImageView depth_image_view;
    VkDeviceMemory depth_image_memory;
    VkMemoryRequirements alloc_req;
    VkMemoryAllocateInfo alloc_info;
    VkImageView *swapchain_image_views = NULL;
    uint32_t swapchain_images_count = 0;
    VkFramebufferCreateInfo framebuffer_create_info = {};
    VkRenderPassCreateInfo render_pass_create_info = {};
    VkAttachmentReference color_attachment_reference = {};
    VkAttachmentReference depth_attachment_reference = {};
    VkAttachmentDescription attachment_description[2] = {};
    VkSubpassDescription subpass_description = {};
    // VkAttachmentDescription depth_attachment_description = {};
    VkRenderPass render_pass;

    VkResult result;
    SDL_SysWMinfo wm_info;
    const char const *instance_extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    const char const *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const char const *validation_layers[] = {"VK_LAYER_LUNARG_core_validation"};

    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = NULL;
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = validation_layers;

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

    #ifdef PHYSICAL_DEVICES_ENUMERATION_ORDER 
        vkEnumeratePhysicalDevices(r_backend.vk_backend.instance, &physical_device_count, NULL);

        if(physical_device_count)
        {
            physical_devices = alloca(sizeof(VkPhysicalDevice) * physical_device_count);
            vkEnumeratePhysicalDevices(r_backend.vk_backend.instance, &physical_device_count, physical_devices);

            VkPhysicalDeviceProperties device_properties = {};
            // VkPhysicalDeviceDriverProperties driver_properties = {};
            const char *device_type = NULL;
            
            for(uint32_t i = 0; i < physical_device_count; i++)
            {
                vkGetPhysicalDeviceProperties(physical_devices[i], &device_properties);
            
                printf("api: %d.%d.%d\n", VK_VERSION_MAJOR(device_properties.apiVersion),
                                          VK_VERSION_MINOR(device_properties.apiVersion),
                                          VK_VERSION_PATCH(device_properties.apiVersion));

                printf("vendor: %d\n", device_properties.vendorID);
                printf("device: %d\n", device_properties.deviceID);

                switch(device_properties.deviceType)
                {
                    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                        device_type = "Other";
                    break;

                    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                        device_type = "Integrated GPU";
                    break;

                    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                        device_type = "Discrete GPU";
                    break;

                    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                        device_type = "Virtual GPU";
                    break;

                    case VK_PHYSICAL_DEVICE_TYPE_CPU:
                        device_type = "CPU";
                    break;

                    default:
                        device_type = "Unknown";
                    break;
                }

                printf("device type: %s\n", device_type);

                printf("%s\n", device_properties.deviceName);
            }
        }

        physical_device_count = 1;
    #endif

    /* will the main gpu be the first on the list? I'm assuming it will for now... */
    result = vkEnumeratePhysicalDevices(r_backend.vk_backend.instance, &physical_device_count, &r_backend.vk_backend.physical_device);

    if(result != VK_SUCCESS)
    {
        printf("error enumerating physical devices!\n");
    }

    vkGetPhysicalDeviceMemoryProperties(r_backend.vk_backend.physical_device, &r_backend.vk_backend.memory_properties);
    vkGetPhysicalDeviceQueueFamilyProperties(r_backend.vk_backend.physical_device, &queue_families_count, NULL);

    if(queue_families_count)
    {
        queue_families = alloca(sizeof(VkQueueFamilyProperties) * queue_families_count);
        vkGetPhysicalDeviceQueueFamilyProperties(r_backend.vk_backend.physical_device, &queue_families_count, queue_families);

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
                vkGetPhysicalDeviceSurfaceSupportKHR(r_backend.vk_backend.physical_device, i, r_backend.vk_backend.surface, &present_capable);
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

    result = vkCreateDevice(r_backend.vk_backend.physical_device, &device_create_info, NULL, &r_backend.vk_backend.device);

    if(result != VK_SUCCESS)
    {
        printf("error creating device!\n");
        return;
    }

    vkGetDeviceQueue(r_backend.vk_backend.device, r_backend.vk_backend.graphics_queue_index, 0, &r_backend.vk_backend.queue);
}

void r_InitVkRenderer()
{
    VkResult result;

    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = NULL;
    command_pool_create_info.queueFamilyIndex = r_backend.vk_backend.graphics_queue_index;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    result = vkCreateCommandPool(r_backend.vk_backend.device, &command_pool_create_info, NULL, &r_backend.vk_backend.command_pool);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create a command pool!\n");
        return;
    }

    uint32_t surface_format_count = 1;
    VkSurfaceFormatKHR surface_format;
    VkSurfaceCapabilitiesKHR surface_capabilities;

    vkGetPhysicalDeviceSurfaceFormatsKHR(r_backend.vk_backend.physical_device, r_backend.vk_backend.surface, &surface_format_count, &surface_format);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_backend.vk_backend.physical_device, r_backend.vk_backend.surface, &surface_capabilities);

    /* this pretty much works as the window backbuffer... */
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = r_backend.vk_backend.surface;
    swapchain_create_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = surface_capabilities.currentExtent;
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

    uint32_t swapchain_images_count;
    VkImage *swapchain_images;

    vkGetSwapchainImagesKHR(r_backend.vk_backend.device, r_backend.vk_backend.swapchain, &swapchain_images_count, NULL);

    if(!swapchain_images_count)
    {
        printf("swapchain has no images!\n");
        return;
    }

    swapchain_images = alloca(sizeof(VkImage) * swapchain_images_count);
    /* get the swapchain's images. Their memory has been allocated and bound by the swapchain */
    vkGetSwapchainImagesKHR(r_backend.vk_backend.device, r_backend.vk_backend.swapchain, &swapchain_images_count, swapchain_images);

    /* however, the swapchain didn't create image views, so we do it here */
    VkImageViewCreateInfo image_view_create_info = {};
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

    VkImageView *swapchain_image_views;
    swapchain_image_views = alloca(sizeof(VkImageView) * swapchain_images_count);

    for(uint32_t i; i < swapchain_images_count; i++)
    {
        image_view_create_info.image = swapchain_images[i];
        vkCreateImageView(r_backend.vk_backend.device, &image_view_create_info, NULL, swapchain_image_views + i);
    }

    /* swapchain didn't create a depth image, so we create it here as well */
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_D16_UNORM;
    image_create_info.extent.width = surface_capabilities.currentExtent.width;        
    image_create_info.extent.height = surface_capabilities.currentExtent.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = 1;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage depth_image;
    vkCreateImage(r_backend.vk_backend.device, &image_create_info, NULL, &depth_image);

    VkMemoryRequirements alloc_req;
    /* find out how much memory the depth image will require */
    vkGetImageMemoryRequirements(r_backend.vk_backend.device, depth_image, &alloc_req);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = alloc_req.size;
    alloc_info.memoryTypeIndex = r_MemoryTypeFromProperties(alloc_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    VkDeviceMemory depth_image_memory;
    result = vkAllocateMemory(r_backend.vk_backend.device, &alloc_info, NULL, &depth_image_memory);

    if(result != VK_SUCCESS)
    {
        printf("error allocating memory for depth buffer!\n");
        return;
    }        
    /* bind the depth image to its memory */
    result = vkBindImageMemory(r_backend.vk_backend.device, depth_image, depth_image_memory, (VkDeviceSize)0);

    if(result != VK_SUCCESS)
    {
        printf("error binding memory to depth image!\n");
        return;
    }

    /* same story for the depth image view */
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = NULL;
    image_view_create_info.flags = 0;
    image_view_create_info.image = depth_image;
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
    image_view_create_info.subresourceRange.layerCount = 1;

    VkImageView depth_image_view;
    vkCreateImageView(r_backend.vk_backend.device, &image_view_create_info, NULL, &depth_image_view);

    

    
    /* our renderer will render to a color and a depth image. The 
    color image will be attachment 0, depth will be 1 */
    VkAttachmentDescription attachment_description[2] = {};
    attachment_description[0].flags = 0;
    attachment_description[0].format = surface_format.format;
    attachment_description[0].samples = 1;
    attachment_description[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_description[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    /* transform the image to a layout that can be used for presenting, as the 
    optimal layout most likely isn't a linear one. */
    attachment_description[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_description[1].flags = 0;
    attachment_description[1].format = VK_FORMAT_D16_UNORM;
    attachment_description[1].samples = 1;
    attachment_description[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_description[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    /* leave the depth image on its most optimal layout, since we won't present it */
    attachment_description[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /* our renderer will have just a subpass for now, in which we
    draw the scene. */

    /* this attachment reference will reference attachment 0, the color
    attachment */
    VkAttachmentReference color_attachment_reference = {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    /* this will reference attachment 1, the depth attachment */
    VkAttachmentReference depth_attachment_reference = {};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = {};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = NULL;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.pResolveAttachments = NULL;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = NULL;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = 2;
    render_pass_create_info.pAttachments = attachment_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies = NULL;

    // VkRenderPass render_pass;
    vkCreateRenderPass(r_backend.vk_backend.device, &render_pass_create_info, NULL, &r_backend.vk_backend.render_pass);

    /* a framebuffer defines a set of attachments on which operations are made. We need one 
    for each swapchain image */
    r_backend.vk_backend.framebuffers = calloc(sizeof(VkFramebuffer), swapchain_images_count);

    VkImageView framebuffer_attachments[2] = {};
    VkFramebufferCreateInfo framebuffer_create_info = {};

    for(uint32_t i = 0; i < swapchain_images_count; i++)
    {
        framebuffer_attachments[0] = swapchain_image_views[i];
        framebuffer_attachments[1] = depth_image_view;

        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = r_backend.vk_backend.render_pass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = framebuffer_attachments;
        framebuffer_create_info.width = surface_capabilities.currentExtent.width;
        framebuffer_create_info.height = surface_capabilities.currentExtent.height;
        framebuffer_create_info.layers = 1;

        result = vkCreateFramebuffer(r_backend.vk_backend.device, &framebuffer_create_info, NULL, r_backend.vk_backend.framebuffers + i);

        if(result != VK_SUCCESS)
        {
            printf("framebuffer creation failed!\n");
            return;
        }
    }

    result = r_CreateBuffer(&r_backend.vk_backend.vertex_buffer, UNIFORM_BUFFER_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create vertex buffer!\n");
        return;
    }

    result = r_CreateBuffer(&r_backend.vk_backend.uniform_buffer, sizeof(float [16]), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create uniform buffer!\n");
        return;
    }

    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    binding.pImmutableSamplers = NULL;


    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = NULL;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = 1;
    descriptor_set_layout_create_info.pBindings = &binding;

    VkDescriptorSetLayout descriptor_set_layout;
    
    result = vkCreateDescriptorSetLayout(r_backend.vk_backend.device, &descriptor_set_layout_create_info, NULL, &descriptor_set_layout);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create descriptor set layout!\n");
        return;
    }

    VkDescriptorPoolSize descriptor_pool_size = {};
    descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = NULL;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;

    result = vkCreateDescriptorPool(r_backend.vk_backend.device, &descriptor_pool_create_info, NULL, &r_backend.vk_backend.descriptor_pool);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create a descriptor pool!\n");
        return;
    }


    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = NULL;
    descriptor_set_allocate_info.descriptorPool = r_backend.vk_backend.descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;

    result = vkAllocateDescriptorSets(r_backend.vk_backend.device, &descriptor_set_allocate_info, &r_backend.vk_backend.descriptor_set);

    if(result != VK_SUCCESS)
    {
        printf("coulndn't allocate descriptor sets!\n");
        return;
    }

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = r_backend.vk_backend.uniform_buffer.buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = sizeof(float [16]);

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = NULL;
    write_descriptor_set.dstSet = r_backend.vk_backend.descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pImageInfo = NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = NULL;

    vkUpdateDescriptorSets(r_backend.vk_backend.device, 1, &write_descriptor_set, 0, NULL);



    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = NULL;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = NULL;

    // VkPipelineLayout pipeline_layout;

    result = vkCreatePipelineLayout(r_backend.vk_backend.device, &pipeline_layout_create_info, NULL, &r_backend.vk_backend.pipeline_layout);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create pipeline layout!\n");
        return;
    }

    struct rVkShader vertex_shader;
    struct rVkShader fragment_shader;

    vertex_shader = r_LoadShader("shader.vert.spv");
    fragment_shader = r_LoadShader("shader.frag.spv");

    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext = NULL;
    shader_module_create_info.flags = 0;
    shader_module_create_info.codeSize = vertex_shader.size;
    shader_module_create_info.pCode = vertex_shader.shader;

    result = vkCreateShaderModule(r_backend.vk_backend.device, &shader_module_create_info, NULL, &r_backend.vk_backend.vertex_shader_module);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create vertex shader module!\n");
        return;
    }



    shader_module_create_info.codeSize = fragment_shader.size;
    shader_module_create_info.pCode = fragment_shader.shader;

    result = vkCreateShaderModule(r_backend.vk_backend.device, &shader_module_create_info, NULL, &r_backend.vk_backend.fragment_shader_module);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create fragment shader module!\n");
        return;
    }
    
    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkPipelineShaderStageCreateInfo shader_stage_create_info[2] = {};
    shader_stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info[0].pNext = NULL;
    shader_stage_create_info[0].flags = 0;
    shader_stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_create_info[0].module = r_backend.vk_backend.vertex_shader_module;
    shader_stage_create_info[0].pName = "main";
    shader_stage_create_info[0].pSpecializationInfo = NULL;

    shader_stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info[1].pNext = NULL;
    shader_stage_create_info[1].flags = 0;
    shader_stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_create_info[1].module = r_backend.vk_backend.fragment_shader_module;
    shader_stage_create_info[1].pName = "main";
    shader_stage_create_info[1].pSpecializationInfo = NULL;
    
    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkVertexInputBindingDescription vertex_input_binding_description = {};
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(struct vertex_t);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attribute_description[2] = {};
    vertex_input_attribute_description[0].location = 0;
    vertex_input_attribute_description[0].binding = 0;
    vertex_input_attribute_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attribute_description[0].offset = 0;

    vertex_input_attribute_description[1].location = 1;
    vertex_input_attribute_description[1].binding = 0;
    vertex_input_attribute_description[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attribute_description[1].offset = (uint32_t)&((struct vertex_t *)0)->color;

    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.pNext = NULL;
    vertex_input_state_create_info.flags = 0;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_description;

    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.pNext = NULL;
    input_assembly_state_create_info.flags = 0;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = 0;

    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_backend.vk_backend.physical_device, r_backend.vk_backend.surface, &surface_capabilities);

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = surface_capabilities.currentExtent.width;
    viewport.height = surface_capabilities.currentExtent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;


    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = surface_capabilities.currentExtent.width;
    scissor.extent.height = surface_capabilities.currentExtent.height;


    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.pNext = NULL;
    viewport_state_create_info.flags = 0;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;
    
    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.pNext = NULL;
    rasterization_state_create_info.flags = 0;
    rasterization_state_create_info.depthClampEnable = 0;
    rasterization_state_create_info.rasterizerDiscardEnable = 0;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = 0;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0;
    rasterization_state_create_info.depthBiasClamp = 0;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0;
    rasterization_state_create_info.lineWidth = 1.0;

    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.pNext = NULL;
    multisample_state_create_info.flags = 0;
    multisample_state_create_info.rasterizationSamples = 1;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.minSampleShading = 0.0;
    multisample_state_create_info.pSampleMask = NULL;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.pNext = NULL;
    depth_stencil_state_create_info.flags = 0;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front.failOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state_create_info.front.passOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state_create_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state_create_info.front.compareOp = VK_COMPARE_OP_EQUAL;
    depth_stencil_state_create_info.front.compareMask = 0;
    depth_stencil_state_create_info.front.writeMask = 0;
    depth_stencil_state_create_info.front.reference = 0;
    depth_stencil_state_create_info.back = depth_stencil_state_create_info.front;
    depth_stencil_state_create_info.minDepthBounds = 0.0;
    depth_stencil_state_create_info.maxDepthBounds = 0.0;

    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask = 0;


    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.pNext = NULL;
    color_blend_state_create_info.flags = 0;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_NO_OP;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0;
    color_blend_state_create_info.blendConstants[1] = 0.0;
    color_blend_state_create_info.blendConstants[2] = 0.0;
    color_blend_state_create_info.blendConstants[3] = 0.0;

    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext = NULL;
    dynamic_state_create_info.flags = 0;
    dynamic_state_create_info.dynamicStateCount = 0;
    dynamic_state_create_info.pDynamicStates = NULL;

    /*
    ======================================================================
    ======================================================================
    ======================================================================
    */

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = NULL;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = 2;
    graphics_pipeline_create_info.pStages = shader_stage_create_info;
    graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState = NULL;
    graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    graphics_pipeline_create_info.layout = r_backend.vk_backend.pipeline_layout;
    graphics_pipeline_create_info.renderPass = r_backend.vk_backend.render_pass;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = 0;


    result = vkCreateGraphicsPipelines(r_backend.vk_backend.device, NULL, 1, &graphics_pipeline_create_info, NULL, &r_backend.vk_backend.graphics_pipeline);

    if(result != VK_SUCCESS)
    {
        printf("error creating graphics pipeline!\n");
        return;
    }

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = NULL;
    semaphore_create_info.flags = 0;

    result = vkCreateSemaphore(r_backend.vk_backend.device, &semaphore_create_info, NULL, &r_backend.vk_backend.image_aquire_semaphore);
    
    if(result != VK_SUCCESS)
    {
        printf("couldn't create the image acquire semaphore!\n");
        return;
    }


    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0;

    result = vkCreateFence(r_backend.vk_backend.device, &fence_create_info, NULL, &r_backend.vk_backend.submit_fence);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create queue submit fence!\n");
        return;
    }


    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = NULL;
    command_buffer_allocate_info.commandPool = r_backend.vk_backend.command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(r_backend.vk_backend.device, &command_buffer_allocate_info, &r_backend.vk_backend.command_buffer);

    if(result != VK_SUCCESS)
    {
        printf("couldn't allocate command buffer!\n");
        return;
    }
}

void r_InitVkBackend()
{
    r_InitVkDevice();
    r_InitVkRenderer();
}

VkResult r_CreateBuffer(struct rVkBuffer *buffer, uint32_t size, uint32_t usage)
{
    VkResult result;

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = NULL;
    buffer_create_info.flags = 0;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = NULL;

    result = vkCreateBuffer(r_backend.vk_backend.device, &buffer_create_info, NULL, &buffer->buffer);

    if(result != VK_SUCCESS)
    {
        printf("r_CreateBuffer: couldn't create buffer!\n");
        return result;
    }

    VkMemoryRequirements memory_reqs;
    vkGetBufferMemoryRequirements(r_backend.vk_backend.device, buffer->buffer, &memory_reqs);

    VkMemoryAllocateInfo memory_alloc_info = {};
    memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc_info.pNext = NULL;
    memory_alloc_info.memoryTypeIndex = r_MemoryTypeFromProperties(memory_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    memory_alloc_info.allocationSize = memory_reqs.size;

    result = vkAllocateMemory(r_backend.vk_backend.device, &memory_alloc_info, NULL, &buffer->memory);

    if(result != VK_SUCCESS)
    {
        printf("r_CreateBuffer: couldn't alloc memory!\n");
        return result;
    }

    result = vkBindBufferMemory(r_backend.vk_backend.device, buffer->buffer, buffer->memory, 0);

    if(result != VK_SUCCESS)
    {
        printf("r_CreateBuffer: couldn't bind buffer to memory!\n");
        return result;
    }

    return result;
}


VkResult r_BeginCommandBuffer()
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;

    vkResetCommandBuffer(r_backend.vk_backend.command_buffer, 0);    
    return vkBeginCommandBuffer(r_backend.vk_backend.command_buffer, &begin_info);
}

VkResult r_EndCommandBuffer()
{
    return vkEndCommandBuffer(r_backend.vk_backend.command_buffer);
}

uint32_t r_AcquireNextImage()
{
    vkAcquireNextImageKHR(r_backend.vk_backend.device, r_backend.vk_backend.swapchain, UINT64_MAX, r_backend.vk_backend.image_aquire_semaphore, NULL, &r_backend.vk_backend.current_image);
    return r_backend.vk_backend.current_image;
}

void r_UploadData()
{
    void *memory;
    VkResult result;
    float projection_matrix[16] = 
    {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    result = vkMapMemory(r_backend.vk_backend.device, r_backend.vk_backend.uniform_buffer.memory, 0, sizeof(float [16]), 0, &memory);

    if(result != VK_SUCCESS)
    {
        printf("couldn't map memory of uniform buffer!\n");
        return;
    }

    memcpy(memory, projection_matrix, sizeof(projection_matrix));
    vkUnmapMemory(r_backend.vk_backend.device, r_backend.vk_backend.uniform_buffer.memory);


    result = vkMapMemory(r_backend.vk_backend.device, r_backend.vk_backend.vertex_buffer.memory, 0, UNIFORM_BUFFER_SIZE, 0, &memory);

    if(result != VK_SUCCESS)
    {
        printf("couldn't map memory of vertex buffer!\n");
        return;
    }

    memcpy(memory, vertices, sizeof(vertices));
    vkUnmapMemory(r_backend.vk_backend.device, r_backend.vk_backend.vertex_buffer.memory);
}

void r_BeginRenderpass()
{
    VkResult result;

    result = r_BeginCommandBuffer();

    if(result != VK_SUCCESS)
    {
        printf("couldn't begin command buffer!\n");
        return;
    }

    uint32_t next_image;

    next_image = r_AcquireNextImage();

    VkSurfaceCapabilitiesKHR surface_capabilities;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_backend.vk_backend.physical_device, r_backend.vk_backend.surface, &surface_capabilities);

    VkClearValue clear_value[2] = {};
    clear_value[0].color.float32[0] = 0.5;
    clear_value[0].color.float32[1] = 0.0;
    clear_value[0].color.float32[2] = 0.0;
    clear_value[0].color.float32[3] = 1.0;
    

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = NULL;
    render_pass_begin_info.renderPass = r_backend.vk_backend.render_pass;
    render_pass_begin_info.framebuffer = r_backend.vk_backend.framebuffers[next_image];
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = surface_capabilities.currentExtent;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_value;

    VkDeviceSize offsets = 0;

    /* bind the pipeline, vertex buffer and descriptor set */
    vkCmdBeginRenderPass(r_backend.vk_backend.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindVertexBuffers(r_backend.vk_backend.command_buffer, 0, 1, &r_backend.vk_backend.vertex_buffer.buffer, &offsets);
    vkCmdBindDescriptorSets(r_backend.vk_backend.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, r_backend.vk_backend.pipeline_layout, 0, 1, &r_backend.vk_backend.descriptor_set, 0, NULL);
    vkCmdBindPipeline(r_backend.vk_backend.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, r_backend.vk_backend.graphics_pipeline);
}

void r_Draw()
{
    vkCmdDraw(r_backend.vk_backend.command_buffer, 6, 1, 0, 0);
}

void r_EndRenderpass()
{   
    VkResult result;
    VkSubmitInfo submit_info;
    VkPipelineStageFlags wait_stage_flags;

    vkCmdEndRenderPass(r_backend.vk_backend.command_buffer);

    result = r_EndCommandBuffer();

    if(result != VK_SUCCESS)
    {
        printf("couldn't end command buffer!\n");
        return;
    }

    wait_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &r_backend.vk_backend.image_aquire_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &r_backend.vk_backend.command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    vkResetFences(r_backend.vk_backend.device, 1, &r_backend.vk_backend.submit_fence);
    result = vkQueueSubmit(r_backend.vk_backend.queue, 1, &submit_info, r_backend.vk_backend.submit_fence);
    while(vkWaitForFences(r_backend.vk_backend.device, 1, &r_backend.vk_backend.submit_fence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT);
}

void r_Present()
{
    VkResult result;

    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.waitSemaphoreCount = 0;
    present_info.pWaitSemaphores = NULL;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &r_backend.vk_backend.swapchain;
    present_info.pImageIndices = &r_backend.vk_backend.current_image;
    present_info.pResults = NULL;

    result = vkQueuePresentKHR(r_backend.vk_backend.queue, &present_info);

    if(result != VK_SUCCESS)
    {
        printf("couldn't present the shit!\n");
        return;
    }
}

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t requirement)
{
    for(uint32_t i = 0; i < r_backend.vk_backend.memory_properties.memoryTypeCount; i++)
    {
        if(type_bits & 1)
        {
            if((r_backend.vk_backend.memory_properties.memoryTypes[i].propertyFlags & requirement) == requirement)
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

struct rVkShader r_LoadShader(const char *file_name)
{
    struct rVkShader shader = {};
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