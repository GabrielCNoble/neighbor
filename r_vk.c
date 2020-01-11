#include "r_vk.h"
// #include "r_common.h"
#include "r_main.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "SDL\include\SDL2\SDL.h"
#include "SDL\include\SDL2\SDL_vulkan.h"
#include "SDL\include\SDL2\SDL_syswm.h"

extern struct r_renderer_t r_renderer;
struct r_vk_renderer_t r_vk_renderer;

PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSet;


VkFilter filter_map[R_TEXTURE_FILTER_LAST] = {(VkFilter)R_TEXTURE_FILTER_LAST};
VkSamplerMipmapMode mipmap_mode_map[R_TEXTURE_FILTER_LAST] = {(VkSamplerMipmapMode)R_TEXTURE_FILTER_LAST};
VkSamplerAddressMode address_mode_map[R_TEXTURE_ADDRESS_MODE_LAST] = {(VkSamplerAddressMode)R_TEXTURE_ADDRESS_MODE_LAST};

void r_vk_InitRenderer()
{
    r_renderer.textures = create_stack_list(sizeof(struct r_vk_texture_t), 32);
    r_vk_renderer.samplers = create_list(sizeof(struct r_vk_sampler_t), 16);

    filter_map[R_TEXTURE_FILTER_NEAREST] = VK_FILTER_NEAREST;
    filter_map[R_TEXTURE_FILTER_LINEAR] = VK_FILTER_LINEAR;
    mipmap_mode_map[R_TEXTURE_FILTER_NEAREST] = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    mipmap_mode_map[R_TEXTURE_FILTER_LINEAR] = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    address_mode_map[R_TEXTURE_ADDRESS_MODE_REPEAT] = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    address_mode_map[R_TEXTURE_ADDRESS_MODE_CLAMP_TO_EDGE] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    address_mode_map[R_TEXTURE_ADDRESS_MODE_CLAMP_TO_BORDER] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    // address_mode_map[R_TEXTURE_ADDRESS_MODE_MIRROR] = VK_SAMPLER_ADDRESS_MODE_MIRROR

    r_vk_InitDevice();
    // printf("device ok!\n");
    r_vk_InitSwapchain();
    // printf("swapchain ok!\n");
    r_vk_InitUniformBuffer();
    // printf("uniform buffer ok!\n");
    r_vk_InitHeap();
    // printf("heap ok!\n");
    r_vk_InitDescriptorSets();
    // printf("descriptor sets ok!\n");
    r_vk_InitCommandPool();
    // printf("command pool ok!\n");
    r_vk_InitPipeline();
    // printf("pipeline ok!\n");

    vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(r_vk_renderer.device, "vkCmdPushDescriptorSetKHR");
}

