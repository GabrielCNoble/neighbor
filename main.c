

#define SDL_MAIN_HANDLED
#include "SDL/include/SDL2/SDL.h"
#include "SDL/include/SDL2/SDL_syswm.h"
#include "Windows.h"

#include "r_vk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct vertex_t
{
    float x, y, z, w;
    float r, g, b, a;
};

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
    VkDeviceQueueCreateInfo queue_create_info = {};
    VkDeviceCreateInfo device_create_info = {};
    VkDevice device;
    VkCommandPoolCreateInfo command_pool_create_info = {};
    VkCommandPool command_pool;
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {}; 
    VkCommandBuffer command_buffer;
    SDL_Window *window;
    SDL_SysWMinfo info;
    HWND window_handle;
    HINSTANCE h_instance;

    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    VkSurfaceKHR surface;

    VkSwapchainCreateInfoKHR swap_chain_create_info = {};
    VkSwapchainKHR swap_chain;

    VkSurfaceFormatKHR *surface_formats;
    uint32_t surface_formats_count;

    VkSurfaceCapabilitiesKHR surface_capabilites = {};

    VkPresentModeKHR *present_modes;
    uint32_t present_modes_count;

    VkImageCreateInfo image_create_info = {};
    VkImageViewCreateInfo image_view_create_info = {};

    VkPhysicalDeviceMemoryProperties memory_properties = {};
    VkMemoryRequirements memory_requirements = {};
    VkMemoryAllocateInfo alloc_info = {};

    VkBufferCreateInfo buffer_create_info = {};
    VkBuffer buffer;
    VkDeviceMemory uniform_buffer_device_memory;
    VkDescriptorBufferInfo buffer_info = {};

    VkImage *images;
    VkImage depth_buffer_image;
    VkImageView *image_views;
    VkImageView image_view;
    VkImageView depth_buffer_image_view;
    uint32_t images_count;

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet descriptor_set;
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    VkDescriptorPool descriptor_pool;
    VkDescriptorPoolSize descriptor_pool_size = {};
    VkWriteDescriptorSet writes[1] = {};

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    VkPipelineLayout pipeline_layout;

    VkDeviceMemory depth_buffer_device_memory;

    VkAttachmentDescription attachments[2] = {};
    VkAttachmentReference color_reference = {};
    VkAttachmentReference depth_reference = {};
    VkSubpassDescription subpass_description = {};
    VkRenderPassCreateInfo render_pass_create_info = {};
    VkRenderPass render_pass;

    VkShaderModuleCreateInfo shader_module_create_info = {};
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    struct vk_shader_t vertex_shader = {};
    struct vk_shader_t fragment_shader = {};
    VkPipelineShaderStageCreateInfo shader_stage_create_info[2] = {};
    // VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info = {};
    
    
    VkDynamicState dynamic_states[VK_DYNAMIC_STATE_RANGE_SIZE];
    // uint32_t dynamic_states_count = 0;
    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info;
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info;
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info;
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info;
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info;
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state;
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info;
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info;
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info;
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info;
    VkPipeline graphics_pipeline;

    VkQueue queue;

    VkViewport viewport;
    VkRect2D scissor;

    VkPipelineStageFlags pipeline_stage_flags;
    VkSubmitInfo submit_info;
    

    VkImageView attachment_views[2];
    VkFramebufferCreateInfo framebuffer_create_info = {};
    VkFramebuffer *framebuffers;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    VkVertexInputBindingDescription attrib_binding_description = {};
    VkVertexInputAttributeDescription attrib_description[2] = {};

    VkFenceCreateInfo fence_create_info;
    VkFence fence;

    uint32_t host_visible_memory_property_index;

    struct vertex_t verts[] = 
                            {
                                {
                                    -0.5, 0.5, 0.0, 1.0,
                                    1.0, 0.0, 0.0, 1.0,
                                },

                                {
                                    -0.5, -0.5, 0.0, 1.0,
                                    1.0, 0.0, 0.0, 1.0,
                                },

                                {
                                    0.5, -0.5, 0.0, 1.0,
                                    1.0, 0.0, 0.0, 1.0,
                                },



                                {
                                    0.5, -0.5, 0.0, 1.0,
                                    1.0, 0.0, 0.0, 1.0,
                                },

                                {
                                    0.5, 0.5, 0.0, 1.0,
                                    1.0, 0.0, 0.0, 1.0,
                                },

                                {
                                    -0.5, 0.5, 0.0, 1.0,
                                    1.0, 0.0, 0.0, 1.0,
                                },
                            };

    VkClearValue clear_values[2] = {};
    VkRenderPassBeginInfo render_pass_begin_info = {};
    VkSemaphore image_aquire_semaphore;
    VkSemaphoreCreateInfo image_aquire_semaphore_create_info = {};
    VkDeviceSize offsets[1] = {};



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

                    vkGetDeviceQueue(device,  graphics_queue_index, 0, &queue);

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

                            VkCommandBufferBeginInfo command_buffer_begin_info;

                            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                            command_buffer_begin_info.pNext = NULL;
                            command_buffer_begin_info.pInheritanceInfo = NULL;
                            command_buffer_begin_info.flags = 0;

                    
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

                                        printf("%d X %d\n", surface_capabilites.currentExtent.width, surface_capabilites.currentExtent.height); 

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

                                                swap_chain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
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
                                                    framebuffers = calloc(sizeof(VkFramebuffer) * 10, images_count);

                                                    if(result == VK_SUCCESS)
                                                    {
                                                        printf("%d images created!\n", images_count);

                                                        image_views = calloc(sizeof(VkImageView) * 10, images_count);
                                                        for(uint32_t i = 0; i < images_count; i++)
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
                                                            image_create_info.imageType = VK_IMAGE_TYPE_2D;
                                                            image_create_info.format = VK_FORMAT_D16_UNORM;
                                                            image_create_info.extent.width = width;
                                                            image_create_info.extent.height = height;
                                                            image_create_info.extent.depth = 1;
                                                            image_create_info.mipLevels = 1;
                                                            image_create_info.arrayLayers = 1;
                                                            image_create_info.samples = 1;
                                                            image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                                                            image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                                                            image_create_info.queueFamilyIndexCount = 0;
                                                            image_create_info.pQueueFamilyIndices = NULL;
                                                            image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                                                            image_create_info.flags = 0;
                                                            
                                                            // image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;

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

                                                                result = vkCreateImageView(device, &image_view_create_info, NULL, &depth_buffer_image_view);

                                                                if(result == VK_SUCCESS)
                                                                {
                                                                    printf("image view for depth buffer created successfully!\n");

                                                                    vkGetImageMemoryRequirements(device, depth_buffer_image, &memory_requirements);
                                                                    vkGetPhysicalDeviceMemoryProperties(physical_devices[0], &memory_properties);

                                                                    // for(host_visible_memory_property_index = 0; host_visible_memory_property_index < memory_properties.memoryTypeCount; host_visible_memory_property_index++)
                                                                    // {
                                                                    //     if(memory_properties.memoryTypes[host_visible_memory_property_index].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                                                                    //     {
                                                                    //         break;
                                                                    //     }
                                                                    // }

                                                                    // host_visible_memory_property_index = r_GetMemoryTypeFromProperties(&memory_properties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

                                                                    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                                                                    alloc_info.pNext = NULL;
                                                                    alloc_info.allocationSize = memory_requirements.size;
                                                                    alloc_info.memoryTypeIndex = r_GetMemoryTypeFromProperties(&memory_properties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);;

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
                                                                                printf("uniform buffer created sucessfully!\n");

                                                                                buffer_info.buffer = buffer;
                                                                                buffer_info.offset = 0;
                                                                                buffer_info.range = buffer_create_info.size;

                                                                                vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

                                                                                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                                                                                alloc_info.pNext = NULL;
                                                                                alloc_info.memoryTypeIndex = r_GetMemoryTypeFromProperties(&memory_properties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
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

                                                                                            descriptor_set_layout_binding.binding = 0;
                                                                                            descriptor_set_layout_binding.descriptorCount = 1;
                                                                                            descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                                                                            descriptor_set_layout_binding.pImmutableSamplers = NULL;
                                                                                            descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                                                                                            descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                                                                                            descriptor_set_layout_create_info.pNext = NULL;
                                                                                            descriptor_set_layout_create_info.flags = 0;
                                                                                            descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;
                                                                                            descriptor_set_layout_create_info.bindingCount = 1;

                                                                                            result = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, NULL, &descriptor_set_layout);

                                                                                            if(result == VK_SUCCESS)
                                                                                            {
                                                                                                printf("descriptor set layout created successfully!\n");

                                                                                                pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                                                                                                pipeline_layout_create_info.pNext = NULL;
                                                                                                pipeline_layout_create_info.pPushConstantRanges = NULL;
                                                                                                pipeline_layout_create_info.pushConstantRangeCount = 0;
                                                                                                pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
                                                                                                pipeline_layout_create_info.setLayoutCount = 1;
                                                                                                pipeline_layout_create_info.flags = 0;

                                                                                                result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout);

                                                                                                if(result == VK_SUCCESS)
                                                                                                {
                                                                                                    printf("pipeline layout created successfully!\n");

                                                                                                    descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                                                                                    descriptor_pool_size.descriptorCount = 1;

                                                                                                    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                                                                                                    descriptor_pool_create_info.pNext = NULL;
                                                                                                    descriptor_pool_create_info.maxSets = 1;
                                                                                                    descriptor_pool_create_info.poolSizeCount = 1;
                                                                                                    descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
                                                                                                    descriptor_pool_create_info.flags = 0;

                                                                                                    result = vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &descriptor_pool);

                                                                                                    if(result == VK_SUCCESS)
                                                                                                    {
                                                                                                        printf("descriptor pool created successfully!\n");


                                                                                                        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                                                                                                        descriptor_set_allocate_info.pNext = NULL;
                                                                                                        descriptor_set_allocate_info.descriptorPool = descriptor_pool;
                                                                                                        descriptor_set_allocate_info.descriptorSetCount = 1;
                                                                                                        descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;
                                                                                                        

                                                                                                        result = vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set);

                                                                                                        if(result == VK_SUCCESS)
                                                                                                        {
                                                                                                            printf("descriptor set allocated successfully!\n");

                                                                                                            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                                                                                            writes[0].pNext = NULL;
                                                                                                            writes[0].dstSet = descriptor_set;
                                                                                                            writes[0].descriptorCount = 1;
                                                                                                            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                                                                                            writes[0].pBufferInfo = &buffer_info;
                                                                                                            writes[0].dstArrayElement = 0;
                                                                                                            writes[0].dstBinding = 0;

                                                                                                            vkUpdateDescriptorSets(device, 1, writes, 0, NULL);

                                                                                                            printf("descriptor set updated\n");

                                                                                                            attachments[0].format = surface_formats[0].format;
                                                                                                            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
                                                                                                            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                                                                                                            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                                                                                                            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                                                                                                            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                                                                                                            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                                                                                                            attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                                                                                                            attachments[0].flags = 0;

                                                                                                            attachments[1].format = VK_FORMAT_D16_UNORM;
                                                                                                            attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
                                                                                                            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                                                                                                            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                                                                                                            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                                                                                                            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                                                                                                            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                                                                                                            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                                                                                                            attachments[1].flags = 0;

                                                                                                            

                                                                                                            color_reference.attachment = 0;
                                                                                                            color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                                                                                                            depth_reference.attachment = 1;
                                                                                                            depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


                                                                                                            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                                                                                                            subpass_description.flags = 0;
                                                                                                            subpass_description.inputAttachmentCount = 0;
                                                                                                            subpass_description.pInputAttachments = NULL;
                                                                                                            subpass_description.colorAttachmentCount = 1;
                                                                                                            subpass_description.pColorAttachments = &color_reference;
                                                                                                            subpass_description.pResolveAttachments = NULL;
                                                                                                            subpass_description.pDepthStencilAttachment = &depth_reference;
                                                                                                            subpass_description.preserveAttachmentCount = 0;
                                                                                                            subpass_description.pPreserveAttachments = NULL;



                                                                                                            render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                                                                                                            render_pass_create_info.pNext = NULL;
                                                                                                            render_pass_create_info.attachmentCount = 2;
                                                                                                            render_pass_create_info.pAttachments = attachments;
                                                                                                            render_pass_create_info.subpassCount = 1;
                                                                                                            render_pass_create_info.pSubpasses = &subpass_description;
                                                                                                            render_pass_create_info.dependencyCount = 0;
                                                                                                            render_pass_create_info.pDependencies = NULL;
                                                                                                            render_pass_create_info.flags = 0;

                                                                                                            result = vkCreateRenderPass(device, &render_pass_create_info, NULL, &render_pass);

                                                                                                            if(result == VK_SUCCESS)
                                                                                                            {
                                                                                                                printf("render pass created successfully!\n");

                                                                                                                vertex_shader = r_LoadShader("shader.vert.spv");
                                                                                                                fragment_shader = r_LoadShader("shader.frag.spv");

                                                                                                                if(vertex_shader.shader != NULL)
                                                                                                                {
                                                                                                                    printf("vertex shader loaded successfully!\n");
                                                                                                                }

                                                                                                                if(fragment_shader.shader != NULL)
                                                                                                                {
                                                                                                                    printf("fragment shader loaded successfully!\n");   
                                                                                                                }

                                                                                                                shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                                                                                                                shader_module_create_info.pNext = NULL;
                                                                                                                shader_module_create_info.flags = 0;
                                                                                                                shader_module_create_info.codeSize = vertex_shader.size;
                                                                                                                shader_module_create_info.pCode = vertex_shader.shader;

                                                                                                                result = vkCreateShaderModule(device, &shader_module_create_info, NULL, &vertex_shader_module);

                                                                                                                if(result == VK_SUCCESS)
                                                                                                                {
                                                                                                                    printf("vertex shader module created successfully!\n");

                                                                                                                    shader_module_create_info.codeSize = fragment_shader.size;
                                                                                                                    shader_module_create_info.pCode = fragment_shader.shader;

                                                                                                                    result = vkCreateShaderModule(device, &shader_module_create_info, NULL, &fragment_shader_module);

                                                                                                                    if(result == VK_SUCCESS)
                                                                                                                    {
                                                                                                                        printf("fragment shader module created succesfully!\n");

                                                                                                                        shader_stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                                                                                                                        shader_stage_create_info[0].pNext = NULL;
                                                                                                                        shader_stage_create_info[0].pSpecializationInfo = NULL;
                                                                                                                        shader_stage_create_info[0].flags = 0;
                                                                                                                        shader_stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
                                                                                                                        shader_stage_create_info[0].pName = "main";
                                                                                                                        shader_stage_create_info[0].module = vertex_shader_module;


                                                                                                                        shader_stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                                                                                                                        shader_stage_create_info[1].pNext = NULL;
                                                                                                                        shader_stage_create_info[1].pSpecializationInfo = NULL;
                                                                                                                        shader_stage_create_info[1].flags = 0;
                                                                                                                        shader_stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                                                                                                                        shader_stage_create_info[1].pName = "main";
                                                                                                                        shader_stage_create_info[1].module = fragment_shader_module;


                                                                                                                        attachment_views[1] = depth_buffer_image_view;


                                                                                                                        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                                                                                                                        framebuffer_create_info.pNext = NULL;
                                                                                                                        framebuffer_create_info.renderPass = render_pass;
                                                                                                                        framebuffer_create_info.attachmentCount = 2;
                                                                                                                        framebuffer_create_info.pAttachments = attachment_views;
                                                                                                                        framebuffer_create_info.width = width;
                                                                                                                        framebuffer_create_info.height = height;
                                                                                                                        framebuffer_create_info.layers = 1;

                                                                                                                        for(uint32_t i = 0; i < images_count; i++)
                                                                                                                        {
                                                                                                                            attachment_views[0] = image_views[i];
                                                                                                                            
                                                                                                                            result = vkCreateFramebuffer(device, &framebuffer_create_info, NULL, framebuffers + i);

                                                                                                                            if(result != VK_SUCCESS)
                                                                                                                            {
                                                                                                                                printf("error creating framebuffer for image %d!\n", i);
                                                                                                                                break;
                                                                                                                            }
                                                                                                                        }

                                                                                                                        if(result == VK_SUCCESS)
                                                                                                                        {
                                                                                                                            printf("framebuffers created successfully!\n");

                                                                                                                            buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                                                                                                                            buffer_create_info.pNext = NULL;
                                                                                                                            buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                                                                                                                            buffer_create_info.size = sizeof(verts);
                                                                                                                            buffer_create_info.queueFamilyIndexCount = 0;
                                                                                                                            buffer_create_info.pQueueFamilyIndices = NULL;
                                                                                                                            buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                                                                                                                            buffer_create_info.flags = 0;

                                                                                                                            result = vkCreateBuffer(device, &buffer_create_info, NULL, &vertex_buffer);

                                                                                                                            if(result == VK_SUCCESS)
                                                                                                                            {
                                                                                                                                printf("vertex buffer created successfully!\n");
                                                                                                                                vkGetBufferMemoryRequirements(device, vertex_buffer, &memory_requirements);
                                                                                                                                
                                                                                                                                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                                                                                                                                alloc_info.pNext = NULL;
                                                                                                                                alloc_info.memoryTypeIndex = r_GetMemoryTypeFromProperties(&memory_properties, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                                                                                                                                alloc_info.allocationSize = memory_requirements.size;

                                                                                                                                result = vkAllocateMemory(device, &alloc_info, NULL, &vertex_buffer_memory);

                                                                                                                                if(result == VK_SUCCESS)
                                                                                                                                {
                                                                                                                                    printf("memory for vertex buffer allocated successfully!\n");

                                                                                                                                    result = vkMapMemory(device, vertex_buffer_memory, 0, alloc_info.allocationSize, 0, &memory);

                                                                                                                                    if(result == VK_SUCCESS)
                                                                                                                                    {
                                                                                                                                        printf("vertex buffer memory mapped successfully!\n");
                                                                                                                                        memcpy(memory, verts, sizeof(verts));
                                                                                                                                        vkUnmapMemory(device, vertex_buffer_memory);

                                                                                                                                        result = vkBindBufferMemory(device, vertex_buffer, vertex_buffer_memory, 0);

                                                                                                                                        if(result == VK_SUCCESS)
                                                                                                                                        {
                                                                                                                                            printf("vertex buffer memory bound successully!\n");

                                                                                                                                            attrib_binding_description.binding = 0;
                                                                                                                                            attrib_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                                                                                                                                            attrib_binding_description.stride = sizeof(verts[0]);

                                                                                                                                            attrib_description[0].binding = 0;
                                                                                                                                            attrib_description[0].location = 0;
                                                                                                                                            attrib_description[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                                                                                                                                            attrib_description[0].offset = 0;

                                                                                                                                            attrib_description[1].binding = 0;
                                                                                                                                            attrib_description[1].location = 1;
                                                                                                                                            attrib_description[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                                                                                                                                            attrib_description[1].offset = 16;

                                                                                                                                            clear_values[0].color.float32[0] = 0.2;
                                                                                                                                            clear_values[0].color.float32[1] = 0.2;
                                                                                                                                            clear_values[0].color.float32[2] = 0.2;
                                                                                                                                            clear_values[0].color.float32[3] = 1.0;
                                                                                                                                            clear_values[1].depthStencil.depth = 1.0;
                                                                                                                                            clear_values[1].depthStencil.stencil = 0;

                                                                                                                                            image_aquire_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                                                                                                                                            image_aquire_semaphore_create_info.pNext = NULL;
                                                                                                                                            image_aquire_semaphore_create_info.flags = 0;

                                                                                                                                            result = vkCreateSemaphore(device, &image_aquire_semaphore_create_info, NULL, &image_aquire_semaphore);

                                                                                                                                            if(result == VK_SUCCESS)
                                                                                                                                            {
                                                                                                                                                printf("image aquire semaphore created successfully!\n");
                                                                                                                                                uint32_t next_image;
                                                                                                                                                result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_aquire_semaphore, VK_NULL_HANDLE, &next_image);

                                                                                                                                                if(result == VK_SUCCESS)
                                                                                                                                                {
                                                                                                                                                    printf("image aquired!\n");

                                                                                                                                                    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                                                                                                                                                    render_pass_begin_info.pNext = NULL;
                                                                                                                                                    render_pass_begin_info.renderPass = render_pass;          
                                                                                                                                                    render_pass_begin_info.framebuffer = framebuffers[next_image];
                                                                                                                                                    render_pass_begin_info.renderArea.offset.x = 0;
                                                                                                                                                    render_pass_begin_info.renderArea.offset.y = 0;
                                                                                                                                                    render_pass_begin_info.renderArea.extent.width = width;
                                                                                                                                                    render_pass_begin_info.renderArea.extent.height = height;
                                                                                                                                                    render_pass_begin_info.clearValueCount = 2; 
                                                                                                                                                    render_pass_begin_info.pClearValues = clear_values;


                                                                                                                                                    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
                                                                                                                                                    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
                                                                                                                                                    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);
                                                                                                                                                    vkCmdEndRenderPass(command_buffer);
                                                                                                                                                    // vkEndCommandBuffer(command_buffer);

                                                                                                                                                    memset(dynamic_states, 0, sizeof(dynamic_states));

                                                                                                                                                    pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                                                                                                                                                    pipeline_dynamic_state_create_info.pNext = NULL;
                                                                                                                                                    pipeline_dynamic_state_create_info.pDynamicStates = dynamic_states;
                                                                                                                                                    pipeline_dynamic_state_create_info.dynamicStateCount = 0;

                                                                                                                                                    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                                                                                                                                                    pipeline_vertex_input_state_create_info.pNext = NULL;
                                                                                                                                                    pipeline_vertex_input_state_create_info.flags = 0;
                                                                                                                                                    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
                                                                                                                                                    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &attrib_binding_description;
                                                                                                                                                    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
                                                                                                                                                    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = attrib_description;

                                                                                                                                                    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                                                                                                                                                    pipeline_input_assembly_state_create_info.pNext = NULL;
                                                                                                                                                    pipeline_input_assembly_state_create_info.flags = 0;
                                                                                                                                                    pipeline_input_assembly_state_create_info.primitiveRestartEnable = 0;
                                                                                                                                                    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

                                                                                                                                                    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                                                                                                                                                    pipeline_rasterization_state_create_info.pNext = NULL;
                                                                                                                                                    pipeline_rasterization_state_create_info.flags = 0;
                                                                                                                                                    pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
                                                                                                                                                    pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
                                                                                                                                                    pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
                                                                                                                                                    pipeline_rasterization_state_create_info.depthClampEnable = VK_TRUE;
                                                                                                                                                    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
                                                                                                                                                    pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
                                                                                                                                                    pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0;
                                                                                                                                                    pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0;
                                                                                                                                                    pipeline_rasterization_state_create_info.lineWidth = 1.0;

                                                                                                                                                    pipeline_color_blend_attachment_state.colorWriteMask = 0xf;
                                                                                                                                                    pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
                                                                                                                                                    pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
                                                                                                                                                    pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
                                                                                                                                                    pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                                                                                                                                                    pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                                                                                                                                                    pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                                                                                                                                                    pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;


                                                                                                                                                    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                                                                                                                                                    pipeline_color_blend_state_create_info.pNext = NULL;
                                                                                                                                                    pipeline_color_blend_state_create_info.flags = 0;
                                                                                                                                                    pipeline_color_blend_state_create_info.attachmentCount = 1;
                                                                                                                                                    pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;
                                                                                                                                                    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
                                                                                                                                                    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_NO_OP;
                                                                                                                                                    pipeline_color_blend_state_create_info.blendConstants[0] = 1.0;
                                                                                                                                                    pipeline_color_blend_state_create_info.blendConstants[1] = 1.0;
                                                                                                                                                    pipeline_color_blend_state_create_info.blendConstants[2] = 1.0;
                                                                                                                                                    pipeline_color_blend_state_create_info.blendConstants[3] = 1.0;

                                                                                                                                                    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                                                                                                                                                    pipeline_viewport_state_create_info.pNext = NULL;
                                                                                                                                                    pipeline_viewport_state_create_info.flags = 0;
                                                                                                                                                    pipeline_viewport_state_create_info.viewportCount = 1;
                                                                                                                                                    dynamic_states[pipeline_dynamic_state_create_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
                                                                                                                                                    pipeline_viewport_state_create_info.scissorCount = 1;
                                                                                                                                                    dynamic_states[pipeline_dynamic_state_create_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
                                                                                                                                                    pipeline_viewport_state_create_info.pScissors = NULL;
                                                                                                                                                    pipeline_viewport_state_create_info.pViewports = NULL;

                                                                                                                                                    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.pNext = NULL;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.flags = 0;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.minDepthBounds = 0;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.maxDepthBounds = 0;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.back.compareMask = 0;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.back.reference = 0;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.back.writeMask = 0;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.back.writeMask = 0;
                                                                                                                                                    pipeline_depth_stencil_state_create_info.front = pipeline_depth_stencil_state_create_info.back;

                                                                                                                                                    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                                                                                                                                                    pipeline_multisample_state_create_info.pNext = NULL;
                                                                                                                                                    pipeline_multisample_state_create_info.flags = 0;
                                                                                                                                                    pipeline_multisample_state_create_info.pSampleMask = NULL;
                                                                                                                                                    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                                                                                                                                                    pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
                                                                                                                                                    pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
                                                                                                                                                    pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
                                                                                                                                                    pipeline_multisample_state_create_info.minSampleShading = 0.0;

                                                                                                                                                    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                                                                                                                                                    graphics_pipeline_create_info.pNext = NULL;
                                                                                                                                                    graphics_pipeline_create_info.layout = pipeline_layout;
                                                                                                                                                    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
                                                                                                                                                    graphics_pipeline_create_info.basePipelineIndex = 0;
                                                                                                                                                    graphics_pipeline_create_info.flags = 0;
                                                                                                                                                    graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
                                                                                                                                                    graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
                                                                                                                                                    graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
                                                                                                                                                    graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
                                                                                                                                                    graphics_pipeline_create_info.pTessellationState = NULL;
                                                                                                                                                    graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
                                                                                                                                                    graphics_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
                                                                                                                                                    graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
                                                                                                                                                    graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
                                                                                                                                                    graphics_pipeline_create_info.stageCount = 2;
                                                                                                                                                    graphics_pipeline_create_info.pStages = shader_stage_create_info;
                                                                                                                                                    graphics_pipeline_create_info.renderPass = render_pass;
                                                                                                                                                    graphics_pipeline_create_info.subpass = 0;

                                                                                                                                                    result = vkCreateGraphicsPipelines(device, NULL, 1, &graphics_pipeline_create_info, NULL, &graphics_pipeline);

                                                                                                                                                    if(result == VK_SUCCESS)
                                                                                                                                                    {
                                                                                                                                                        printf("graphics pipeline created successfully!\n");

                                                                                                                                                        result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_aquire_semaphore, NULL, &next_image);

                                                                                                                                                        if(result == VK_SUCCESS)
                                                                                                                                                        {
                                                                                                                                                            printf("next image acquired, ready for rendering!\n");

                                                                                                                                                            render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                                                                                                                                                            render_pass_begin_info.pNext = NULL;
                                                                                                                                                            render_pass_begin_info.renderPass = render_pass;
                                                                                                                                                            render_pass_begin_info.framebuffer = framebuffers[next_image];
                                                                                                                                                            render_pass_begin_info.renderArea.extent.width = width;
                                                                                                                                                            render_pass_begin_info.renderArea.extent.height = height;
                                                                                                                                                            render_pass_begin_info.renderArea.offset.x = 0;
                                                                                                                                                            render_pass_begin_info.renderArea.offset.y = 0;
                                                                                                                                                            render_pass_begin_info.clearValueCount = 2;
                                                                                                                                                            render_pass_begin_info.pClearValues = clear_values;

                                                                                                                                                            vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
                                                                                                                                                            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
                                                                                                                                                            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
                                                                                                                                                            vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);

                                                                                                                                                            viewport.width = width;
                                                                                                                                                            viewport.height = height;
                                                                                                                                                            viewport.x = 0;
                                                                                                                                                            viewport.y = 0;
                                                                                                                                                            viewport.minDepth = 0.0;
                                                                                                                                                            viewport.maxDepth = 1.0;

                                                                                                                                                            vkCmdSetViewport(command_buffer, 0, 1, &viewport);

                                                                                                                                                            scissor.offset.x = 0;
                                                                                                                                                            scissor.offset.y = 0;
                                                                                                                                                            scissor.extent.width = width;
                                                                                                                                                            scissor.extent.height = height;

                                                                                                                                                            vkCmdSetScissor(command_buffer, 0, 1, &scissor);

                                                                                                                                                            vkCmdDraw(command_buffer, 6, 1, 0, 0);
                                                                                                                                                            vkCmdEndRenderPass(command_buffer);

                                                                                                                                                            result = vkEndCommandBuffer(command_buffer);

                                                                                                                                                            if(result == VK_SUCCESS)
                                                                                                                                                            {
                                                                                                                                                                printf("command buffer recorded successfully!\n");

                                                                                                                                                                fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                                                                                                                                                                fence_create_info.pNext = NULL;
                                                                                                                                                                fence_create_info.flags = 0;

                                                                                                                                                                result = vkCreateFence(device, &fence_create_info, NULL, &fence);

                                                                                                                                                                if(result == VK_SUCCESS)
                                                                                                                                                                {
                                                                                                                                                                    printf("fence created successfully!\n");

                                                                                                                                                                    pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                                                                                                                                                                    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                                                                                                                                                                    submit_info.pNext = NULL;
                                                                                                                                                                    submit_info.waitSemaphoreCount = 1;
                                                                                                                                                                    submit_info.pWaitSemaphores = &image_aquire_semaphore;
                                                                                                                                                                    submit_info.pWaitDstStageMask = &pipeline_stage_flags;
                                                                                                                                                                    submit_info.commandBufferCount = 1;
                                                                                                                                                                    submit_info.pCommandBuffers = &command_buffer;
                                                                                                                                                                    submit_info.signalSemaphoreCount = 0;
                                                                                                                                                                    submit_info.pSignalSemaphores = NULL;
                                                                                                                                                                    
                                                                                                                                                                    result = vkQueueSubmit(queue, 1, &submit_info, fence);

                                                                                                                                                                    if(result == VK_SUCCESS)
                                                                                                                                                                    {
                                                                                                                                                                        printf("submission successful!\n");

                                                                                                                                                                        do
                                                                                                                                                                        {
                                                                                                                                                                            result = vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000);
                                                                                                                                                                            printf("waiting...\n");
                                                                                                                                                                        }
                                                                                                                                                                        while(result == VK_TIMEOUT);

                                                                                                                                                                        VkPresentInfoKHR present;

                                                                                                                                                                        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                                                                                                                                                                        present.pNext = NULL;
                                                                                                                                                                        present.swapchainCount = 1;
                                                                                                                                                                        present.pSwapchains = &swap_chain;
                                                                                                                                                                        present.pImageIndices = &next_image;
                                                                                                                                                                        present.pWaitSemaphores = NULL;
                                                                                                                                                                        present.waitSemaphoreCount = 0;
                                                                                                                                                                        present.pResults = NULL;

                                                                                                                                                                        result = vkQueuePresentKHR(queue, &present);

                                                                                                                                                                        if(result == VK_SUCCESS)
                                                                                                                                                                        {
                                                                                                                                                                            printf("YAY!\n");
                                                                                                                                                                            while(1)
                                                                                                                                                                            {
                                                                                                                                                                                SDL_PollEvent(NULL);
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