void r_vk_InitDevice()
{
    VkInstanceCreateInfo instance_create_info = {};
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    VkDeviceCreateInfo device_create_info = {};
    VkDeviceQueueCreateInfo queue_create_info = {};
    VkPhysicalDeviceMemoryProperties memory_properties = {};

    VkImageCreateInfo image_create_info;
    VkImageViewCreateInfo image_view_create_info;

    uint32_t physical_device_count = 1;

    VkQueueFamilyProperties *queue_families = NULL;
    uint32_t queue_families_count = 0;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSurfaceFormatKHR surface_format;
    uint32_t surface_format_count = 1;

    VkResult result;
    SDL_SysWMinfo wm_info;
    const char *instance_extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    // const char **instance_extensions;
    // uint32_t instance_extensions_count = 0;
    const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME};
    const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

    // SDL_Vulkan_GetInstanceExtensions(NULL, &instance_extensions_count, NULL);
    // instance_extensions = alloca(sizeof(char *) * instance_extensions_count);
    // SDL_Vulkan_GetInstanceExtensions(NULL, &instance_extensions_count, instance_extensions);

    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = NULL;
    application_info.pApplicationName = "NO ONE IS HERE";
    application_info.applicationVersion = 0;
    application_info.pEngineName = "MAYHEM";
    application_info.engineVersion = 0;
    application_info.apiVersion = VK_API_VERSION_1_1;

    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = &application_info;
    // instance_create_info.enabledLayerCount = sizeof(validation_layers) / sizeof(validation_layers[0]);
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = validation_layers;

    /* I find it a bit odd that surfaces are made available through an
    extension. If this is a graphics api, wouldn't having support for
    showing stuff on the screen be something required for it to fulfill
    its use? If this was a general purpose computing library, and doing
    graphics related operations was one of the things supported, I guess
    then it'd make sense. */
    // instance_create_info.enabledExtensionCount = instance_extensions_count;
    instance_create_info.enabledExtensionCount = 2;
    instance_create_info.ppEnabledExtensionNames = instance_extensions;
    
    result = vkCreateInstance(&instance_create_info, NULL, &r_vk_renderer.instance);

    if(result != VK_SUCCESS)
    {
        printf("%s\n", r_vk_ResultString(result));
        printf("error creating instance!\n");
        return;
    }  

    // uint32_t surface_result = SDL_Vulkan_CreateSurface(r_renderer.window, r_vk_renderer.instance, &r_vk_renderer.surface); 
    // if(!surface_result)
    // {
    //     printf("error creating surface!\n");
    //     return;
    // }

    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(r_renderer.window, &wm_info);

    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = NULL;
    surface_create_info.flags = 0;
    surface_create_info.hinstance = wm_info.info.win.hinstance;
    surface_create_info.hwnd = wm_info.info.win.window;

    result = vkCreateWin32SurfaceKHR(r_vk_renderer.instance, &surface_create_info, NULL, &r_vk_renderer.surface);

    if(result != VK_SUCCESS)
    {
        printf("error creating surface!\n");
        return;
    }

    /* will the main gpu be the first on the list? I'm assuming it will for now... */
    result = vkEnumeratePhysicalDevices(r_vk_renderer.instance, &physical_device_count, &r_vk_renderer.physical_device);

    if(result != VK_SUCCESS)
    {
        printf("error enumerating physical devices: %s!\n", r_vk_ResultString(result));
    }

    vkGetPhysicalDeviceMemoryProperties(r_vk_renderer.physical_device, &r_vk_renderer.memory_properties);
    vkGetPhysicalDeviceQueueFamilyProperties(r_vk_renderer.physical_device, &queue_families_count, NULL);

    if(queue_families_count)
    {
        queue_families = (VkQueueFamilyProperties *)alloca(sizeof(VkQueueFamilyProperties) * queue_families_count);
        vkGetPhysicalDeviceQueueFamilyProperties(r_vk_renderer.physical_device, &queue_families_count, queue_families);
        r_vk_renderer.graphics_queue_index = 0xffffffff;
        r_vk_renderer.present_queue_index = 0xffffffff;

        for(uint32_t i = 0; i < queue_families_count; i++)
        {
            if((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
                r_vk_renderer.graphics_queue_index == 0xffffffff)
            {
                r_vk_renderer.graphics_queue_index = i;
            }

            VkBool32 present_capable = 0;

            if(r_vk_renderer.present_queue_index == 0xffffffff)
            {
                result = vkGetPhysicalDeviceSurfaceSupportKHR(r_vk_renderer.physical_device, i, r_vk_renderer.surface, &present_capable);
                if(result != VK_SUCCESS)
                {
                    printf("result %s for queue family %d\n", r_vk_ResultString(result), i);
                }
                
                if(present_capable)
                {
                    r_vk_renderer.present_queue_index = i;
                }
            }
        }
    }
    float *priorities = (float *)alloca(sizeof(float) * queue_families[r_vk_renderer.graphics_queue_index].queueCount);
    /* give all queues the same priority (0.0)... */
    memset(priorities, 0, sizeof(float) * queue_families[r_vk_renderer.graphics_queue_index].queueCount);

    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pNext = NULL;
    queue_create_info.flags = 0;
    queue_create_info.queueFamilyIndex = r_vk_renderer.graphics_queue_index;
    queue_create_info.queueCount = queue_families[r_vk_renderer.graphics_queue_index].queueCount;

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
    device_create_info.enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]);
    device_create_info.ppEnabledExtensionNames = device_extensions;
    device_create_info.pEnabledFeatures = NULL;

    result = vkCreateDevice(r_vk_renderer.physical_device, &device_create_info, NULL, &r_vk_renderer.device);

    if(result != VK_SUCCESS)
    {
        printf("error creating device!\n");
        return;
    }

    vkGetDeviceQueue(r_vk_renderer.device, r_vk_renderer.graphics_queue_index, 0, &r_vk_renderer.queue);
}

void r_vk_InitSwapchain()
{
    VkResult result;
    uint32_t surface_format_count = 1;
    VkSurfaceFormatKHR surface_format;
    VkSurfaceCapabilitiesKHR surface_capabilities;

    vkGetPhysicalDeviceSurfaceFormatsKHR(r_vk_renderer.physical_device, r_vk_renderer.surface, &surface_format_count, &surface_format);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_vk_renderer.physical_device, r_vk_renderer.surface, &surface_capabilities);

    /* this pretty much works as the window backbuffer... */
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = r_vk_renderer.surface;
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

    result = vkCreateSwapchainKHR(r_vk_renderer.device, &swapchain_create_info, NULL, &r_vk_renderer.swapchain.swapchain);

    if(result != VK_SUCCESS)
    {
        printf("error creating the swapchain!\n");
        return;
    }



    vkGetSwapchainImagesKHR(r_vk_renderer.device, r_vk_renderer.swapchain.swapchain, &r_vk_renderer.swapchain.image_count, NULL);

    if(!r_vk_renderer.swapchain.image_count)
    {
        printf("swapchain has no images!\n");
        return;
    }

    r_vk_renderer.swapchain.images = (VkImage *)calloc(r_vk_renderer.swapchain.image_count, sizeof(VkImage));
    /* get the swapchain's images. Their memory has been allocated and bound by the swapchain */
    vkGetSwapchainImagesKHR(r_vk_renderer.device, r_vk_renderer.swapchain.swapchain, &r_vk_renderer.swapchain.image_count, r_vk_renderer.swapchain.images);

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

    r_vk_renderer.swapchain.image_views = (VkImageView *)calloc(r_vk_renderer.swapchain.image_count, sizeof(VkImageView));
    
    for(uint32_t i; i < r_vk_renderer.swapchain.image_count; i++)
    {
        image_view_create_info.image = r_vk_renderer.swapchain.images[i];
        vkCreateImageView(r_vk_renderer.device, &image_view_create_info, NULL, r_vk_renderer.swapchain.image_views + i);
    }

}

void r_vk_InitUniformBuffer()
{
    r_vk_CreateBuffer(&r_vk_renderer.uniform_buffer, sizeof(mat4_t), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

void r_vk_InitHeap()
{
    r_vk_CreateBuffer(&r_vk_renderer.vertex_buffer, R_HEAP_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    r_vk_CreateBuffer(&r_vk_renderer.index_buffer, R_HEAP_SIZE, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void r_vk_InitDescriptorSets()
{
    // VkDescriptorSetLayoutBinding uniform_buffer_binding = {};
    // uniform_buffer_binding.binding = 0;
    // uniform_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // uniform_buffer_binding.descriptorCount = 1;
    // uniform_buffer_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    // uniform_buffer_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding texture_sampler_bindings[R_SAMPLER_COUNT] = {};
    for(uint32_t i = 0; i < R_SAMPLER_COUNT; i++)
    {
        texture_sampler_bindings[i].binding = i;
        texture_sampler_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texture_sampler_bindings[i].descriptorCount = 1;
        texture_sampler_bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        texture_sampler_bindings[i].pImmutableSamplers = NULL;
    }

    // VkDescriptorSetLayoutCreateInfo uniform_buffer_descriptor_set_layout = {};
    // uniform_buffer_descriptor_set_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // uniform_buffer_descriptor_set_layout.pNext = NULL;
    // uniform_buffer_descriptor_set_layout.flags = 0;
    // uniform_buffer_descriptor_set_layout.bindingCount = 1;
    // uniform_buffer_descriptor_set_layout.pBindings = &uniform_buffer_binding;

    VkDescriptorSetLayoutCreateInfo texture_sampler_descriptor_set_layout = {};
    texture_sampler_descriptor_set_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texture_sampler_descriptor_set_layout.pNext = NULL;
    texture_sampler_descriptor_set_layout.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    texture_sampler_descriptor_set_layout.bindingCount = 4;
    texture_sampler_descriptor_set_layout.pBindings = texture_sampler_bindings;

    VkDescriptorSetLayout descriptor_set_layouts[2];
    // vkCreateDescriptorSetLayout(r_vk_renderer.device, &uniform_buffer_descriptor_set_layout, NULL, descriptor_set_layouts);
    vkCreateDescriptorSetLayout(r_vk_renderer.device, &texture_sampler_descriptor_set_layout, NULL, descriptor_set_layouts);


    VkPushConstantRange push_constant_ranges[2] = {};
    push_constant_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_ranges[0].size = sizeof(mat4_t) * 2;
    push_constant_ranges[0].offset = 0;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = NULL;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges;

    vkCreatePipelineLayout(r_vk_renderer.device, &pipeline_layout_create_info, NULL, &r_vk_renderer.pipeline_layout);


    VkDescriptorPoolSize descriptor_pool_sizes[2];
    // descriptor_pool_sizes[0].descriptorCount = 1;
    // descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_sizes[0].descriptorCount = 4;
    descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;


    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = NULL;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
    
    vkCreateDescriptorPool(r_vk_renderer.device, &descriptor_pool_create_info, NULL, &r_vk_renderer.descriptor_pool);


    // VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    // descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    // descriptor_set_allocate_info.pNext = NULL;
    // descriptor_set_allocate_info.descriptorPool = r_vk_renderer.descriptor_pool;
    // descriptor_set_allocate_info.descriptorSetCount = 1;
    // descriptor_set_allocate_info.pSetLayouts = descriptor_set_layouts;

    // vkAllocateDescriptorSets(r_vk_renderer.device, &descriptor_set_allocate_info, r_vk_renderer.descriptor_sets);

    // VkDescriptorBufferInfo buffer_info = {};
    // buffer_info.buffer = r_vk_renderer.uniform_buffer.buffer;
    // buffer_info.offset = 0;
    // buffer_info.range = sizeof(mat4_t);

    // VkWriteDescriptorSet uniform_buffer_write;
    // uniform_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // uniform_buffer_write.pNext = NULL;
    // uniform_buffer_write.dstSet = r_vk_renderer.descriptor_sets[0];
    // uniform_buffer_write.dstBinding = 0;
    // uniform_buffer_write.dstArrayElement = 0;
    // uniform_buffer_write.descriptorCount = 1;
    // uniform_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // uniform_buffer_write.pImageInfo = NULL;
    // uniform_buffer_write.pBufferInfo = &buffer_info;
    // uniform_buffer_write.pTexelBufferView = NULL;

    // vkUpdateDescriptorSets(r_vk_renderer.device, 1, &uniform_buffer_write, 0, NULL);
}

void r_vk_InitCommandPool()
{
    VkResult result;

    r_vk_renderer.command_state.max_command_buffer_draw_cmds = 512;

    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = NULL;
    command_pool_create_info.queueFamilyIndex = r_vk_renderer.graphics_queue_index;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    result = vkCreateCommandPool(r_vk_renderer.device, &command_pool_create_info, NULL, &r_vk_renderer.command_state.command_pool);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create a command pool!\n");
        return;
    }


    r_vk_renderer.command_state.command_buffers = (VkCommandBuffer *)calloc(sizeof(VkCommandBuffer), R_VK_AVAILABLE_CMD_BUFFERS);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = NULL;
    command_buffer_allocate_info.commandPool = r_vk_renderer.command_state.command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = R_VK_AVAILABLE_CMD_BUFFERS;

    result = vkAllocateCommandBuffers(r_vk_renderer.device, &command_buffer_allocate_info, r_vk_renderer.command_state.command_buffers);

    if(result != VK_SUCCESS)
    {
        printf("couldn't allocate command buffer!\n");
        return;
    }
}

void r_vk_InitPipeline()
{
    VkResult result;

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = NULL;
    semaphore_create_info.flags = 0;

    result = vkCreateSemaphore(r_vk_renderer.device, &semaphore_create_info, NULL, &r_vk_renderer.image_aquire_semaphore);
    
    if(result != VK_SUCCESS)
    {
        printf("couldn't create the image acquire semaphore!\n");
        return;
    }


    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0;

    result = vkCreateFence(r_vk_renderer.device, &fence_create_info, NULL, &r_vk_renderer.submit_fence);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create queue submit fence!\n");
        return;
    }


    uint32_t surface_format_count = 1;
    VkSurfaceFormatKHR surface_format;
    VkSurfaceCapabilitiesKHR surface_capabilities;

    vkGetPhysicalDeviceSurfaceFormatsKHR(r_vk_renderer.physical_device, r_vk_renderer.surface, &surface_format_count, &surface_format);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_vk_renderer.physical_device, r_vk_renderer.surface, &surface_capabilities);

    /* swapchain didn't create a depth image, so we create it here as well */
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_D32_SFLOAT;
    image_create_info.extent.width = surface_capabilities.currentExtent.width;        
    image_create_info.extent.height = surface_capabilities.currentExtent.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage depth_image;
    result = vkCreateImage(r_vk_renderer.device, &image_create_info, NULL, &depth_image);

    VkMemoryRequirements alloc_req = {};
    /* find out how much memory the depth image will require */
    vkGetImageMemoryRequirements(r_vk_renderer.device, depth_image, &alloc_req);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = alloc_req.size;
    alloc_info.memoryTypeIndex = r_MemoryTypeFromProperties(alloc_req.memoryTypeBits, 0);
    VkDeviceMemory depth_image_memory;
    result = vkAllocateMemory(r_vk_renderer.device, &alloc_info, NULL, &depth_image_memory);

    if(result != VK_SUCCESS)
    {
        printf("error allocating memory for depth buffer!\n");
        return;
    }        
    /* bind the depth image to its memory */
    result = vkBindImageMemory(r_vk_renderer.device, depth_image, depth_image_memory, (VkDeviceSize)0);

    if(result != VK_SUCCESS)
    {
        printf("error binding memory to depth image!\n");
        return;
    }

    /* same story for the depth image view */
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = NULL;
    image_view_create_info.flags = 0;
    image_view_create_info.image = depth_image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = VK_FORMAT_D32_SFLOAT;
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
    vkCreateImageView(r_vk_renderer.device, &image_view_create_info, NULL, &depth_image_view);

    

    
    /* our renderer will render to a color and a depth image. The 
    color image will be attachment 0, depth will be 1 */
    VkAttachmentDescription attachment_description[2] = {};
    attachment_description[0].flags = 0;
    attachment_description[0].format = surface_format.format;
    attachment_description[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_description[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    /* transform the image to a layout that can be used for presenting, as the 
    optimal layout most likely isn't a linear one. */
    attachment_description[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_description[1].flags = 0;
    attachment_description[1].format = VK_FORMAT_D32_SFLOAT;
    attachment_description[1].samples = VK_SAMPLE_COUNT_1_BIT;
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
    vkCreateRenderPass(r_vk_renderer.device, &render_pass_create_info, NULL, &r_vk_renderer.render_pass);

    /* a framebuffer defines a set of attachments on which operations are made. We need one 
    for each swapchain image */
    r_vk_renderer.framebuffers = (VkFramebuffer *)calloc(sizeof(VkFramebuffer), r_vk_renderer.swapchain.image_count);

    VkImageView framebuffer_attachments[2] = {};
    VkFramebufferCreateInfo framebuffer_create_info = {};

    for(uint32_t i = 0; i < r_vk_renderer.swapchain.image_count; i++)
    {
        framebuffer_attachments[0] = r_vk_renderer.swapchain.image_views[i];
        framebuffer_attachments[1] = depth_image_view;

        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = r_vk_renderer.render_pass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = framebuffer_attachments;
        framebuffer_create_info.width = surface_capabilities.currentExtent.width;
        framebuffer_create_info.height = surface_capabilities.currentExtent.height;
        framebuffer_create_info.layers = 1;

        result = vkCreateFramebuffer(r_vk_renderer.device, &framebuffer_create_info, NULL, r_vk_renderer.framebuffers + i);

        if(result != VK_SUCCESS)
        {
            printf("framebuffer creation failed!\n");
            return;
        }
    }

    // VkSamplerCreateInfo sampler_create_info = {};
    // sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // sampler_create_info.pNext = NULL;
    // sampler_create_info.flags = 0;
    // sampler_create_info.magFilter = VK_FILTER_LINEAR;
    // sampler_create_info.minFilter = VK_FILTER_LINEAR;
    // sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // sampler_create_info.mipLodBias = 0.0;
    // sampler_create_info.anisotropyEnable = VK_FALSE;
    // sampler_create_info.maxAnisotropy = 8.0;
    // sampler_create_info.compareEnable = VK_FALSE;
    // sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
    // sampler_create_info.minLod = 0;
    // sampler_create_info.maxLod = 0;
    // sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    // sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    // for(uint32_t i = 0; i < R_SAMPLER_COUNT; i++)
    // {
    //     vkCreateSampler(r_vk_renderer.device, &sampler_create_info, NULL, r_vk_renderer.samplers + i);
    // }
  
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

    result = vkCreateShaderModule(r_vk_renderer.device, &shader_module_create_info, NULL, &r_vk_renderer.vertex_shader_module);

    if(result != VK_SUCCESS)
    {
        printf("couldn't create vertex shader module!\n");
        return;
    }



    shader_module_create_info.codeSize = fragment_shader.size;
    shader_module_create_info.pCode = fragment_shader.shader;

    result = vkCreateShaderModule(r_vk_renderer.device, &shader_module_create_info, NULL, &r_vk_renderer.fragment_shader_module);

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
    shader_stage_create_info[0].module = r_vk_renderer.vertex_shader_module;
    shader_stage_create_info[0].pName = "main";
    shader_stage_create_info[0].pSpecializationInfo = NULL;

    shader_stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info[1].pNext = NULL;
    shader_stage_create_info[1].flags = 0;
    shader_stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_create_info[1].module = r_vk_renderer.fragment_shader_module;
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

    VkVertexInputAttributeDescription vertex_input_attribute_description[3] = {};
    vertex_input_attribute_description[0].location = 0;
    vertex_input_attribute_description[0].binding = 0;
    vertex_input_attribute_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attribute_description[0].offset = offsetof(struct vertex_t, position);

    vertex_input_attribute_description[1].location = 1;
    vertex_input_attribute_description[1].binding = 0;
    vertex_input_attribute_description[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attribute_description[1].offset = offsetof(struct vertex_t, normal);
    
    vertex_input_attribute_description[2].location = 2;
    vertex_input_attribute_description[2].binding = 0;
    vertex_input_attribute_description[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_input_attribute_description[2].offset = offsetof(struct vertex_t, tex_coords);

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
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 3;
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

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_vk_renderer.physical_device, r_vk_renderer.surface, &surface_capabilities);

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
    rasterization_state_create_info.depthBiasEnable = VK_TRUE;
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
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                                  VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT |
                                                  VK_COLOR_COMPONENT_A_BIT;


    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.pNext = NULL;
    color_blend_state_create_info.flags = 0;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_NO_OP;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 1.0;
    color_blend_state_create_info.blendConstants[1] = 1.0;
    color_blend_state_create_info.blendConstants[2] = 1.0;
    color_blend_state_create_info.blendConstants[3] = 1.0;

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
    graphics_pipeline_create_info.layout = r_vk_renderer.pipeline_layout;
    graphics_pipeline_create_info.renderPass = r_vk_renderer.render_pass;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = 0;


    result = vkCreateGraphicsPipelines(r_vk_renderer.device, NULL, 1, &graphics_pipeline_create_info, NULL, &r_vk_renderer.graphics_pipeline);

    if(result != VK_SUCCESS)
    {
        printf("error creating graphics pipeline!\n");
        return;
    }

    
}

void r_vk_InitExtensions()
{
    // vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(r_vk_renderer.device, "vkCmdPushDescriptorSetKHR");
}

/*
=================================================================
=================================================================
=================================================================
*/

VkResult r_vk_CreateBuffer(struct r_vk_buffer_t *buffer, uint32_t size, uint32_t usage)
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

    result = vkCreateBuffer(r_vk_renderer.device, &buffer_create_info, NULL, &buffer->buffer);

    if(result != VK_SUCCESS)
    {
        printf("r_CreateBuffer: couldn't create buffer!\n");
        return result;
    }

    VkMemoryRequirements memory_reqs;
    vkGetBufferMemoryRequirements(r_vk_renderer.device, buffer->buffer, &memory_reqs);

    VkMemoryAllocateInfo memory_alloc_info = {};
    memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc_info.pNext = NULL;
    memory_alloc_info.memoryTypeIndex = r_MemoryTypeFromProperties(memory_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    memory_alloc_info.allocationSize = memory_reqs.size;

    result = vkAllocateMemory(r_vk_renderer.device, &memory_alloc_info, NULL, &buffer->memory);

    if(result != VK_SUCCESS)
    {
        printf("r_CreateBuffer: couldn't alloc memory!\n");
        return result;
    }

    result = vkBindBufferMemory(r_vk_renderer.device, buffer->buffer, buffer->memory, 0);

    if(result != VK_SUCCESS)
    {
        printf("r_CreateBuffer: couldn't bind buffer to memory!\n");
        return result;
    }

    return result;
}

void *r_vk_MapAlloc(struct r_alloc_handle_t handle, struct r_alloc_t *alloc)
{
    VkDeviceMemory device_memory;
    void *memory;
    device_memory = handle.is_index ? r_vk_renderer.index_buffer.memory : r_vk_renderer.vertex_buffer.memory;
    vkMapMemory(r_vk_renderer.device, device_memory, alloc->start + alloc->align, alloc->size - alloc->align, 0, &memory);
    return memory;
}

void r_vk_UnmapAlloc(struct r_alloc_handle_t handle)
{
    VkDeviceMemory device_memory;
    device_memory = handle.is_index ? r_vk_renderer.index_buffer.memory : r_vk_renderer.vertex_buffer.memory;
    vkUnmapMemory(r_vk_renderer.device, device_memory);
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_InitWithDefaultTexture(struct r_vk_texture_t *texture)
{
    struct r_vk_texture_t *default_texture;
    default_texture = (struct r_vk_texture_t *)r_GetTexturePointer(R_TEXTURE_HANDLE(R_DEFAULT_TEXTURE_INDEX));
    texture->image = default_texture->image;
    texture->image_view = default_texture->image_view;
    texture->memory = default_texture->memory;
    texture->sampler_index = default_texture->sampler_index;
}

void r_vk_LoadTextureData(struct r_vk_texture_t *texture, unsigned char *pixels)
{
    /* TODO: this function needs to be more generic... */
    
    void *pointer; 
    char *texture_pixels;
    
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_create_info.extent.width = texture->base.width;
    image_create_info.extent.height = texture->base.height;
    image_create_info.extent.depth = texture->base.depth;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    vkCreateImage(r_vk_renderer.device, &image_create_info, NULL, &texture->image);

    VkMemoryRequirements memory_reqs;
    vkGetImageMemoryRequirements(r_vk_renderer.device, texture->image, &memory_reqs);

    VkMemoryAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = memory_reqs.size;
    alloc_info.memoryTypeIndex = r_MemoryTypeFromProperties(memory_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // if(alloc_info.memoryTypeIndex == 0xffffffff)
    {
        /* this memory can't be mapped, so we'll need to take
         the long route to copy the image */
    }
    // else
    {
        vkAllocateMemory(r_vk_renderer.device, &alloc_info, NULL, &texture->memory);
        vkBindImageMemory(r_vk_renderer.device, texture->image, texture->memory, 0);

        VkSubresourceLayout subresource_layout;
        VkImageSubresource subresource;
        subresource.arrayLayer = 0;
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource.mipLevel = 0;
        vkGetImageSubresourceLayout(r_vk_renderer.device, texture->image, &subresource, &subresource_layout);


        vkMapMemory(r_vk_renderer.device, texture->memory, 0, memory_reqs.size, 0, &pointer);
        texture_pixels = (char *)pointer;

        for(uint32_t y = 0; y < texture->base.height; y++)
        {
            memcpy(texture_pixels, pixels + texture->base.width * 4 * y, 4 * texture->base.width);
            texture_pixels += subresource_layout.rowPitch;
        }

        vkUnmapMemory(r_vk_renderer.device, texture->memory);
    }

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = NULL;
    image_view_create_info.flags = 0;
    image_view_create_info.image = texture->image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    image_view_create_info.subresourceRange.levelCount = 1;

    vkCreateImageView(r_vk_renderer.device, &image_view_create_info, NULL, &texture->image_view);

    r_vk_TextureSamplerIndex(texture);
}

// void r_vk_SetTexture(struct r_vk_texture_t *texture, uint32_t binding_index)
// {
//     struct r_vk_sampler_t *sampler;

//     sampler = get_list_element(&r_vk_renderer.samplers, texture->sampler_index);

//     VkDescriptorImageInfo image_info = {};
//     image_info.sampler = sampler->sampler;
//     image_info.imageView = texture->image_view;
//     image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

//     VkWriteDescriptorSet write_descriptor_set = {};
//     write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//     write_descriptor_set.pNext = NULL;
//     // write_descriptor_set.dstSet = r_vk_renderer.descriptor_sets[0];
//     write_descriptor_set.dstBinding = binding_index;
//     write_descriptor_set.descriptorCount = 1;
//     write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//     write_descriptor_set.pImageInfo = &image_info;
//     write_descriptor_set.pBufferInfo = NULL;
//     write_descriptor_set.pTexelBufferView = NULL;

//     // printf("set texture %s\n", texture->base.name); 

//     vkCmdPushDescriptorSet(r_vk_renderer.command_state.command_buffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
//         r_vk_renderer.pipeline_layout, 0, 1, &write_descriptor_set);

//     // vkUpdateDescriptorSets(r_vk_renderer.device, 1, &write_descriptor_set, 0, NULL);
// }

void r_vk_TextureSamplerIndex(struct r_vk_texture_t *texture)
{
    struct r_vk_sampler_t *sampler;
    union r_vk_sampler_tag_t tag;
    uint32_t sampler_index;

    tag.params = texture->base.sampler_params;

    for(sampler_index = 0; sampler_index < r_vk_renderer.samplers.cursor; sampler_index++)
    {
        sampler = (struct r_vk_sampler_t *)get_list_element(&r_vk_renderer.samplers, sampler_index);

        if(sampler->tag.tag == tag.tag)
        {
            break;
        }
    }

    if(sampler_index >= r_vk_renderer.samplers.cursor)
    {
        /* didn't find a suitable sampler for this texture, so create one */
        VkSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = NULL;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = filter_map[texture->base.sampler_params.mag_filter];
        sampler_create_info.minFilter = filter_map[texture->base.sampler_params.min_filter];
        sampler_create_info.mipmapMode = mipmap_mode_map[texture->base.sampler_params.mip_filter];
        sampler_create_info.addressModeU = address_mode_map[texture->base.sampler_params.addr_mode_u];
        sampler_create_info.addressModeV = address_mode_map[texture->base.sampler_params.addr_mode_v];
        sampler_create_info.addressModeW = address_mode_map[texture->base.sampler_params.addr_mode_w];
        sampler_create_info.mipLodBias = 0.0;
        sampler_create_info.anisotropyEnable = VK_FALSE;
        sampler_create_info.maxAnisotropy = 2.0;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
        sampler_create_info.minLod = 0.0;
        sampler_create_info.maxLod = 0.0;
        sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;

        sampler_index = add_list_element(&r_vk_renderer.samplers, NULL);
        sampler = (struct r_vk_sampler_t *)get_list_element(&r_vk_renderer.samplers, sampler_index);
        
        vkCreateSampler(r_vk_renderer.device, &sampler_create_info, NULL, &sampler->sampler);
        sampler->tag.params = texture->base.sampler_params;
    }

    texture->sampler_index = sampler_index;
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_vk_SetMaterial(struct r_material_t *material)
{
    struct r_vk_texture_t *texture;
    struct r_vk_sampler_t *sampler;

    texture = (struct r_vk_texture_t *)material->diffuse_texture;
    sampler = (struct r_vk_sampler_t *)get_list_element(&r_vk_renderer.samplers, texture->sampler_index);

    VkDescriptorImageInfo image_info = {};
    image_info.sampler = sampler->sampler;
    image_info.imageView = texture->image_view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    /* TODO: pre-fill most of this struct elsewhere */
    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = NULL;
    write_descriptor_set.dstBinding = R_DIFFUSE_TEXTURE_BINDING;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &image_info;
    write_descriptor_set.pBufferInfo = NULL;
    write_descriptor_set.pTexelBufferView = NULL;

    vkCmdPushDescriptorSet(r_vk_renderer.command_state.command_buffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
        r_vk_renderer.pipeline_layout, 0, 1, &write_descriptor_set);
}

/*
=================================================================
=================================================================
=================================================================
*/


void r_vk_BeginFrame()
{
    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = NULL;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = NULL;
    
    vkResetCommandBuffer(r_vk_renderer.command_state.command_buffers[0], 0);
    vkBeginCommandBuffer(r_vk_renderer.command_state.command_buffers[0], &command_buffer_begin_info);
    r_AcquireNextImage();

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_vk_renderer.physical_device, r_vk_renderer.surface, &surface_capabilities);

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.1;
    clear_values[0].color.float32[1] = 0.1;
    clear_values[0].color.float32[2] = 0.1;
    clear_values[0].color.float32[3] = 1.0;

    clear_values[1].depthStencil.depth = 1.0;

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = NULL;
    render_pass_begin_info.renderPass = r_vk_renderer.render_pass;
    render_pass_begin_info.framebuffer = r_vk_renderer.framebuffers[r_vk_renderer.current_image];
    render_pass_begin_info.renderArea.extent.width = surface_capabilities.currentExtent.width;
    render_pass_begin_info.renderArea.extent.height = surface_capabilities.currentExtent.height;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    VkDeviceSize offset = 0;

    vkCmdBeginRenderPass(r_vk_renderer.command_state.command_buffers[0], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindVertexBuffers(r_vk_renderer.command_state.command_buffers[0], 0, 1, &r_vk_renderer.vertex_buffer.buffer, &offset);
    vkCmdBindPipeline(r_vk_renderer.command_state.command_buffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, r_vk_renderer.graphics_pipeline);
}

void r_vk_PushViewMatrix(mat4_t *view_matrix)
{   
    // printf("[%f %f %f %f]\n[%f %f %f %f]\n[%f %f %f %f]\n[%f %f %f %f]\n\n", view_matrix->mcomps[0][0], 
    //                                                                          view_matrix->mcomps[0][1], 
    //                                                                          view_matrix->mcomps[0][2], 
    //                                                                          view_matrix->mcomps[0][3],
                                                                             
    //                                                                          view_matrix->mcomps[1][0], 
    //                                                                          view_matrix->mcomps[1][1], 
    //                                                                          view_matrix->mcomps[1][2], 
    //                                                                          view_matrix->mcomps[1][3],
                                                                             
    //                                                                          view_matrix->mcomps[2][0], 
    //                                                                          view_matrix->mcomps[2][1], 
    //                                                                          view_matrix->mcomps[2][2], 
    //                                                                          view_matrix->mcomps[2][3],
                                                                             
    //                                                                          view_matrix->mcomps[3][0], 
    //                                                                          view_matrix->mcomps[3][1], 
    //                                                                          view_matrix->mcomps[3][2], 
    //                                                                          view_matrix->mcomps[3][3]);


    vkCmdPushConstants(r_vk_renderer.command_state.command_buffers[0], r_vk_renderer.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 64, sizeof(mat4_t), view_matrix);
}
 
void r_vk_Draw(struct r_material_t *material, mat4_t *view_projection_matrix, struct r_draw_cmd_t *draw_cmds, uint32_t count)
{
    mat4_t model_view_projection_matrix;
    struct r_draw_cmd_t *draw_cmd;

    r_vk_SetMaterial(material);
    
    for(uint32_t i = 0; i < count; i++)
    {
        draw_cmd = draw_cmds + i;
        // mat4_t_mul(&model_view_projection_matrix, &draw_cmd->model_matrix, view_projection_matrix);
        model_view_projection_matrix = draw_cmd->model_matrix * (*view_projection_matrix);
        vkCmdPushConstants(r_vk_renderer.command_state.command_buffers[0], r_vk_renderer.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), &model_view_projection_matrix);
        vkCmdDraw(r_vk_renderer.command_state.command_buffers[0], draw_cmd->range.count, 1, draw_cmd->range.start, 0);
    }
}

void r_vk_EndFrame()
{
    vkCmdEndRenderPass(r_vk_renderer.command_state.command_buffers[0]);
    vkEndCommandBuffer(r_vk_renderer.command_state.command_buffers[0]);

    VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &r_vk_renderer.image_aquire_semaphore;
    submit_info.pWaitDstStageMask = &stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = r_vk_renderer.command_state.command_buffers;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    vkResetFences(r_vk_renderer.device, 1, &r_vk_renderer.submit_fence);
    vkQueueSubmit(r_vk_renderer.queue, 1, &submit_info, r_vk_renderer.submit_fence);
    while(vkWaitForFences(r_vk_renderer.device, 1, &r_vk_renderer.submit_fence, VK_TRUE, UINT64_MAX) == VK_TIMEOUT);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.waitSemaphoreCount = 0;
    present_info.pWaitSemaphores = NULL;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &r_vk_renderer.swapchain.swapchain;
    present_info.pImageIndices = &r_vk_renderer.current_image;
    present_info.pResults = NULL;

    vkQueuePresentKHR(r_vk_renderer.queue, &present_info);
}

uint32_t r_AcquireNextImage()
{
    vkAcquireNextImageKHR(r_vk_renderer.device, r_vk_renderer.swapchain.swapchain, UINT64_MAX, r_vk_renderer.image_aquire_semaphore, NULL, &r_vk_renderer.current_image);
    return r_vk_renderer.current_image;
}

/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t requirement)
{
    printf("memory types: %d\n", r_vk_renderer.memory_properties.memoryTypeCount);
    for(uint32_t i = 0; i < r_vk_renderer.memory_properties.memoryTypeCount; i++)
    {
        if(type_bits & 1)
        {
            printf("memory type %d has %x property flags\n", i, r_vk_renderer.memory_properties.memoryTypes[i].propertyFlags);
            if((r_vk_renderer.memory_properties.memoryTypes[i].propertyFlags & requirement) == requirement)
            {
                return i;
            }
        }

        type_bits >>= 1;
    }

    return 0xffffffff;
}

char *r_vk_ResultString(VkResult error)
{
    switch(error)
    {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FRAGMENTATION_EXT:
            return "VK_ERROR_FRAGMENTATION_EXT";
        case VK_ERROR_NOT_PERMITTED_EXT:
            return "VK_ERROR_NOT_PERMITTED_EXT";
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
            return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        default:
            return "Unknown VkResult value";
    }
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

        shader.shader = (uint32_t *)calloc(1, shader.size + 1);

        fread(shader.shader, 1, shader.size, file);
        fclose(file);
    }
    else
    {
        printf("couldn't find file %s!\n", file_name);    
    }
    
    return shader;
}

void r_vk_AdjustProjectionMatrix(mat4_t *projection_matrix)
{
    projection_matrix->comps[1][1] *= -1.0;
    projection_matrix->comps[2][2] *= 0.5;
    projection_matrix->comps[2][3] *= 0.5; 
}