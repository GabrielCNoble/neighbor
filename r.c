#include "r.h"
#include "lib/dstuff/file/path.h"
//#include "r_vk.h"
#include "lib/dstuff/containers/stack_list.h"
#include "lib/dstuff/containers/ringbuffer.h"
#include "lib/dstuff/containers/list.h"
#include <stdlib.h>
#include <string.h>
#include "lib/stb/stb_image.h"
#include "lib/SDL/include/SDL2/SDL_thread.h"
#include "lib/SDL/include/SDL2/SDL_atomic.h"
#include "lib/SDL/include/SDL2/SDL_vulkan.h"

//struct r_renderer_t r_renderer;

struct
{
    VkPhysicalDevice physical_device;
    VkInstance instance;
    VkDevice device;
    VkSurfaceKHR surface;

    SDL_Window *window;

    struct
    {
        VkBuffer base_staging_buffer;
        VkDeviceMemory base_staging_memory;
        void *base_staging_pointer;
        uint32_t base_staging_buffer_size;
        uint32_t staging_buffer_count;
        uint32_t staging_buffer_size;
        struct r_staging_buffer_t *staging_buffers;
        struct list_t free_buffers;
        struct list_t used_buffers;
        SDL_SpinLock staging_spinlock;
    }staging;

    struct
    {
        VkCommandPool command_pool;
        struct stack_list_t command_buffers;
        struct list_t pending_command_buffers;
        SDL_SpinLock spinlock;
    }draw_command_pool;

//    VkCommandBuffer draw_command_buffer;
    VkFence draw_fence;
//    VkCommandPool transfer_command_pool;
//    VkCommandBuffer transfer_command_buffer;
    VkFence transfer_fence;
    struct r_swapchain_handle_t swapchain;
    uint32_t graphics_queue_family;
    uint32_t graphics_queue_count;
    uint32_t present_queue_family;
    uint32_t present_queue_count;
    struct r_queue_t *draw_queue;
    struct r_queue_t *transfer_queue;
    struct r_queue_t *present_queue;
    struct r_queue_t queues[3];

    struct stack_list_t images;
    struct stack_list_t image_descriptions;
    struct stack_list_t buffers;
    struct stack_list_t textures;
    struct stack_list_t framebuffers;
    struct stack_list_t render_passes;
    struct stack_list_t samplers;
    struct stack_list_t shaders;
    struct stack_list_t swapchains;
    struct stack_list_t image_heaps;
    struct stack_list_t buffer_heaps;

    struct r_heap_h texture_heap;
    struct r_heap_h buffer_heap;

    struct
    {
        uint32_t max_image_dimension_1D;
        uint32_t max_image_dimension_2D;
        uint32_t max_image_array_layers;
        uint32_t max_uniform_buffer_range;
    }limits;

    VkPhysicalDeviceLimits physical_device_limits;

}r_device;


struct
{
//    SDL_Window *window;

    float z_near;
    float z_far;
    float fov_y;

    uint32_t width;
    uint32_t height;

    struct stack_list_t allocd_blocks[2];
    struct list_t free_blocks[2];

//    struct stack_list_t textures;
    struct stack_list_t materials;
    struct stack_list_t lights;
//    struct stack_list_t framebuffers;
//    struct stack_list_t render_passes;

    SDL_SpinLock cmd_buffer_lock;
    struct ringbuffer_t cmd_buffer;
    struct ringbuffer_t cmd_buffer_data;

//    mat4_t projection_matrix;
//    mat4_t view_matrix;
//    mat4_t model_matrix;
//    mat4_t view_projection_matrix;
//    mat4_t model_view_projection_matrix;
//    uint32_t outdated_view_projection_matrix;

//    struct r_draw_cmd_buffer_t *submiting_draw_cmd_buffer;

//    struct stack_list_t pipelines;
//    struct stack_list_t shaders;
}r_renderer;

uint32_t r_supports_push_descriptors = 0;
PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSet = NULL;


void r_Init()
{
    r_InitDevice();
    r_CreateDefaultTexture();
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_InitDevice()
{
    const char **extensions = NULL;
    uint32_t extension_count = 0;
    uint32_t physical_device_count = 1;
    uint32_t queue_family_property_count;
    uint32_t queue_create_info_count;
    uint32_t present_supported;

    VkExtensionProperties *extension_properties;
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
    extension_properties = calloc(sizeof(VkExtensionProperties), extension_count);
    extensions = calloc(sizeof(char *), extension_count);
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_properties);

    for(uint32_t i = 0; i < extension_count; i++)
    {
        extensions[i] = extension_properties[i].extensionName;
    }

    const char * const layers[] = {"VK_LAYER_KHRONOS_validation"};

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.flags = 0;
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = layers;
    instance_create_info.enabledExtensionCount = extension_count;
    instance_create_info.ppEnabledExtensionNames = extensions;
    vkCreateInstance(&instance_create_info, NULL, &r_device.instance);
    free(extensions);
    free(extension_properties);

    r_device.window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, R_DEFAULT_WIDTH, R_DEFAULT_HEIGHT, SDL_WINDOW_VULKAN);

    SDL_Vulkan_CreateSurface(r_device.window, r_device.instance, &r_device.surface);

    vkEnumeratePhysicalDevices(r_device.instance, &physical_device_count, &r_device.physical_device);
    VkQueueFamilyProperties *queue_family_properties;
    vkGetPhysicalDeviceQueueFamilyProperties(r_device.physical_device, &queue_family_property_count, NULL);
    queue_family_properties = calloc(sizeof(VkQueueFamilyProperties), queue_family_property_count);
    vkGetPhysicalDeviceQueueFamilyProperties(r_device.physical_device, &queue_family_property_count, queue_family_properties);

    for(uint32_t i = 0; i < queue_family_property_count; i++)
    {
        if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            if(queue_family_properties[i].queueCount > r_device.graphics_queue_count)
            {
                r_device.graphics_queue_count = queue_family_properties[i].queueCount;
                r_device.graphics_queue_family = i;
            }
        }
    }

    free(queue_family_properties);

    for(uint32_t i = 0; i < queue_family_property_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(r_device.physical_device, i, r_device.surface, &present_supported);
        if(present_supported)
        {
            r_device.present_queue_family = i;
            break;
        }
    }

    VkDeviceQueueCreateInfo *queue_create_info;
    queue_create_info_count = 1 + (r_device.graphics_queue_family != r_device.present_queue_family);
    queue_create_info = calloc(sizeof(VkDeviceQueueCreateInfo), queue_create_info_count);
    float *queue_priorities = calloc(sizeof(float), r_device.graphics_queue_count);

    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].pNext = NULL;
    queue_create_info[0].flags = 0;
    /* all the queues means all the pleasure... */
    queue_create_info[0].queueCount = r_device.graphics_queue_count;
    queue_create_info[0].queueFamilyIndex = r_device.graphics_queue_family;
    queue_create_info[0].pQueuePriorities = queue_priorities;

    if(queue_create_info_count > 1)
    {
        queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[1].pNext = NULL;
        queue_create_info[1].flags = 0;
        /* a single queue for presenting is enough */
        queue_create_info[1].queueCount = 1;
        queue_create_info[1].queueFamilyIndex = r_device.present_queue_family;
        queue_create_info[1].pQueuePriorities = queue_priorities;
    }

    vkEnumerateDeviceExtensionProperties(r_device.physical_device, NULL, &extension_count, NULL);
    extensions = calloc(sizeof(char **), extension_count);
    extension_properties = calloc(sizeof(VkExtensionProperties), extension_count);
    vkEnumerateDeviceExtensionProperties(r_device.physical_device, NULL, &extension_count, extension_properties);

    for(uint32_t i = 0; i < extension_count; i++)
    {
        extensions[i] = extension_properties[i].extensionName;
        if(!strcmp(extension_properties[i].extensionName, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME))
        {
            r_supports_push_descriptors = 1;
        }
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = NULL;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = queue_create_info_count;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.enabledExtensionCount = extension_count;
    device_create_info.ppEnabledExtensionNames = extensions;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = NULL;
    device_create_info.pEnabledFeatures = NULL;
    vkCreateDevice(r_device.physical_device, &device_create_info, NULL, &r_device.device);

    free(extensions);
    free(extension_properties);
    free(queue_create_info);
    free(queue_priorities);


    vkGetDeviceQueue(r_device.device, r_device.graphics_queue_family, 0, &r_device.queues[0].queue);
    r_device.draw_queue = &r_device.queues[0];

    if(r_device.graphics_queue_count > 1)
    {
        vkGetDeviceQueue(r_device.device, r_device.graphics_queue_family, 0, &r_device.queues[1].queue);
        r_device.transfer_queue = &r_device.queues[1];
    }
    else
    {
        r_device.transfer_queue = r_device.draw_queue;
    }

    if(r_device.graphics_queue_family != r_device.present_queue_family || r_device.graphics_queue_count > 2)
    {
        vkGetDeviceQueue(r_device.device, r_device.present_queue_family, 0, &r_device.queues[2].queue);
        r_device.present_queue = &r_device.queues[2];
    }
    else
    {
        r_device.present_queue = r_device.draw_queue;
    }

    if(r_supports_push_descriptors)
    {
        vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(r_device.device, "vkCmdPushDescriptorSetKHR");
    }

//    VkFenceCreateInfo fence_create_info = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
//    vkCreateFence(r_device.device, &fence_create_info, NULL, &r_device.draw_fence);
//    vkCreateFence(r_device.device, &fence_create_info, NULL, &r_device.transfer_fence);

    r_device.transfer_fence = r_CreateFence();

    /*
    =================================================================
    =================================================================
    =================================================================
    */

    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = NULL;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = r_device.graphics_queue_family;
    vkCreateCommandPool(r_device.device, &command_pool_create_info, NULL, &r_device.draw_command_pool.command_pool);
    r_device.draw_command_pool.command_buffers = create_stack_list(sizeof(struct r_command_buffer_t), 128);
    r_device.draw_command_pool.pending_command_buffers = create_list(sizeof(uint32_t), 128);

    /*
    =================================================================
    =================================================================
    =================================================================
    */
    VkMemoryAllocateInfo memory_allocate_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    VkMemoryRequirements memory_requirements;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    r_device.staging.base_staging_buffer_size = 33554432;

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = r_device.staging.base_staging_buffer_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(r_device.device, &buffer_create_info, NULL, &r_device.staging.base_staging_buffer);
    vkGetBufferMemoryRequirements(r_device.device, r_device.staging.base_staging_buffer, &memory_requirements);
    memory_allocate_info.memoryTypeIndex = r_MemoryIndexWithProperties(memory_requirements.memoryTypeBits, properties);
    memory_allocate_info.allocationSize = memory_requirements.size;
    vkAllocateMemory(r_device.device, &memory_allocate_info, NULL, &r_device.staging.base_staging_memory);
    vkBindBufferMemory(r_device.device, r_device.staging.base_staging_buffer, r_device.staging.base_staging_memory, 0);
    vkMapMemory(r_device.device, r_device.staging.base_staging_memory, 0, buffer_create_info.size, 0, &r_device.staging.base_staging_pointer);
    r_device.staging.staging_buffer_count = 8;
    r_device.staging.staging_buffer_size = r_device.staging.base_staging_buffer_size / r_device.staging.staging_buffer_count;
    r_device.staging.staging_buffers = calloc(sizeof(struct r_staging_buffer_t), r_device.staging.staging_buffer_count);
    r_device.staging.free_buffers = create_list(sizeof(uint32_t ), r_device.staging.staging_buffer_count);
    r_device.staging.used_buffers = create_list(sizeof(uint32_t ), r_device.staging.staging_buffer_count);
    buffer_create_info.size /= r_device.staging.staging_buffer_count;

    for(uint32_t staging_buffer_index = 0; staging_buffer_index < r_device.staging.staging_buffer_count; staging_buffer_index++)
    {
        struct r_staging_buffer_t *staging_buffer = r_device.staging.staging_buffers + staging_buffer_index;
        staging_buffer->complete_event = r_CreateEvent();
        staging_buffer->offset = r_device.staging.staging_buffer_size * staging_buffer_index;
        staging_buffer->memory = (char *)r_device.staging.base_staging_pointer + staging_buffer->offset;
        vkCreateBuffer(r_device.device, &buffer_create_info, NULL, &staging_buffer->buffer);
        vkBindBufferMemory(r_device.device, staging_buffer->buffer, r_device.staging.base_staging_memory, staging_buffer->offset);
        add_list_element(&r_device.staging.free_buffers, &staging_buffer_index);
    }

    r_device.textures = create_stack_list(sizeof(struct r_texture_t), 512);
    r_device.images = create_stack_list(sizeof(struct r_image_t), 512);
    r_device.image_descriptions = create_stack_list(sizeof(VkImageCreateInfo), 512);
    r_device.buffers = create_stack_list(sizeof(struct r_buffer_t), 512);
    r_device.framebuffers = create_stack_list(sizeof(struct r_framebuffer_t), 16);
    r_device.samplers = create_stack_list(sizeof(struct r_sampler_t), 16);
    r_device.shaders = create_stack_list(sizeof(struct r_shader_t), 128);
    r_device.render_passes = create_stack_list(sizeof(struct r_render_pass_t), 16);
    r_device.swapchains = create_stack_list(sizeof(struct r_swapchain_t), 4);
    r_device.image_heaps = create_stack_list(sizeof(struct r_image_heap_t), 16);
    r_device.buffer_heaps = create_stack_list(sizeof(struct r_buffer_heap_t), 16);

    VkFormat *formats = (VkFormat []){
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    r_device.buffer_heap = r_CreateBufferHeap(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 33554432);
    r_device.texture_heap = r_CreateImageHeap(formats, 4, 67108864);

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(r_device.physical_device, &physical_device_properties);
    r_device.physical_device_limits = physical_device_properties.limits;

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.surface = r_device.surface;
    swapchain_create_info.imageExtent.width = R_DEFAULT_WIDTH;
    swapchain_create_info.imageExtent.height = R_DEFAULT_HEIGHT;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    r_device.swapchain = r_CreateSwapchain(&swapchain_create_info);
}

void r_Shutdown()
{

}

/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_MemoryIndexWithProperties(uint32_t type_bits, uint32_t properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(r_device.physical_device, &memory_properties);

    for(uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if(type_bits & 1)
        {
            if((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        type_bits >>= 1;
    }

    return 0xffffffff;
}

uint32_t r_IsDepthStencilFormat(VkFormat format)
{
    switch(format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return 1;
    }

    return 0;
}

uint32_t r_ImageUsageFromFormat(VkFormat format)
{
    switch(format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}

VkImageAspectFlagBits r_GetFormatAspectFlags(VkFormat format)
{
    VkImageAspectFlagBits flags;

    switch(format)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
            flags = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;

        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;

        default:
            flags = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    }

    return flags;
}

uint32_t r_GetFormatPixelPitch(VkFormat format)
{
    switch(format)
    {
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
            return 3;

        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;
    }

    return 0;
}

/*
=================================================================
=================================================================
=================================================================
*/


struct stack_list_t *r_GetHeapListFromType(uint32_t type)
{
    switch(type)
    {
        case R_HEAP_TYPE_IMAGE:
            return &r_device.image_heaps;
        case R_HEAP_TYPE_BUFFER:
            return &r_device.buffer_heaps;
    }

    return NULL;
}

struct r_heap_h r_CreateHeap(uint32_t size, uint32_t type)
{
    struct stack_list_t *heap_list;
    struct r_heap_h handle = R_INVALID_HEAP_HANDLE;
    struct r_heap_t *heap;

    if(size)
    {
        heap_list = r_GetHeapListFromType(type);

        handle.index = add_stack_list_element(heap_list, NULL);
        handle.type = type;
        heap = get_stack_list_element(heap_list, handle.index);

        heap->alloc_chunks = create_stack_list(sizeof(struct r_chunk_t), 128);
        heap->free_chunks = create_list(sizeof(struct r_chunk_t), 128);
        heap->size = size;
        heap->type = type;

        add_list_element(&heap->free_chunks, &(struct r_chunk_t){.size = size, .start = 0});
    }

    return handle;
}

struct r_heap_h r_CreateImageHeap(VkFormat *formats, uint32_t format_count, uint32_t size)
{
    struct r_heap_h handle;
    struct r_image_heap_t *heap;
    VkImage dummy_image;
    VkImageCreateInfo dummy_image_create_info = {};
    VkMemoryRequirements memory_requirements;
    VkMemoryAllocateInfo allocate_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    VkFormat *supported_formats;
    uint32_t supported_formats_count = 0;
    uint32_t memory_type_bits = 0xffffffff;
    uint32_t memory_type_bits_copy;

    supported_formats = alloca(sizeof(VkFormat) * format_count);

    dummy_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    dummy_image_create_info.imageType = VK_IMAGE_TYPE_2D;
    dummy_image_create_info.extent.width = 1;
    dummy_image_create_info.extent.height = 1;
    dummy_image_create_info.extent.depth = 1;
    dummy_image_create_info.mipLevels = 1;
    dummy_image_create_info.arrayLayers = 1;
    dummy_image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    dummy_image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    dummy_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for(uint32_t i = 0; i < format_count; i++)
    {
        /* there has to be a better way for of doing this... */
        dummy_image_create_info.format = formats[i];
        dummy_image_create_info.usage = r_ImageUsageFromFormat(formats[i]);
        dummy_image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        vkCreateImage(r_device.device, &dummy_image_create_info, NULL, &dummy_image);
        vkGetImageMemoryRequirements(r_device.device, dummy_image, &memory_requirements);
        memory_type_bits_copy = memory_type_bits;
        memory_type_bits_copy &= memory_requirements.memoryTypeBits;

        if(memory_type_bits_copy)
        {
           supported_formats[supported_formats_count] = formats[i];
           supported_formats_count++;
           memory_type_bits = memory_type_bits_copy;
        }

        vkDestroyImage(r_device.device, dummy_image, NULL);
    }

    allocate_info.allocationSize = size;
    allocate_info.memoryTypeIndex = r_MemoryIndexWithProperties(memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    handle = r_CreateHeap(size, R_HEAP_TYPE_IMAGE);
    heap = (struct r_image_heap_t *)r_GetHeapPointer(handle);
    heap->supported_formats = calloc(sizeof(VkFormat), supported_formats_count);
    heap->supported_format_count = supported_formats_count;
    memcpy(heap->supported_formats, supported_formats, sizeof(VkFormat) * supported_formats_count);

    vkAllocateMemory(r_device.device, &allocate_info, NULL, &heap->memory);

    return handle;
}

struct r_heap_h r_CreateBufferHeap(uint32_t usage, uint32_t size)
{
    VkBufferCreateInfo buffer_create_info = {};
//    VkBuffer dummy_buffer;
    VkMemoryRequirements memory_requirements;
    VkMemoryAllocateInfo allocate_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    struct r_heap_h handle = R_INVALID_HEAP_HANDLE;
    struct r_buffer_heap_t *heap;
    uint32_t mapable_memory = 1;

    handle = r_CreateHeap(size, R_HEAP_TYPE_BUFFER);
    heap = (struct r_buffer_heap_t *)r_GetHeapPointer(handle);

    heap->mapped_memory = NULL;

    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(r_device.device, &buffer_create_info, NULL, &heap->buffer);
    vkGetBufferMemoryRequirements(r_device.device, heap->buffer, &memory_requirements);
    allocate_info.memoryTypeIndex = r_MemoryIndexWithProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                                                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    allocate_info.allocationSize = size;

    if(allocate_info.memoryTypeIndex == 0xffffffff)
    {
        mapable_memory = 0;
        allocate_info.memoryTypeIndex = r_MemoryIndexWithProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    vkAllocateMemory(r_device.device, &allocate_info, NULL, &heap->memory);
    vkBindBufferMemory(r_device.device, heap->buffer, heap->memory, 0);

    if(mapable_memory)
    {
        vkMapMemory(r_device.device, heap->memory, 0, size, 0, &heap->mapped_memory);
    }

    return handle;
}

void r_DestroyHeap(struct r_heap_h handle)
{
    struct r_heap_t *heap;
    struct stack_list_t *heap_list;
    heap = r_GetHeapPointer(handle);
    heap_list = r_GetHeapListFromType(handle.type);

    if(heap)
    {
        destroy_list(&heap->free_chunks);
        destroy_stack_list(&heap->alloc_chunks);
        heap->size = 0;
        remove_stack_list_element(heap_list, handle.index);
    }
}

struct r_heap_t *r_GetHeapPointer(struct r_heap_h handle)
{
    struct r_heap_t *heap = NULL;
    struct stack_list_t *heap_list;

    heap_list = r_GetHeapListFromType(handle.type);
    if(heap_list)
    {
        heap = get_stack_list_element(heap_list, handle.index);
        if(heap && !heap->size)
        {
            heap = NULL;
        }
    }

    return heap;
}

void r_DefragHeap(struct r_heap_h handle, uint32_t move_allocs)
{

}

struct r_chunk_h r_AllocChunk(struct r_heap_h handle, uint32_t size, uint32_t align)
{
    struct r_heap_t *heap;
    struct r_chunk_t *chunk;
    struct r_chunk_t new_chunk;
    struct r_chunk_h chunk_handle = R_INVALID_CHUNK_HANDLE;
    uint32_t chunk_align;
    uint32_t chunk_size;
//    uint32_t chunk_start;

    heap = r_GetHeapPointer(handle);

    SDL_AtomicLock(&heap->spinlock);

    for(uint32_t chunk_index = 0; chunk_index < heap->free_chunks.cursor; chunk_index++)
    {
        chunk = get_list_element(&heap->free_chunks, chunk_index);
        chunk_align = 0;

        if(chunk->start % align)
        {
            chunk_align = align - chunk->start % align;
        }

        /* usable size after taking alignment into consideration */
        chunk_size = chunk->size - chunk_align;

        if(chunk_size >= size)
        {
            if(chunk_size == size)
            {
                /* taking alignment into consideration this chunk has the exact size
                of the required allocation */

                chunk->align = chunk_align;
                /* the start of the chunk will move chunk_align bytes forward */
                chunk->start += chunk_align;
                /* the chunk will 'shrink', since its start point moved forward by
                chunk_align bytes, the usable space will decrease by chunk_align bytes */
                chunk->size = chunk_size;

                /* this chunk fits the request perfectly, so move it from the free list
                to the alloc list */
                chunk_handle.index = add_stack_list_element(&heap->alloc_chunks, chunk);
                remove_list_element(&heap->free_chunks, chunk_index);
            }
            else
            {
                /* this chunk is bigger even after accounting for alignment, so chop off the
                beginning part of this chunk, and adjust its size accordingly */



                new_chunk.align = chunk_align;
                new_chunk.start = chunk->start + chunk_align;
                /* since this chunk's size is bigger even after adjusting the start point,
                the allocated chunk size remains the same */
                new_chunk.size = size;

                chunk_handle.index = add_stack_list_element(&heap->alloc_chunks, &new_chunk);
                /* new_chunk start moved forward chunk_align bytes, which means its
                end will move forward chunk_align bytes */
                chunk->start += size + chunk_align;
                /* also, since its end moved forward chunk_align bytes, the sliced chunk's
                usable size also shrinks by chunk_align bytes */
                chunk->size -= size + chunk_align;
            }

            break;
        }
    }

    chunk_handle.heap = handle;

    SDL_AtomicUnlock(&heap->spinlock);

    return chunk_handle;
}

void r_FreeChunk(struct r_chunk_h handle)
{
    struct r_heap_t *heap = NULL;
    struct r_chunk_t *chunk = NULL;

    heap = r_GetHeapPointer(handle.heap);
    chunk = r_GetChunkPointer(handle);

    if(heap && chunk)
    {
        add_list_element(&heap->free_chunks, chunk);
        remove_stack_list_element(&heap->alloc_chunks, handle.index);
        chunk->size = 0;
    }
}

struct r_chunk_t *r_GetChunkPointer(struct r_chunk_h handle)
{
    struct r_heap_t *heap = NULL;
    struct r_chunk_t *chunk = NULL;

    heap = r_GetHeapPointer(handle.heap);

    if(heap)
    {
        chunk = get_stack_list_element(&heap->alloc_chunks, handle.index);
        if(chunk && !chunk->size)
        {
            chunk = NULL;
        }
    }

    return chunk;
}

void *r_GetBufferChunkMappedMemory(struct r_chunk_h handle)
{
    struct r_buffer_heap_t *heap;
    struct r_chunk_t *chunk;
    void *memory = NULL;

    if(handle.heap.type == R_HEAP_TYPE_BUFFER)
    {
        heap = (struct r_buffer_heap_t *)r_GetHeapPointer(handle.heap);
        chunk = r_GetChunkPointer(handle);

        if(heap->mapped_memory)
        {
            memory = (char *)heap->mapped_memory + chunk->start;
        }
    }

    return memory;
}

void r_FillImageChunk(struct r_image_handle_t handle, void *data, VkBufferImageCopy *copy)
{
    VkImageCreateInfo *description;
    struct r_image_t *image;
    struct r_staging_buffer_t *staging_buffer;
    VkBufferImageCopy *regions;
    VkBufferImageCopy *region;
    union r_command_buffer_h command_buffer;
    struct r_submit_info_t submit_info = {};

    uint32_t data_row_pitch;
    uint32_t region_height;
//    uint32_t pixel_pitch;
    uint32_t region_count;

    description = r_GetImageDescriptionPointer(handle);
    image = r_GetImagePointer(handle);

    if(!copy)
    {
        copy = alloca(sizeof(VkBufferImageCopy));

        copy->bufferOffset = 0;
        copy->bufferRowLength = 0;
        copy->bufferImageHeight = 0;
        copy->imageSubresource.aspectMask = image->aspect_mask;
        copy->imageSubresource.baseArrayLayer = 0;
        copy->imageSubresource.layerCount = 1;
        copy->imageSubresource.mipLevel = 0;
        copy->imageOffset.x = 0;
        copy->imageOffset.y = 0;
        copy->imageOffset.z = 0;
        copy->imageExtent.width = description->extent.width;
        copy->imageExtent.height = description->extent.height;
        copy->imageExtent.depth = 1;
    }

    data_row_pitch = copy->imageExtent.width * r_GetFormatPixelPitch(description->format);
    region_height = r_device.staging.staging_buffer_size / data_row_pitch;
    region_count = copy->imageExtent.height / region_height;

    if(!region_count)
    {
        regions = copy;
        region_count = 1;
    }
    else
    {
        regions = alloca(sizeof(VkImageBlit) * region_count);
        region = regions;
        uint32_t region_index;

        for(region_index = 0; region_index < region_count - 1; region_index++, region++)
        {
            region->bufferOffset = 0;
            region->bufferRowLength = 0;
            region->bufferImageHeight = 0;
            region->imageSubresource.aspectMask = image->aspect_mask;
            region->imageSubresource.baseArrayLayer = 0;
            region->imageSubresource.layerCount = 1;
            region->imageSubresource.mipLevel = 0;
            region->imageOffset.x = 0;
            region->imageOffset.y = region_height * region_index;
            region->imageOffset.z = 0;
            region->imageExtent.width = description->extent.width;
            region->imageExtent.height = region_height;
            region->imageExtent.depth = 1;
        }

        region->bufferOffset = 0;
        region->bufferRowLength = 0;
        region->bufferImageHeight = 0;
        region->imageSubresource.aspectMask = image->aspect_mask;
        region->imageSubresource.baseArrayLayer = 0;
        region->imageSubresource.layerCount = 1;
        region->imageSubresource.mipLevel = 0;
        region->imageOffset.x = 0;
        region->imageOffset.y = region_height * region_index;
        region->imageOffset.z = 0;
        region->imageExtent.width = description->extent.width;
        region->imageExtent.height = description->extent.height - region->imageOffset.y;
        region->imageExtent.depth = 1;
    }

//    uint32_t old_layout = image->current_layout;
    uint32_t region_index = 0;
    uint32_t first_region = 0;

    command_buffer = r_AllocateCommandBuffer();

    submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.command_buffer_count = 1;
    submit_info.command_buffers = &command_buffer;

    do
    {
        r_vkBeginCommandBuffer(command_buffer);
        if(!region_index)
        {
            r_vkCmdSetImageLayout(command_buffer, handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        }

        while(region_index < region_count)
        {
            /* if we happen to run out of staging buffers, but have uncommitted work, we'll break out,
            commit the work, then come back here. After committing the work we'll spin in here until
            a buffer becomes available. This may take a while if other threads are also uploading */
            staging_buffer = r_AllocateStagingBuffer(command_buffer);
            if(staging_buffer)
            {
                /* got a staging buffer, so copy some data */
                region = regions + region_index;
                region->bufferOffset = staging_buffer->offset;
                uint32_t data_offset = region->imageOffset.y * data_row_pitch;
                uint32_t copy_size = region->imageExtent.height * data_row_pitch;
                memcpy(staging_buffer->memory, (char *)data + data_offset, copy_size);
                region_index++;
                continue;
            }

            if(first_region < region_index)
            {
                /* didn't get a staging buffer, but we have some work we can dispatch, so break out and dispatch it */
                break;
            }
        }

        r_vkCmdCopyBufferToImage(command_buffer, r_device.staging.base_staging_buffer, image->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_index - first_region, regions + first_region);
        r_vkEndCommandBuffer(command_buffer);
        SDL_AtomicLock(&r_device.transfer_queue->spinlock);
        vkResetFences(r_device.device, 1, &r_device.transfer_fence);
        r_vkQueueSubmit(r_device.transfer_queue->queue, 1, &submit_info, r_device.transfer_fence);
        vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);
        SDL_AtomicUnlock(&r_device.transfer_queue->spinlock);
        /* next copy will start at the first region after the end of this range */
        first_region = region_index;
    }
    while(region_index < region_count);
}

void r_FillBufferChunk(struct r_buffer_h handle, void *data, uint32_t size, uint32_t offset)
{
    struct r_buffer_t *buffer;
//    struct r_chunk_t *chunk;
    struct r_buffer_heap_t *heap;
//    struct r_staging_buffer_t *staging_buffer;
//    VkBufferCreateInfo copy_buffer_create_info = {};
//    VkBuffer copy_buffer;
//    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    struct r_submit_info_t submit_info = {};
//    VkBufferCopy buffer_copy;
    union r_command_buffer_h command_buffer;

    buffer = r_GetBufferPointer(handle);
    heap = (struct r_buffer_heap_t *)r_GetHeapPointer(buffer->memory.heap);
    if(heap->mapped_memory)
    {
        memcpy((char *)r_GetBufferChunkMappedMemory(buffer->memory) + offset, data, size);
    }
    else
    {
        command_buffer = r_AllocateCommandBuffer();
        submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.command_buffer_count = 1;
        submit_info.command_buffers = &command_buffer;

        SDL_AtomicLock(&r_device.transfer_queue->spinlock);
        r_vkBeginCommandBuffer(command_buffer);
        r_vkCmdUpdateBuffer(command_buffer, handle, offset, size, data);
        r_vkEndCommandBuffer(command_buffer);
        vkResetFences(r_device.device, 1, &r_device.transfer_fence);
        r_vkQueueSubmit(r_device.transfer_queue->queue, 1, &submit_info, r_device.transfer_fence);
        vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);
        SDL_AtomicUnlock(&r_device.transfer_queue->spinlock);
    }
}

struct r_staging_buffer_t *r_AllocateStagingBuffer(union r_command_buffer_h command_buffer)
{
    VkResult result;
    struct r_staging_buffer_t *buffer = NULL;
    uint32_t *buffer_index;
//    struct r_command_buffer_t *command_buffer_ptr;

    SDL_AtomicLock(&r_device.staging.staging_spinlock);
    for(uint32_t index = 0; index < r_device.staging.used_buffers.cursor; index++)
    {
        buffer_index = get_list_element(&r_device.staging.used_buffers, index);
        buffer = r_device.staging.staging_buffers + *buffer_index;
        result = vkGetEventStatus(r_device.device, buffer->complete_event);

        if(result == VK_EVENT_SET)
        {
            vkResetEvent(r_device.device, buffer->complete_event);
            add_list_element(&r_device.staging.free_buffers, buffer_index);
            remove_list_element(&r_device.staging.used_buffers, index);
        }
    }
    buffer_index = get_list_element(&r_device.staging.free_buffers, 0);
    if(buffer_index)
    {
        buffer = r_device.staging.staging_buffers + *buffer_index;
        r_AppendEvent(command_buffer, buffer->complete_event);
        add_list_element(&r_device.staging.used_buffers, buffer_index);
        remove_list_element(&r_device.staging.free_buffers, 0);
    }

    SDL_AtomicUnlock(&r_device.staging.staging_spinlock);
    return buffer;
}

void r_FreeStagingBuffer(struct r_staging_buffer_t *buffer)
{
    for(uint32_t buffer_index = 0; buffer_index < r_device.staging.staging_buffer_count; buffer_index++)
    {
        if(r_device.staging.staging_buffers + buffer_index == buffer)
        {
            vkResetEvent(r_device.device, r_device.staging.staging_buffers[buffer_index].complete_event);
            add_list_element(&r_device.staging.free_buffers, &buffer_index);
            return;
        }
    }
}

//void *r_LockStagingMemory()
//{
//    SDL_AtomicLock(&r_device.staging.staging_spinlock);
//    return r_device.staging.base_staging_pointer;
//}
//
//void r_UnlockStagingMemory()
//{
//    SDL_AtomicUnlock(&r_device.staging.staging_spinlock);
//}

/*
=================================================================
=================================================================
=================================================================
*/

union r_command_buffer_h r_AllocateCommandBuffer()
{
    struct r_command_buffer_t *command_buffer;
    union r_command_buffer_h command_buffer_handle;
    uint32_t command_buffer_index;
//    uint32_t new_command_buffer_index;
//    uint32_t semaphore_value;
    VkCommandBufferAllocateInfo allocate_info = {};
    VkEventCreateInfo event_create_info = {.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};
    VkResult result;
//    uint32_t recycled_command_buffer_count = 0;

    SDL_AtomicLock(&r_device.draw_command_pool.spinlock);

    for(uint32_t index = 0; index < r_device.draw_command_pool.pending_command_buffers.cursor; index++)
    {
        command_buffer_index = *(uint32_t *)get_list_element(&r_device.draw_command_pool.pending_command_buffers, index);
        command_buffer = get_stack_list_element(&r_device.draw_command_pool.command_buffers, command_buffer_index);
        result = vkGetEventStatus(r_device.device, command_buffer->complete_event);

        if(result == VK_EVENT_SET)
        {
            remove_list_element(&r_device.draw_command_pool.pending_command_buffers, index);
            remove_stack_list_element(&r_device.draw_command_pool.command_buffers, command_buffer_index);
            vkResetEvent(r_device.device, command_buffer->complete_event);
        }
    }

    command_buffer_handle.index = add_stack_list_element(&r_device.draw_command_pool.command_buffers, NULL);
    command_buffer = get_stack_list_element(&r_device.draw_command_pool.command_buffers, command_buffer_handle.index);

    if(command_buffer->command_buffer == VK_NULL_HANDLE)
    {
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;
        allocate_info.commandPool = r_device.draw_command_pool.command_pool;

        vkAllocateCommandBuffers(r_device.device, &allocate_info, &command_buffer->command_buffer);
        vkCreateEvent(r_device.device, &event_create_info, NULL, &command_buffer->complete_event);
        command_buffer->events = create_list(sizeof(VkEvent), 1024);
    }

    SDL_AtomicUnlock(&r_device.draw_command_pool.spinlock);
    return command_buffer_handle;
}

struct r_command_buffer_t *r_GetCommandBufferPointer(union r_command_buffer_h command_buffer)
{
    struct r_command_buffer_t *command_buffer_ptr = NULL;
    command_buffer_ptr = get_stack_list_element(&r_device.draw_command_pool.command_buffers, command_buffer.index);
    return command_buffer_ptr;
}

void r_MarkCommandBufferAsPending(VkCommandBuffer command_buffer)
{

}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_buffer_h r_CreateBuffer(VkBufferCreateInfo *description)
{
    struct r_buffer_h handle = R_INVALID_BUFFER_HANDLE;
    struct r_buffer_t *buffer;
    struct r_buffer_heap_t *heap;
    struct r_chunk_t *chunk;
    VkBufferCreateInfo buffer_create_info = {};
    VkMemoryRequirements memory_requirements;

    handle.index = add_stack_list_element(&r_device.buffers, NULL);
    buffer = get_stack_list_element(&r_device.buffers, handle.index);

    memcpy(&buffer_create_info, description, sizeof(VkBufferCreateInfo));

    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(r_device.device, &buffer_create_info, NULL, &buffer->buffer);
    vkGetBufferMemoryRequirements(r_device.device, buffer->buffer, &memory_requirements);
    buffer->memory = r_AllocChunk(r_device.buffer_heap, memory_requirements.size, memory_requirements.alignment);
    chunk = r_GetChunkPointer(buffer->memory);
    heap = (struct r_buffer_heap_t *)r_GetHeapPointer(buffer->memory.heap);
    vkBindBufferMemory(r_device.device, buffer->buffer, heap->memory, chunk->start);

    return handle;
}

void r_DestroyBuffer(struct r_buffer_h handle)
{

}

struct r_buffer_t *r_GetBufferPointer(struct r_buffer_h handle)
{
    struct r_buffer_t *buffer;
    buffer = get_stack_list_element(&r_device.buffers, handle.index);
    if(buffer && buffer->memory.index == R_INVALID_CHUNK_INDEX)
    {
        buffer = NULL;
    }
    return buffer;
}

//void r_FillBuffer(struct r_buffer_handle_t handle, void *data, uint32_t size, uint32_t offset)
//{
//    struct r_buffer_t *buffer;
//    struct r_buffer_heap_t *heap;
//    VkBufferCreateInfo copy_buffer_create_info = {};
//    VkBuffer copy_buffer;
//    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
//    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
//    VkBufferCopy buffer_copy;
//
//    buffer = r_GetBufferPointer(handle);
//    heap = r_GetHeapPointer(buffer->memory.heap);
//
//    copy_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//    copy_buffer_create_info.size = size;
//    copy_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
//    copy_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//    vkCreateBuffer(r_device.device, &copy_buffer_create_info, NULL, &copy_buffer);
//    vkBindBufferMemory(r_device.device, copy_buffer, r_device.staging.staging_memory, 0);
//    memcpy(r_device.staging.staging_pointer, data, size);
//
//    submit_info.commandBufferCount = 1;
//    submit_info.pCommandBuffers = &r_device.transfer_command_buffer;
//
//    buffer_copy.srcOffset = 0;
//    buffer_copy.dstOffset = offset;
//    buffer_copy.size = size;
//
//    SDL_AtomicLock(&r_device.transfer_queue->spinlock);
//    vkBeginCommandBuffer(r_device.transfer_command_buffer, &begin_info);
//    vkCmdCopyBuffer(r_device.transfer_command_buffer, copy_buffer, buffer->buffer, 1, &buffer_copy);
//    vkEndCommandBuffer(r_device.transfer_command_buffer);
//    vkResetFences(r_device.device, 1, &r_device.transfer_fence);
//    vkQueueSubmit(r_device.transfer_queue->queue, 1, &submit_info, r_device.transfer_fence);
//    vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);
//    SDL_AtomicUnlock(&r_device.transfer_queue->spinlock);
//    vkDestroyBuffer(r_device.device, copy_buffer, NULL);
//}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_image_handle_t r_CreateImage(VkImageCreateInfo *description)
{
    struct r_image_handle_t handle;
    handle = r_AllocImage(description);
    r_AllocImageMemory(handle);
    return handle;
}

struct r_image_handle_t r_AllocImage(VkImageCreateInfo *description)
{
    struct r_image_handle_t handle = R_INVALID_IMAGE_HANDLE;
    struct r_image_t *image;
    VkImage vk_image;
    VkImageCreateInfo description_copy;
//    VkMemoryRequirements memory_requirements;
//    VkMemoryAllocateInfo allocate_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};



    memcpy(&description_copy, description, sizeof(VkImageCreateInfo));

    description_copy.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

    if(!description_copy.mipLevels)
    {
        description_copy.mipLevels = 1;
    }

    if(!description_copy.arrayLayers)
    {
        description_copy.arrayLayers = 1;
    }

    if(!description_copy.samples)
    {
        description_copy.samples = VK_SAMPLE_COUNT_1_BIT;
    }

    if(!description_copy.usage)
    {
        description_copy.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        description_copy.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if(r_IsDepthStencilFormat(description_copy.format))
        {
            description_copy.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        else
        {
            description_copy.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }

    description_copy.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(r_device.device, &description_copy, NULL, &vk_image);

    if(vk_image != VK_NULL_HANDLE)
    {
        handle.index = add_stack_list_element(&r_device.images, NULL);
        add_stack_list_element(&r_device.image_descriptions, &description_copy);

        image = get_stack_list_element(&r_device.images, handle.index);
        image->aspect_mask = r_GetFormatAspectFlags(description_copy.format);
        image->memory = R_INVALID_CHUNK_HANDLE;
        image->image = vk_image;
    }

    return handle;
}

void r_AllocImageMemory(struct r_image_handle_t handle)
{
    struct r_image_t *image;
    struct r_chunk_t *chunk;
    struct r_image_heap_t *heap;
    VkMemoryRequirements memory_requirements;
//    VkMemoryAllocateInfo allocate_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};

    image = r_GetImagePointer(handle);
    vkGetImageMemoryRequirements(r_device.device, image->image, &memory_requirements);

//    allocate_info.memoryTypeIndex = r_MemoryIndexWithProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//    allocate_info.allocationSize = memory_requirements.size;
    image->memory = r_AllocChunk(r_device.texture_heap, memory_requirements.size, memory_requirements.alignment);
    chunk = r_GetChunkPointer(image->memory);
    heap = (struct r_image_heap_t *)r_GetHeapPointer(image->memory.heap);
    vkBindImageMemory(r_device.device, image->image, heap->memory, chunk->start);
}

struct r_image_handle_t r_CreateImageFrom(struct r_image_t *image)
{
    struct r_image_handle_t handle = R_INVALID_IMAGE_HANDLE;
    handle.index = add_stack_list_element(&r_device.images, image);
    return handle;
}

void r_DestroyImage(struct r_image_handle_t handle)
{
    struct r_image_t *image;
    image = r_GetImagePointer(handle);
    if(image)
    {
        vkDestroyImage(r_device.device, image->image, NULL);
        r_FreeChunk(image->memory);
        remove_stack_list_element(&r_device.images, handle.index);
        remove_stack_list_element(&r_device.image_descriptions, handle.index);
    }
}

struct r_image_t *r_GetImagePointer(struct r_image_handle_t handle)
{
    struct r_image_t *image = NULL;
    image = get_stack_list_element(&r_device.images, handle.index);
    return image;
}

VkImageCreateInfo *r_GetImageDescriptionPointer(struct r_image_handle_t handle)
{
    VkImageCreateInfo *description = NULL;
    description = get_stack_list_element(&r_device.image_descriptions, handle.index);
    return description;
}

//void r_BlitImage(struct r_image_handle_t src_handle, struct r_image_handle_t dst_handle, VkImageBlit *blit)
//{
//    VkCommandBufferBeginInfo command_buffer_begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
//    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
//
//    submit_info.commandBufferCount = 1;
//    submit_info.pCommandBuffers = &r_device.transfer_command_buffer;
//
//    vkResetCommandBuffer(r_device.transfer_command_buffer, 0);
//    vkBeginCommandBuffer(r_device.transfer_command_buffer, &command_buffer_begin_info);
//    r_vkCmdBlitImage(r_device.transfer_command_buffer, src_handle, dst_handle, blit);
//    vkEndCommandBuffer(r_device.transfer_command_buffer);
//
////    r_LockQueue(r_device.transfer_queue);
//    SDL_AtomicLock(&r_device.transfer_queue->spinlock);
//    vkResetFences(r_device.device, 1, &r_device.transfer_fence);
//    vkQueueSubmit(r_device.transfer_queue->queue, 1, &submit_info, r_device.transfer_fence);
//    vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);
//    SDL_AtomicUnlock(&r_device.transfer_queue->spinlock);
////    r_UnlockQueue(r_device.transfer_queue);
//}

void r_SetImageLayout(struct r_image_handle_t image, VkImageLayout new_layout)
{
    struct r_submit_info_t submit_info = {};
    union r_command_buffer_h command_buffer = r_AllocateCommandBuffer();
    r_vkBeginCommandBuffer(command_buffer);
    r_vkCmdSetImageLayout(command_buffer, image, new_layout);
    r_vkEndCommandBuffer(command_buffer);

    vkResetFences(r_device.device, 1, &r_device.transfer_fence);
    submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.command_buffer_count = 1;
    submit_info.command_buffers = &command_buffer;
    r_vkQueueSubmit(r_device.transfer_queue->queue, 1, &submit_info, r_device.transfer_fence);
    vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);
}

void r_CreateDefaultTexture()
{
    struct r_texture_handle_t default_texture;
    struct r_texture_description_t description = {};;
    struct r_texture_t *texture;
    uint32_t *default_texture_pixels;
    uint32_t pixel_pitch;
//    uint32_t row_pitch;

    description.extent.width = 8;
    description.extent.height = 8;
    description.extent.depth = 1;
    description.image_type = VK_IMAGE_TYPE_2D;
    description.name = "default_texture";
    description.format = VK_FORMAT_R8G8B8A8_UNORM;
    description.sampler_params.min_filter = VK_FILTER_NEAREST;
    description.sampler_params.mag_filter = VK_FILTER_NEAREST;
    description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    default_texture = r_CreateTexture(&description);
    pixel_pitch = r_GetFormatPixelPitch(description.format);
//    row_pitch = pixel_pitch * description.extent.width;
    default_texture_pixels = calloc(pixel_pitch, description.extent.width * description.extent.height);
    texture = r_GetTexturePointer(default_texture);
    for(uint32_t y = 0; y < description.extent.height; y++)
    {
        for(uint32_t x = 0; x < description.extent.width; x++)
        {
            default_texture_pixels[y * description.extent.width + x] = ((x + y) % 2) ? 0xff111111 : 0xff222222;
        }
    }

    r_FillImageChunk(texture->image, default_texture_pixels, NULL);
    r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    free(default_texture_pixels);
}

struct r_texture_handle_t r_CreateTexture(struct r_texture_description_t *description)
{
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_t *texture;
    struct r_image_t *image;
    struct r_texture_description_t texture_description;
    VkImageViewCreateInfo image_view_create_info = {};

    if(description)
    {
        handle.index = add_stack_list_element(&r_device.textures, NULL);
        texture = get_stack_list_element(&r_device.textures, handle.index);
        memcpy(&texture_description, description, sizeof(struct r_texture_description_t));
        texture->image = r_CreateImage((VkImageCreateInfo *)&texture_description);
        memcpy(&texture_description, r_GetImageDescriptionPointer(texture->image), sizeof(VkImageCreateInfo));
        image = r_GetImagePointer(texture->image);

        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = NULL;
        image_view_create_info.flags = 0;
        image_view_create_info.image = image->image;

        switch(texture_description.image_type)
        {
            case VK_IMAGE_TYPE_1D:
                if(texture_description.array_layers > 1)
                {
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                }
                else
                {
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_1D;
                }
            break;

            case VK_IMAGE_TYPE_2D:
                if(texture_description.array_layers > 1)
                {
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                }
                else
                {
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                }
            break;
        }

        image_view_create_info.format = texture_description.format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;

        image_view_create_info.subresourceRange.aspectMask = r_GetFormatAspectFlags(texture_description.format);
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = texture_description.mip_levels;
        image_view_create_info.subresourceRange.layerCount = texture_description.array_layers;
        vkCreateImageView(r_device.device, &image_view_create_info, NULL, &texture->image_view);
        texture->sampler = r_TextureSampler(&texture_description.sampler_params);
        r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return handle;
}

void r_DestroyTexture(struct r_texture_handle_t handle)
{
    struct r_texture_t *texture = r_GetTexturePointer(handle);

    if(texture && handle.index != R_DEFAULT_TEXTURE_INDEX /*&& handle.index != R_MISSING_TEXTURE_INDEX*/ )
    {
        vkDestroyImageView(r_device.device, texture->image_view, NULL);
        r_DestroyImage(texture->image);
//        SDL_AtomicLock(&r_resource_spinlock);
        remove_stack_list_element(&r_device.textures, handle.index);
//        SDL_AtomicUnlock(&r_resource_spinlock);
    }
}


VkSampler r_TextureSampler(struct r_sampler_params_t *params)
{
    struct r_sampler_t *sampler = NULL;
    VkSampler vk_sampler = VK_NULL_HANDLE;
    uint32_t sampler_index;

    for(sampler_index = 0; sampler_index < r_device.samplers.cursor; sampler_index++)
    {
        sampler = get_stack_list_element(&r_device.samplers, sampler_index);

        if(!memcmp(&sampler->params, params, sizeof(struct r_sampler_params_t)))
        {
            vk_sampler = sampler->sampler;
            break;
        }
    }

    if(sampler_index >= r_device.samplers.cursor)
    {
        VkSamplerCreateInfo sampler_create_info = {};
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = NULL;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = params->mag_filter;
        sampler_create_info.minFilter = params->min_filter;
        sampler_create_info.mipmapMode = params->mipmap_mode;
        sampler_create_info.addressModeU = params->addr_mode_u;
        sampler_create_info.addressModeV = params->addr_mode_v;
        sampler_create_info.addressModeW = params->addr_mode_w;
        sampler_create_info.mipLodBias = 0.0;
        sampler_create_info.anisotropyEnable = VK_FALSE;
        sampler_create_info.maxAnisotropy = 2.0;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
        sampler_create_info.minLod = 0.0;
        sampler_create_info.maxLod = 0.0;
        sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;

        sampler_index = add_stack_list_element(&r_device.samplers, NULL);
        sampler = get_stack_list_element(&r_device.samplers, sampler_index);

        vkCreateSampler(r_device.device, &sampler_create_info, NULL, &vk_sampler);
        sampler->sampler = vk_sampler;
        memcpy(&sampler->params, params, sizeof(struct r_sampler_params_t));
    }

    return vk_sampler;
}

struct r_texture_handle_t r_LoadTexture(char *file_name, char *texture_name)
{
    unsigned char *pixels;
    int width;
    int height;
    int channels;
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_t *texture;
    struct r_texture_description_t description = {};

    file_name = format_path(file_name);

    pixels = stbi_load(file_name, &width, &height, &channels, STBI_rgb_alpha);

    if(pixels)
    {
        description.extent.width = width;
        description.extent.height = height;
        description.extent.depth = 1;
        description.image_type = VK_IMAGE_TYPE_2D;
        description.format = VK_FORMAT_R8G8B8A8_UNORM;
        description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.mag_filter = VK_FILTER_LINEAR;
        description.sampler_params.min_filter = VK_FILTER_LINEAR;
        description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        handle = r_CreateTexture(&description);
        texture = r_GetTexturePointer(handle);

        if(texture_name)
        {
            texture->name = strdup(texture_name);
        }
        else
        {
            texture->name = strdup(file_name);
        }

        r_FillImageChunk(texture->image, pixels, NULL);
        printf("texture %s loaded!\n", texture->name);
    }

    return handle;
}

struct r_texture_t *r_GetTexturePointer(struct r_texture_handle_t handle)
{
//    struct r_texture_description_t *description;
    struct r_texture_t *texture = NULL;
//    description = r_GetTextureDescriptionPointer(handle);

//    if(description)
//    {
    texture = get_stack_list_element(&r_device.textures, handle.index);
//    }

    return texture;
}

struct r_texture_t* r_GetDefaultTexturePointer()
{
    return r_GetTexturePointer(R_TEXTURE_HANDLE(R_DEFAULT_TEXTURE_INDEX));
}

struct r_texture_handle_t r_GetDefaultTextureHandle()
{
    return R_TEXTURE_HANDLE(R_DEFAULT_TEXTURE_INDEX);
}

struct r_texture_handle_t r_GetTextureHandle(char *name)
{
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
//    struct r_texture_t *texture;
//
//    for(uint32_t i = 0; i < r_renderer.textures.cursor; i++)
//    {
//        texture = r_GetTexturePointer(R_TEXTURE_HANDLE(i));
//
//        if(texture && !strcmp(texture->name, name))
//        {
//            handle = R_TEXTURE_HANDLE(i);
//            break;
//        }
//    }

    return handle;
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_shader_handle_t r_CreateShader(struct r_shader_description_t *description)
{
    struct r_shader_t *shader;
    struct r_shader_handle_t handle = R_INVALID_SHADER_HANDLE;
    struct r_vertex_attrib_t *attribs;
//    struct r_vertex_attrib_t *attrib;
    struct r_vertex_binding_t *binding;
    struct r_resource_binding_t *resource_binding;
    uint32_t attrib_count;
    uint32_t alloc_size;
    VkDescriptorSetLayoutBinding *layout_bindings;
    VkDescriptorSetLayoutBinding *layout_binding;

    if(description)
    {
        handle.index = add_stack_list_element(&r_device.shaders, NULL);
        shader = (struct r_shader_t *)get_stack_list_element(&r_device.shaders, handle.index);
        memset(shader, 0, sizeof(struct r_shader_t));
        shader->stage = description->stage;
        shader->push_constant_count = description->push_constant_count;
        shader->vertex_binding_count = description->vertex_binding_count;
        shader->resource_count = description->resource_count;

        if(description->push_constant_count)
        {
            shader->push_constants = calloc(description->push_constant_count, sizeof(struct r_push_constant_t));
            memcpy(shader->push_constants, description->push_constants, description->push_constant_count *
                                                                    sizeof(struct r_push_constant_t));
        }

        if(description->vertex_binding_count)
        {
            attrib_count = 0;
            for(uint32_t i = 0; i < description->vertex_binding_count; i++)
            {
                attrib_count += description->vertex_bindings[i].attrib_count;
            }
            shader->vertex_attrib_count = attrib_count;
            alloc_size = attrib_count * sizeof(struct r_vertex_attrib_t);
            alloc_size += description->vertex_binding_count * sizeof(struct r_vertex_binding_t);

            /* single memory block for bindings and attributes */
            shader->vertex_bindings = (struct r_vertex_binding_t *)calloc(1, alloc_size);
            /* attributes exist in memory right after the bindings */
            attribs = (struct r_vertex_attrib_t *)(shader->vertex_bindings + description->vertex_binding_count);
            memcpy(shader->vertex_bindings, description->vertex_bindings, sizeof(struct r_vertex_binding_t) *
                   description->vertex_binding_count);
            attrib_count = 0;
            for(uint32_t i = 0; i < description->vertex_binding_count; i++)
            {
                binding = shader->vertex_bindings + i;
                binding->attribs = attribs + attrib_count;
                attrib_count += binding->attrib_count;

                memcpy(binding->attribs, description->vertex_bindings[i].attribs,
                    sizeof(struct r_vertex_attrib_t) * binding->attrib_count);
            }
        }

        if(description->resource_count)
        {
            shader->resources = calloc(description->resource_count, sizeof(struct r_resource_binding_t));
            memcpy(shader->resources, description->resources, sizeof(struct r_resource_binding_t) * description->resource_count);

            layout_bindings = alloca(sizeof(VkDescriptorSetLayoutBinding) * description->resource_count);

            for(uint32_t i = 0; i < description->resource_count; i++)
            {
                layout_binding = layout_bindings + i;
                resource_binding = description->resources + i;

                layout_binding->binding = i;
                layout_binding->descriptorType = resource_binding->descriptor_type;
                layout_binding->descriptorCount = resource_binding->count;
                layout_binding->pImmutableSamplers = NULL;
                layout_binding->stageFlags = description->stage;
            }

            VkDescriptorSetLayoutCreateInfo layout_create_info = {};
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.pNext = NULL;
            layout_create_info.flags = 0;
            layout_create_info.bindingCount = description->resource_count;
            layout_create_info.pBindings = layout_bindings;
            vkCreateDescriptorSetLayout(r_device.device, &layout_create_info, NULL, &shader->descriptor_set_layout);
        }

        VkShaderModuleCreateInfo module_create_info = {};
        module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_create_info.pNext = NULL;
        module_create_info.flags = 0;
        module_create_info.codeSize = description->code_size;
        module_create_info.pCode = description->code;
        vkCreateShaderModule(r_device.device, &module_create_info, NULL, &shader->module);
    }

    return handle;
}

void r_DestroyShader(struct r_shader_handle_t handle)
{
//    struct r_shader_t *shader;
//    shader = r_GetShaderPointer(handle);
//    if(shader)
//    {
//        r_vk_DestroyShader(shader);
//
//        if(shader->description.push_constants)
//        {
//            free(shader->description.push_constants);
//        }
//
//        if(shader->description.resources)
//        {
//            free(shader->description.resources);
//        }
//
//        if(shader->description.vertex_bindings)
//        {
//            free(shader->description.vertex_bindings);
//        }
//
//        shader->description.resource_count = 0xffffffff;
//        shader->description.vertex_binding_count = 0xffffffff;
//        remove_stack_list_element(&r_renderer.shaders, handle.index);
//    }
}

struct r_shader_t *r_GetShaderPointer(struct r_shader_handle_t handle)
{
    struct r_shader_t *shader = NULL;
    shader = get_stack_list_element(&r_device.shaders, handle.index);
//    if(shader && shader->description.resource_count == 0xffffffff)
//    {
//        shader = NULL;
//    }

    return shader;
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_render_pass_handle_t r_CreateRenderPass(struct r_render_pass_description_t *description)
{
    struct r_render_pass_handle_t handle = R_INVALID_RENDER_PASS_HANDLE;
    struct r_render_pass_t *render_pass;
//    struct r_shader_t *vertex_shader;
//    struct r_shader_t *fragment_shader;
//    struct r_render_pass_description_t *description_copy;
//    struct r_pipeline_description_t pipeline_description;
//    struct r_pipeline_t *pipeline;
//    struct r_resource_binding_t *resource;
    struct r_subpass_description_t *subpass_descriptions; /* will contain the subpass descriptions that came
    in, or the one that was created in here in the case one wasn't provided by the caller */

//    struct r_subpass_description_t *subpass_description; /* used to iterate over the subpass_descriptions array */

    VkAttachmentReference *color_attachment_references;
    VkAttachmentReference *color_attachment_reference;
    VkAttachmentReference *depth_stencil_attachment_references;
    VkAttachmentReference *depth_stencil_attachment_reference;
//    VkAttachmentReference *attachment_reference; /* used to iterate over an array of attachment references */

//    VkAttachmentDescription *attachment_description; /* used to iterate over an array of attachment descriptions */

    VkGraphicsPipelineCreateInfo pipeline_create_info = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    VkRenderPassCreateInfo render_pass_create_info = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

    VkPipelineColorBlendAttachmentState *attachment_state; /* used to iterate over the array of attachment states. */

//    VkPipelineVertexInputStateCreateInfo vertex_input_state;
//    uint32_t max_vertex_bindings = 0;
//    uint32_t max_vertex_attributes = 0;
    uint32_t max_shader_resources = 0;
    uint32_t color_attachment_count = 0;
//    VkVertexInputBindingDescription *vertex_bindings;
//    VkVertexInputAttributeDescription *vertex_attributes;

    /* these are just used to fill any of the attributes that might be missing
    in the pipeline description that we got.   */
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0
    };

    VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .front = (VkStencilOpState){
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .writeMask = 0xffffffff,
            .reference = 0xffffffff
        },
        .back = (VkStencilOpState){
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .writeMask = 0xffffffff,
            .reference = 0xffffffff
        },
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_NO_OP,
        .blendConstants = {1.0, 1.0, 1.0, 1.0}
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = (VkDynamicState []){
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 512,
    };

    VkPipelineVertexInputStateCreateInfo input_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

//    VkVertexInputBindingDescription vertex_binding_description = {};
//    VkVertexInputAttributeDescription vertex_attribute_description = {};

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    struct r_pipeline_t *pipelines;

//    uint32_t depth_stencil_index = 0xffffffff;

    handle.index = add_stack_list_element(&r_device.render_passes, NULL);
    render_pass = (struct r_render_pass_t *)get_stack_list_element(&r_device.render_passes, handle.index);

    subpass_descriptions = calloc(sizeof(struct r_subpass_description_t), description->subpass_count);
    memcpy(subpass_descriptions, description->subpasses, sizeof(struct r_subpass_description_t) * description->subpass_count);

    for(uint32_t attachment_index = 0; attachment_index < description->attachment_count; attachment_index++)
    {
        /* we may have a depth stencil attachment in the middle of the color attachments, so
        find out how many are color attachments */
        if(!r_IsDepthStencilFormat(description->attachments[attachment_index].format))
        {
            color_attachment_count++;
        }
    }

    color_attachment_references = calloc(sizeof(VkAttachmentReference), color_attachment_count * description->subpass_count);
    color_attachment_reference = color_attachment_references;
    depth_stencil_attachment_references = calloc(sizeof(VkAttachmentReference), description->subpass_count);
    depth_stencil_attachment_reference = depth_stencil_attachment_references;
    /* Each sub pass will have its own VkPipeline object. Each pipeline has a VkPipelineColorBlendStateCreateInfo,
    which contains an array of VkPipelineColorBlendAttachmentState. Each element of this array matches an element
    of the pColorAttachments array in the sub pass. Since pColorAttachments will have at most color_attachment_count
    elements, it's safe to allocate the same amount, as just a single VkPipeline will be created at each time, and
    a VkPipeline refers only to a single sub pass */
    color_blend_state.pAttachments = calloc(sizeof(VkPipelineColorBlendAttachmentState), color_attachment_count);

    /* since missing attachment state will be all the same, just pre fill everything with the same
    values upfront */
    for(uint32_t attachment_index = 0; attachment_index < color_attachment_count; attachment_index++)
    {
        /* drop the pesky const */
        attachment_state = (VkPipelineColorBlendAttachmentState *)color_blend_state.pAttachments + attachment_index;
        attachment_state->blendEnable = VK_FALSE;
        attachment_state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment_state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment_state->colorBlendOp = VK_BLEND_OP_ADD;
        attachment_state->srcAlphaBlendFactor = 1.0;
        attachment_state->dstAlphaBlendFactor = 1.0;
        attachment_state->alphaBlendOp = VK_BLEND_OP_ADD;
        attachment_state->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    for(uint32_t subpass_index = 0; subpass_index < description->subpass_count; subpass_index++)
    {
        uint32_t set_layout_count = 0;
        uint32_t push_constant_count = 0;
//        uint32_t vertex_binding_count = 0;
//        uint32_t vertex_attribute_count = 0;

        struct r_subpass_description_t *subpass_description = subpass_descriptions + subpass_index;
        subpass_description->pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

        if(subpass_description->pipeline_description->shader_count > pipeline_create_info.stageCount)
        {
            pipeline_create_info.stageCount = subpass_description->pipeline_description->shader_count;
        }

        for(uint32_t shader_index = 0; shader_index < subpass_description->pipeline_description->shader_count; shader_index++)
        {
            struct r_shader_t *shader = subpass_description->pipeline_description->shaders[shader_index];
            set_layout_count += shader->descriptor_set_layout != VK_NULL_HANDLE;
            push_constant_count += shader->push_constant_count;

            if(shader->resource_count > max_shader_resources)
            {
                max_shader_resources = shader->resource_count;
            }

            if(shader->stage == VK_SHADER_STAGE_VERTEX_BIT)
            {
                if(shader->vertex_attrib_count > input_state_create_info.vertexBindingDescriptionCount)
                {
                    input_state_create_info.vertexBindingDescriptionCount = shader->vertex_binding_count;
                }

                if(shader->vertex_attrib_count > input_state_create_info.vertexAttributeDescriptionCount)
                {
                    input_state_create_info.vertexAttributeDescriptionCount = shader->vertex_attrib_count;
                }
            }
        }

        if(set_layout_count > pipeline_layout_create_info.setLayoutCount)
        {
            pipeline_layout_create_info.setLayoutCount = set_layout_count;
        }

        if(push_constant_count > pipeline_layout_create_info.pushConstantRangeCount)
        {
            pipeline_layout_create_info.pushConstantRangeCount = push_constant_count;
        }

        VkAttachmentReference *first_color_attachment_reference = color_attachment_reference;

        for(uint32_t attachment_index = 0; attachment_index < description->attachment_count; attachment_index++)
        {
            if(r_IsDepthStencilFormat(description->attachments[attachment_index].format))
            {
                continue;
            }

            color_attachment_reference->attachment = VK_ATTACHMENT_UNUSED;
            color_attachment_reference->layout = VK_IMAGE_LAYOUT_UNDEFINED;

            for(uint32_t reference_index = 0; reference_index < subpass_description->color_attachment_count; reference_index++)
            {
                /* if this sub pass uses no color reference all the references will be marked as unused */
                if(attachment_index == subpass_description->color_attachments[reference_index].attachment)
                {
                    memcpy(color_attachment_reference, subpass_description->color_attachments + reference_index, sizeof(VkAttachmentReference));
                    if(color_attachment_reference->layout == VK_IMAGE_LAYOUT_UNDEFINED)
                    {
                        /* since VK_IMAGE_LAYOUT_UNDEFINED is 0, this means this attachment
                        probably had this field zero initialized. So, fill in a sane value here */
                        color_attachment_reference->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    }
                }
            }

            color_attachment_reference++;
        }

        subpass_description->color_attachments = first_color_attachment_reference;
        subpass_description->color_attachment_count = color_attachment_count;

        VkAttachmentReference *first_depth_stencil_attachment_reference = depth_stencil_attachment_reference;
        depth_stencil_attachment_reference->attachment = VK_ATTACHMENT_UNUSED;
        depth_stencil_attachment_reference->layout = VK_IMAGE_LAYOUT_UNDEFINED;

        if(subpass_description->depth_stencil_attachment)
        {
            memcpy(depth_stencil_attachment_reference, subpass_description->depth_stencil_attachment, sizeof(VkAttachmentReference));
            if(depth_stencil_attachment_reference->layout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                /* since VK_IMAGE_LAYOUT_UNDEFINED is 0, this means this attachment
                probably had this field zero initialized. So, fill in a sane value here */
                depth_stencil_attachment_reference->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
        }

        subpass_description->depth_stencil_attachment = first_depth_stencil_attachment_reference;
        depth_stencil_attachment_reference++;
    }

    input_state_create_info.pVertexBindingDescriptions = calloc(sizeof(VkVertexInputBindingDescription), input_state_create_info.vertexBindingDescriptionCount);
    input_state_create_info.pVertexAttributeDescriptions = calloc(sizeof(VkVertexInputAttributeDescription), input_state_create_info.vertexAttributeDescriptionCount);

//    VkVertexInputBindingDescription binding_descriptions[input_state_create_info.vertexBindingDescriptionCount];
//    VkVertexInputAttributeDescription attribute_descriptions[input_state_create_info.vertexAttributeDescriptionCount];
//    input_state_create_info.pVertexBindingDescriptions = binding_descriptions;
//    input_state_create_info.pVertexAttributeDescriptions = attribute_descriptions;

    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = NULL;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = description->attachment_count;
    render_pass_create_info.pAttachments = calloc(sizeof(VkAttachmentDescription), description->attachment_count);

    if(description->attachment_count)
    {
        memcpy((void *)render_pass_create_info.pAttachments, description->attachments, sizeof(VkAttachmentDescription) * description->attachment_count);
        for(uint32_t attachment_index = 0; attachment_index < description->attachment_count; attachment_index++)
        {
            VkAttachmentDescription *attachment_description = (VkAttachmentDescription *)render_pass_create_info.pAttachments + attachment_index;

            if(!attachment_description->samples)
            {
                /* probably zero initialized. No samples means one sample */
                attachment_description->samples = VK_SAMPLE_COUNT_1_BIT;
            }

            if(attachment_description->finalLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                /* fill in the final layout with a sane value if we
                didn't get one */
                if(r_IsDepthStencilFormat(attachment_description->format))
                {
                    attachment_description->finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                else
                {
                    attachment_description->finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            }
        }
    }

    render_pass_create_info.subpassCount = description->subpass_count;
    render_pass_create_info.pSubpasses = calloc(sizeof(VkSubpassDescription), description->subpass_count);
    memset((VkSubpassDescription *)render_pass_create_info.pSubpasses, 0, sizeof(VkSubpassDescription) * description->subpass_count);

    for(uint32_t subpass_index = 0; subpass_index < description->subpass_count; subpass_index++)
    {
        struct r_subpass_description_t *subpass_description = subpass_descriptions + subpass_index;
        VkSubpassDescription *subpass = (VkSubpassDescription *)render_pass_create_info.pSubpasses + subpass_index;
        memcpy(subpass, subpass_description, sizeof(VkSubpassDescription));
    }

    vkCreateRenderPass(r_device.device, &render_pass_create_info, NULL, &render_pass->render_pass);

    if(description->subpass_count > 1)
    {
        render_pass->pipelines = calloc(description->subpass_count, sizeof(struct r_pipeline_t));
        pipelines = render_pass->pipelines;
    }
    else
    {
        pipelines = &render_pass->pipeline;
    }

    pipeline_create_info.pStages = calloc(sizeof(VkPipelineShaderStageCreateInfo), pipeline_create_info.stageCount);
    pipeline_layout_create_info.pSetLayouts = calloc(sizeof(VkDescriptorSetLayout), pipeline_layout_create_info.setLayoutCount);
    pipeline_layout_create_info.pPushConstantRanges = calloc(sizeof(VkPushConstantRange), pipeline_layout_create_info.pushConstantRangeCount);

    descriptor_pool_create_info.pPoolSizes = calloc(sizeof(VkDescriptorPoolSize), max_shader_resources);
    for(uint32_t subpass_index = 0; subpass_index < description->subpass_count; subpass_index++)
    {
        /* create a pipeline object for each subpass in this render pass to reduce verbosity.
        some fields in the pipeline description may be zero initialized (but not left uninitialized!).
        Those zero initialized fields will be filled with some default values. */

        struct r_pipeline_t *pipeline = pipelines + subpass_index;
        pipeline_layout_create_info.setLayoutCount = 0;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_create_info.stageCount = 0;

        struct r_subpass_description_t *subpass_description = subpass_descriptions + subpass_index;

        for(uint32_t shader_index = 0; shader_index < subpass_description->pipeline_description->shader_count; shader_index++)
        {
            struct r_shader_t *shader = subpass_description->pipeline_description->shaders[shader_index];
            if(shader->descriptor_set_layout != VK_NULL_HANDLE)
            {
                VkDescriptorSetLayout *layout = (VkDescriptorSetLayout *)pipeline_layout_create_info.pSetLayouts + pipeline_layout_create_info.setLayoutCount;
                *layout = shader->descriptor_set_layout;
                pipeline_layout_create_info.setLayoutCount++;
            }
            if(shader->push_constant_count)
            {
                for(uint32_t range_index = 0; range_index < shader->push_constant_count; range_index++)
                {
                    VkPushConstantRange *range = (VkPushConstantRange *)pipeline_layout_create_info.pPushConstantRanges + pipeline_layout_create_info.pushConstantRangeCount;
                    pipeline_layout_create_info.pushConstantRangeCount++;
                    range->stageFlags = shader->stage;
                    range->size = shader->push_constants[range_index].size;
                    range->offset = shader->push_constants[range_index].offset;
                }
            }

            uint32_t pool_list_index;
            switch(shader->stage)
            {
                case VK_SHADER_STAGE_VERTEX_BIT:
                    pool_list_index = 0;
                    pipeline->vertex_shader = shader;
                break;

                case VK_SHADER_STAGE_FRAGMENT_BIT:
                    pool_list_index = 1;
                    pipeline->fragment_shader = shader;
                break;
            }

            if(shader->resource_count)
            {
                descriptor_pool_create_info.poolSizeCount = shader->resource_count;
                for(uint32_t resource_index = 0; resource_index < shader->resource_count; resource_index++)
                {
                    struct r_resource_binding_t *resource = shader->resources + resource_index;
                    VkDescriptorPoolSize *size = (VkDescriptorPoolSize *)descriptor_pool_create_info.pPoolSizes + resource_index;
                    size->type = resource->descriptor_type;
                    size->descriptorCount = resource->count * descriptor_pool_create_info.maxSets;
                }

                struct r_descriptor_pool_list_t *list = pipeline->pool_lists + pool_list_index;
                list->free_pools = create_list(sizeof(struct r_descriptor_pool_t), 16);
                list->used_pools = create_list(sizeof(struct r_descriptor_pool_t), 16);

                for(uint32_t pool_index = 0; pool_index < list->free_pools.size; pool_index++)
                {
                    struct r_descriptor_pool_t *pool = get_list_element(&list->free_pools, add_list_element(&list->free_pools, NULL));
                    vkCreateDescriptorPool(r_device.device, &descriptor_pool_create_info, NULL, &pool->descriptor_pool);
                    pool->exhaustion_event = r_CreateEvent();
                    pool->set_count = descriptor_pool_create_info.maxSets;
                    pool->free_count = pool->set_count;
                }
            }

            VkPipelineShaderStageCreateInfo *shader_stage = (VkPipelineShaderStageCreateInfo *)pipeline_create_info.pStages + shader_index;
            pipeline_create_info.stageCount++;
            shader_stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stage->pNext = NULL;
            shader_stage->flags = 0;
            shader_stage->pSpecializationInfo = NULL;
            shader_stage->stage = shader->stage;
            shader_stage->module = shader->module;
            shader_stage->pName = "main";
            shader_stage->pSpecializationInfo = NULL;
        }

        vkCreatePipelineLayout(r_device.device, &pipeline_layout_create_info, NULL, &pipelines[subpass_index].layout);
        struct r_pipeline_description_t pipeline_description;
        /* copy the pipeline description to a local variable so zero initialized fields can be filled
        without affecting the description passed in. */
        memcpy(&pipeline_description, subpass_description->pipeline_description, sizeof(struct r_pipeline_description_t));

//        VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
        pipeline_description.vertex_input_state = &input_state_create_info;

        input_state_create_info.vertexBindingDescriptionCount = pipeline->vertex_shader->vertex_binding_count;
        input_state_create_info.vertexAttributeDescriptionCount = 0;
        /* first fill in the binding descriptions, and also accumulate how many attribute descriptions we have */
        for(uint32_t binding_index = 0; binding_index < pipeline->vertex_shader->vertex_binding_count; binding_index++)
        {
            struct r_vertex_binding_t *binding = pipeline->vertex_shader->vertex_bindings + binding_index;
            VkVertexInputBindingDescription *binding_description = (VkVertexInputBindingDescription *)input_state_create_info.pVertexBindingDescriptions + binding_index;
            binding_description->binding = binding_index;
            binding_description->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            binding_description->stride = binding->size;

            for(uint32_t attribute_index = 0; attribute_index < binding->attrib_count; attribute_index++)
            {
                struct r_vertex_attrib_t *attrib = binding->attribs + attribute_index;
                VkVertexInputAttributeDescription *attrib_description = (VkVertexInputAttributeDescription *)input_state_create_info.pVertexAttributeDescriptions +
                    input_state_create_info.vertexAttributeDescriptionCount;
                input_state_create_info.vertexAttributeDescriptionCount++;

                attrib_description->binding = binding_index;
                attrib_description->format = attrib->format;
                attrib_description->location = attribute_index;
                attrib_description->offset = attrib->offset;
            }
        }

//        memcpy(&vertex_input_state, pipeline_description.vertex_input_state, sizeof(VkPipelineVertexInputStateCreateInfo));
//        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;


        if(pipeline_description.input_assembly_state)
        {
            memcpy(&input_assembly_state, pipeline_description.input_assembly_state, sizeof(VkPipelineInputAssemblyStateCreateInfo));
            input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        }
        pipeline_description.input_assembly_state = &input_assembly_state;

        if(!pipeline_description.viewport_state)
        {
            /* by default assume outside code will provide viewport and
            scissor rects info */
            pipeline_description.viewport_state = &viewport_state;
        }

        if(pipeline_description.rasterization_state)
        {
            memcpy(&rasterization_state, pipeline_description.rasterization_state, sizeof(VkPipelineRasterizationStateCreateInfo));
            rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

            if(rasterization_state.lineWidth == 0.0)
            {
                rasterization_state.lineWidth = 1.0;
            }

        }
        pipeline_description.rasterization_state = &rasterization_state;


        if(!pipeline_description.multisample_state)
        {
            pipeline_description.multisample_state = &multisample_state;
        }

        if(!pipeline_description.depth_stencil_state)
        {
            /* OpenGL disabled depth testing, by default. Here we'll enable
            it by default if there's a depth-stencil attachment reference in this subpass.
            Stencil test remains disabled by default. */
            pipeline_description.depth_stencil_state = &depth_stencil_state;
            depth_stencil_state.depthTestEnable = subpass_description->depth_stencil_attachment != NULL;
            depth_stencil_state.depthWriteEnable = subpass_description->depth_stencil_attachment != NULL;
        }

        if(!pipeline_description.color_blend_state)
        {
            pipeline_description.color_blend_state = &color_blend_state;
            color_blend_state.attachmentCount = color_attachment_count;
        }

        if(pipeline_description.dynamic_state)
        {
            memcpy(&dynamic_state, pipeline_description.dynamic_state, sizeof(VkPipelineDynamicStateCreateInfo));
            dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        }

        pipeline_description.dynamic_state = &dynamic_state;

        pipeline_create_info.pVertexInputState = pipeline_description.vertex_input_state;
        pipeline_create_info.pInputAssemblyState = pipeline_description.input_assembly_state;
        pipeline_create_info.pTessellationState = pipeline_description.tesselation_state;
        pipeline_create_info.pViewportState = pipeline_description.viewport_state;
        pipeline_create_info.pRasterizationState = pipeline_description.rasterization_state;
        pipeline_create_info.pMultisampleState = pipeline_description.multisample_state;
        pipeline_create_info.pDepthStencilState = pipeline_description.depth_stencil_state;
        pipeline_create_info.pColorBlendState = pipeline_description.color_blend_state;
        pipeline_create_info.pDynamicState = pipeline_description.dynamic_state;
        pipeline_create_info.layout = pipelines[subpass_index].layout;
        pipeline_create_info.renderPass = render_pass->render_pass;
        pipeline_create_info.subpass = subpass_index;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;

        vkCreateGraphicsPipelines(r_device.device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline->pipeline);
    }

    free(subpass_descriptions);
    free(color_attachment_references);
    free(depth_stencil_attachment_references);
    free((VkPipelineColorBlendAttachmentState *)color_blend_state.pAttachments);
    free((VkVertexInputAttributeDescription *)input_state_create_info.pVertexAttributeDescriptions);
    free((VkVertexInputBindingDescription *)input_state_create_info.pVertexBindingDescriptions);
    free((VkAttachmentDescription *)render_pass_create_info.pAttachments);
    free((VkSubpassDescription *)render_pass_create_info.pSubpasses);
    free((VkPipelineShaderStageCreateInfo *)pipeline_create_info.pStages);
    free((VkPipelineLayout *)pipeline_layout_create_info.pSetLayouts);
    free((VkPushConstantRange *)pipeline_layout_create_info.pPushConstantRanges);
    free((VkDescriptorPoolSize *)descriptor_pool_create_info.pPoolSizes);

    return handle;
}

void r_DestroyRenderPass(struct r_render_pass_handle_t handle)
{

}

struct r_render_pass_t *r_GetRenderPassPointer(struct r_render_pass_handle_t handle)
{
    struct r_render_pass_t *render_pass;

    render_pass = get_stack_list_element(&r_device.render_passes, handle.index);
//
//    if(render_pass && !(render_pass->description.attachment_count && render_pass->description.subpass_count))
//    {
//        render_pass = NULL;
//    }

    return render_pass;
}

struct r_descriptor_pool_list_t *r_GetDescriptorPoolListPointer(struct r_pipeline_t *pipeline, VkShaderStageFlagBits stage)
{
    return NULL;
}

VkDescriptorSet r_AllocateDescriptorSet(union r_command_buffer_h command_buffer, struct r_pipeline_t *pipeline, VkShaderStageFlagBits stage)
{
    uint32_t list_index;
    struct r_descriptor_pool_list_t *list;
    struct r_descriptor_pool_t *descriptor_pool;
    struct r_descriptor_pool_t *depleted_descriptor_pool;
    struct r_shader_t *shader;
    VkDescriptorSetAllocateInfo allocate_info;
    VkDescriptorSet descriptor_set;
//    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    VkResult result;

    switch(stage)
    {
        case VK_SHADER_STAGE_VERTEX_BIT:
            list_index = 0;
            shader = pipeline->vertex_shader;
        break;

        case VK_SHADER_STAGE_FRAGMENT_BIT:
            list_index = 1;
            shader = pipeline->fragment_shader;
        break;
    }

    list = &pipeline->pool_lists[list_index];

//    if(list->current_pool == 0xffffffff)
//    {
//        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//        descriptor_pool_create_info.
//
//        descriptor_pool = alloca(sizeof(struct r_descriptor_pool_t));
//    }

    /* many threads may use this pipeline to submit draw commands concurrently */
    SDL_AtomicLock(&list->spinlock);

    descriptor_pool = get_list_element(&list->free_pools, list->current_pool);
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.descriptorPool = descriptor_pool->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &shader->descriptor_set_layout;
    result = vkAllocateDescriptorSets(r_device.device, &allocate_info, &descriptor_set);
    descriptor_pool->free_count--;
    if(!descriptor_pool->free_count)
    {
        if(list->used_pools.cursor)
        {
            /* do some recycling only when we have spent a whole descriptor pool */
            depleted_descriptor_pool = get_list_element(&list->used_pools, list->next_pool_check);
            result = vkGetEventStatus(r_device.device, depleted_descriptor_pool->exhaustion_event);
            if(result == VK_EVENT_SET)
            {
                vkResetDescriptorPool(r_device.device, depleted_descriptor_pool->descriptor_pool, 0);
                depleted_descriptor_pool->free_count = depleted_descriptor_pool->set_count;
                vkResetEvent(r_device.device, depleted_descriptor_pool->exhaustion_event);
                add_list_element(&list->free_pools, depleted_descriptor_pool);
                remove_list_element(&list->used_pools, list->next_pool_check);
            }

            if(list->used_pools.cursor)
            {
                list->next_pool_check = (list->next_pool_check + 1) % list->used_pools.cursor;
            }
            else
            {
                list->next_pool_check = 0;
            }
        }

        r_AppendEvent(command_buffer, descriptor_pool->exhaustion_event);
        add_list_element(&list->used_pools, descriptor_pool);
        remove_list_element(&list->free_pools, list->current_pool);

        if(list->free_pools.cursor)
        {
            list->current_pool = (list->current_pool + 1) % list->free_pools.cursor;
        }
        else
        {
            list->current_pool = 0;
        }
    }

    SDL_AtomicUnlock(&list->spinlock);

    return descriptor_set;
}

void r_ResetPipelineDescriptorPools(struct r_pipeline_t *pipeline)
{

}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_framebuffer_handle_t r_CreateFramebuffer(struct r_framebuffer_description_t *description)
{
    struct r_framebuffer_handle_t handle = R_INVALID_FRAMEBUFFER_HANDLE;
    struct r_framebuffer_t *framebuffer;
    struct r_texture_description_t texture_description = {};
    struct r_texture_handle_t *textures;
    struct r_texture_t *texture;
    VkFramebufferCreateInfo framebuffer_create_info = {};
    VkAttachmentDescription *attachment_description;

    if(description)
    {
        handle.index = add_stack_list_element(&r_device.framebuffers, NULL);
        framebuffer = get_stack_list_element(&r_device.framebuffers, handle.index);
        framebuffer->buffers = calloc(description->frame_count, sizeof(VkFramebuffer));
        framebuffer->textures = calloc(description->attachment_count * description->frame_count, sizeof(struct r_texture_handle_t));
//        framebuffer->attachment_count = description->attachment_count;

        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = description->render_pass->render_pass;
        framebuffer_create_info.attachmentCount = description->attachment_count;
        framebuffer_create_info.pAttachments = alloca(sizeof(VkImageView) * description->attachment_count);
        framebuffer_create_info.width = description->width;
        framebuffer_create_info.height = description->height;
        framebuffer_create_info.layers = 1;

        texture_description.extent.width = description->width;
        texture_description.extent.height = description->height;
        texture_description.extent.depth = 1;
        texture_description.array_layers = 1;
        texture_description.image_type = VK_IMAGE_TYPE_2D;
        texture_description.mip_levels = 1;
        texture_description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texture_description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texture_description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texture_description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        texture_description.sampler_params.min_filter = VK_FILTER_NEAREST;
        texture_description.sampler_params.mag_filter = VK_FILTER_NEAREST;

        for(uint32_t frame_index = 0; frame_index < description->frame_count; frame_index++)
        {
            textures = framebuffer->textures + frame_index * description->attachment_count;
            for(uint32_t attachment_index = 0; attachment_index < description->attachment_count; attachment_index++)
            {
                attachment_description = description->attachments + attachment_index;
                texture_description.format = attachment_description->format;
                textures[attachment_index] = r_CreateTexture(&texture_description);
                texture = r_GetTexturePointer(textures[attachment_index]);
                *(VkImageView *)(framebuffer_create_info.pAttachments + attachment_index) = texture->image_view;
            }

            vkCreateFramebuffer(r_device.device, &framebuffer_create_info, NULL, framebuffer->buffers + frame_index);
        }
    }

    return handle;
}

void r_DestroyFramebuffer(struct r_framebuffer_handle_t handle)
{

}

struct r_framebuffer_t *r_GetFramebufferPointer(struct r_framebuffer_handle_t handle)
{
    struct r_framebuffer_t *framebuffer;
//
    framebuffer = get_stack_list_element(&r_device.framebuffers, handle.index);
//    if(framebuffer && framebuffer->description.attachment_count == 0xff)
//    {
//        framebuffer = NULL;
//    }
//
    return framebuffer;
}

void r_PresentFramebuffer(struct r_framebuffer_handle_t handle)
{
    VkPresentInfoKHR present_info = {};
    struct r_submit_info_t submit_info = {.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    union r_command_buffer_h command_buffer;
    struct r_framebuffer_t *framebuffer;
    struct r_texture_t *texture;
    struct r_swapchain_t *swapchain;
    struct r_image_handle_t swapchain_image;
//    struct r_image_t *texture_image_ptr;
//    struct r_image_t *swapchain_image_ptr;

    framebuffer = r_GetFramebufferPointer(handle);
    texture = r_GetTexturePointer(framebuffer->textures[framebuffer->current_buffer]);

    r_NextImage(r_device.swapchain);
    swapchain = r_GetSwapchainPointer(r_device.swapchain);
    swapchain_image = swapchain->images[swapchain->current_image];

//    texture_image_ptr = r_GetImagePointer(texture->image);
//    swapchain_image_ptr = r_GetImagePointer(swapchain_image);

    command_buffer = r_AllocateCommandBuffer();

    r_vkBeginCommandBuffer(command_buffer);
    r_vkCmdSetImageLayout(command_buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    r_vkCmdSetImageLayout(command_buffer, swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    r_vkCmdBlitImage(command_buffer, texture->image, swapchain_image, NULL);
    r_vkCmdSetImageLayout(command_buffer, texture->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    r_vkCmdSetImageLayout(command_buffer, swapchain_image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    r_vkEndCommandBuffer(command_buffer);

    submit_info.command_buffer_count = 1;
    submit_info.command_buffers = &command_buffer;
    vkResetFences(r_device.device, 1, &r_device.transfer_fence);
    r_vkQueueSubmit(r_device.transfer_queue->queue, 1, &submit_info, r_device.transfer_fence);
    vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->swapchain;
    present_info.pImageIndices = &swapchain->current_image;
    vkQueuePresentKHR(r_device.present_queue->queue, &present_info);
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_swapchain_handle_t r_CreateSwapchain(VkSwapchainCreateInfoKHR *description)
{
    struct r_swapchain_handle_t handle = R_INVALID_SWAPCHAIN_HANDLE;
    struct r_swapchain_t *swapchain;
    VkSwapchainCreateInfoKHR *description_copy;
    VkImageCreateInfo image_description = {};
    VkSurfaceCapabilitiesKHR surface_capabilites;
    VkSurfaceFormatKHR surface_format;
    VkImage *swapchain_images;
    struct r_image_t *image;
    uint32_t swapchain_image_count;

    handle.index = add_stack_list_element(&r_device.swapchains, NULL);
    swapchain = get_stack_list_element(&r_device.swapchains, handle.index);

    description_copy = alloca(sizeof(VkSwapchainCreateInfoKHR));
    memcpy(description_copy, description, sizeof(VkSwapchainCreateInfoKHR));
    description = description_copy;

    description->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    if(!description->minImageCount)
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_device.physical_device, description->surface, &surface_capabilites);
        description->minImageCount = surface_capabilites.minImageCount;
    }

    if(!description->imageArrayLayers)
    {
        description->imageArrayLayers = 1;
    }

    if(!description->imageUsage)
    {
        description->imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    description->imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    if(!description->imageFormat)
    {
        vkGetPhysicalDeviceSurfaceFormatsKHR(r_device.physical_device, description->surface, &(uint32_t){1}, &surface_format);
        description->imageFormat = surface_format.format;
        description->imageColorSpace = surface_format.colorSpace;
    }

    if(!description->preTransform)
    {
        description->preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    if(!description->compositeAlpha)
    {
        description->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    vkCreateSwapchainKHR(r_device.device, description, NULL, &swapchain->swapchain);
    swapchain->image_count = description->minImageCount;
    swapchain->images = calloc(sizeof(struct r_image_handle_t), swapchain->image_count);
    swapchain_images = alloca(sizeof(VkImage) * swapchain->image_count);

    vkGetSwapchainImagesKHR(r_device.device, swapchain->swapchain, &swapchain_image_count, NULL);
    vkGetSwapchainImagesKHR(r_device.device, swapchain->swapchain, &swapchain_image_count, swapchain_images);

    image_description.imageType = VK_IMAGE_TYPE_2D;
    image_description.format = description->imageFormat;
    image_description.extent.width = description->imageExtent.width;
    image_description.extent.height = description->imageExtent.height;
    image_description.extent.depth = 1;

    for(uint32_t image_index = 0; image_index < swapchain_image_count; image_index++)
    {
        swapchain->images[image_index] = r_CreateImage(&image_description);
        image = r_GetImagePointer(swapchain->images[image_index]);
        vkDestroyImage(r_device.device, image->image, NULL);
        image->image = swapchain_images[image_index];
    }

    return handle;
}

void r_DestroySwapchain(struct r_swapchain_handle_t handle)
{

}

struct r_swapchain_t *r_GetSwapchainPointer(struct r_swapchain_handle_t handle)
{
    struct r_swapchain_t *swapchain = NULL;
    swapchain = get_stack_list_element(&r_device.swapchains, handle.index);
    return swapchain;
}

void r_NextImage(struct r_swapchain_handle_t handle)
{
    struct r_swapchain_t *swapchain;
    uint32_t image_index;
    swapchain = r_GetSwapchainPointer(handle);
    r_LockQueue(r_device.transfer_queue);
    vkResetFences(r_device.device, 1, &r_device.transfer_fence);
    vkAcquireNextImageKHR(r_device.device, swapchain->swapchain, 0xffffffffffffffff, VK_NULL_HANDLE, r_device.transfer_fence, &image_index);
    vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);
    swapchain->current_image = image_index;
    r_UnlockQueue(r_device.transfer_queue);
}

void r_ResizeSwapchain(struct r_swapchain_handle_t swapchain, uint32_t width, uint32_t height)
{
//    VkSwapchainCreateInfoKHR swapchain_create_info = {};
//    vkDestroySurfaceKHR(r_device.instance, r_device.surface, NULL);
//
//
//    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//    swapchain_create_info.surface =
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_LockQueue(struct r_queue_t *queue)
{
    SDL_AtomicLock(&queue->spinlock);
}

void r_UnlockQueue(struct r_queue_t *queue)
{
    SDL_AtomicUnlock(&queue->spinlock);
}

//void r_QueueSubmit(struct r_queue_t *queue, uint32_t submit_count, VkSubmitInfo *submit_info, VkFence fence)
//{
//    SDL_AtomicLock(&queue->spinlock);
//    vkQueueSubmit(queue->queue, submit_count, submit_info, fence);
//    SDL_AtomicUnlock(&queue->spinlock);
//}

VkQueue r_GetDrawQueue()
{
    return r_device.draw_queue->queue;
}

VkPhysicalDeviceLimits *r_GetDeviceLimits()
{
    return &r_device.physical_device_limits;
}

//VkBuffer r_GetStagingBuffer()
//{
//    return r_device.staging.base_staging_buffer;
//}

//uint32_t r_GetStagingMemorySize()
//{
//    return r_device.staging.staging_memory_size;
//}

/*
=================================================================
=================================================================
=================================================================
*/

VkFence r_CreateFence()
{
    VkFenceCreateInfo fence_create_info = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence;
    vkCreateFence(r_device.device, &fence_create_info, NULL, &fence);
    return fence;
}

VkEvent r_CreateEvent()
{
    VkEventCreateInfo event_create_info = {.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};
    VkEvent event;
    vkCreateEvent(r_device.device, &event_create_info, NULL, &event);
    return event;
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_light_handle_t r_CreateLight(vec3_t *position, float radius, vec3_t *color)
{
    struct r_light_handle_t handle = R_INVALID_LIGHT_HANDLE;
    struct r_light_t *light;

    handle.index = add_stack_list_element(&r_renderer.lights, NULL);

    light = (struct r_light_t*)get_stack_list_element(&r_renderer.lights, handle.index);
    light->pos_radius.comps[0] = position->comps[0];
    light->pos_radius.comps[1] = position->comps[1];
    light->pos_radius.comps[2] = position->comps[2];
    light->pos_radius.comps[3] = radius;
    light->color.comps[0] = color->comps[0];
    light->color.comps[1] = color->comps[1];
    light->color.comps[2] = color->comps[2];
    light->color.comps[3] = 0.0;

    return handle;
}

struct r_light_t *r_GetLightPointer(struct r_light_handle_t handle)
{
    struct r_light_t *light;
    light = (struct r_light_t*)get_stack_list_element(&r_renderer.lights, handle.index);

    if(light && light->pos_radius.comps[3] < 0.0)
    {
        light = NULL;
    }

    return light;
}

void r_DestroyLight(struct r_light_handle_t handle)
{
    struct r_light_t *light;

    light = r_GetLightPointer(handle);

    if(light)
    {
        light->pos_radius.comps[3] = -1.0;
        remove_stack_list_element(&r_renderer.lights, handle.index);
    }
}

/*
=================================================================================
=================================================================================
=================================================================================
*/

void r_vkBeginCommandBuffer(union r_command_buffer_h command_buffer)
{
    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    struct r_command_buffer_t *command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    command_buffer_ptr->events.cursor = 0;
    vkBeginCommandBuffer(command_buffer_ptr->command_buffer, &begin_info);
}

void r_AppendEvent(union r_command_buffer_h command_buffer, VkEvent event)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    if(command_buffer_ptr)
    {
        add_list_element(&command_buffer_ptr->events, &event);
    }
}

void r_vkCmdBindPipeline(union r_command_buffer_h command_buffer, VkPipelineBindPoint bind_point, VkPipeline pipeline)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdBindPipeline(command_buffer_ptr->command_buffer, bind_point, pipeline);
}

void r_vkCmdBindVertexBuffers(union r_command_buffer_h command_buffer, uint32_t first_binding, uint32_t binding_count, VkBuffer *buffers, VkDeviceSize *offsets)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdBindVertexBuffers(command_buffer_ptr->command_buffer, first_binding, binding_count, buffers, offsets);
}

void r_vkCmdBeginRenderPass(union r_command_buffer_h command_buffer, VkRenderPassBeginInfo *begin_info, VkSubpassContents subpass_contents)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdBeginRenderPass(command_buffer_ptr->command_buffer, begin_info, subpass_contents);
}

void r_vkCmdEndRenderPass(union r_command_buffer_h command_buffer)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdEndRenderPass(command_buffer_ptr->command_buffer);
}

void r_vkCmdSetViewport(union r_command_buffer_h command_buffer, uint32_t first_viewport, uint32_t viewport_count, VkViewport *viewports)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdSetViewport(command_buffer_ptr->command_buffer, first_viewport, viewport_count, viewports);
}

void r_vkCmdSetScissor(union r_command_buffer_h command_buffer, uint32_t first_scissor, uint32_t scissor_count, VkRect2D *scissors)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdSetScissor(command_buffer_ptr->command_buffer, first_scissor, scissor_count, scissors);
}

void r_vkCmdSetLineWidth(union r_command_buffer_h command_buffer, float width)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdSetLineWidth(command_buffer_ptr->command_buffer, width);
}

void r_vkCmdPushConstants(union r_command_buffer_h command_buffer, VkPipelineLayout layout, VkPipelineStageFlags stage_flags, uint32_t offset, uint32_t size, void *data)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdPushConstants(command_buffer_ptr->command_buffer, layout, stage_flags, offset, size, data);
}

void r_vkCmdDraw(union r_command_buffer_h command_buffer, uint32_t count, uint32_t instance_count, uint32_t first, uint32_t first_instance)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdDraw(command_buffer_ptr->command_buffer, count, instance_count, first, first_instance);
}

void r_vkCmdCopyBufferToImage(union r_command_buffer_h command_buffer, VkBuffer src_buffer, VkImage dst_image, VkImageLayout dst_layout, uint32_t region_count, VkBufferImageCopy *regions)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdCopyBufferToImage(command_buffer_ptr->command_buffer, src_buffer, dst_image, dst_layout, region_count, regions);
}

void r_vkCmdBindDescriptorSets(union r_command_buffer_h command_buffer, VkPipelineBindPoint bind_point, VkPipelineLayout layout, uint32_t first_set, uint32_t set_count, VkDescriptorSet *descriptor_sets, uint32_t dynamic_offset_count, uint32_t *dynamic_offsets)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdBindDescriptorSets(command_buffer_ptr->command_buffer, bind_point, layout, first_set, set_count, descriptor_sets, dynamic_offset_count, dynamic_offsets);
}

void r_vkCmdBlitImage(union r_command_buffer_h command_buffer, struct r_image_handle_t src_handle, struct r_image_handle_t dst_handle, VkImageBlit *blit)
{
    struct r_image_t *src_image;
    struct r_image_t *dst_image;
    VkImageCreateInfo *src_description;
    VkImageCreateInfo *dst_description;
    struct r_command_buffer_t *command_buffer_ptr;
    uint32_t old_src_layout;
    uint32_t old_dst_layout;
//    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    src_image = r_GetImagePointer(src_handle);
    src_description = r_GetImageDescriptionPointer(src_handle);
    dst_image = r_GetImagePointer(dst_handle);
    dst_description = r_GetImageDescriptionPointer(dst_handle);

    if(!blit)
    {
        blit = alloca(sizeof(VkImageBlit));

        blit->srcOffsets[0].x = 0;
        blit->srcOffsets[0].y = 0;
        blit->srcOffsets[0].z = 0;
        blit->srcOffsets[1].x = src_description->extent.width;
        blit->srcOffsets[1].y = src_description->extent.height;
        blit->srcOffsets[1].z = src_description->extent.depth;
        blit->srcSubresource.aspectMask = src_image->aspect_mask;
        blit->srcSubresource.baseArrayLayer = 0;
        blit->srcSubresource.layerCount = 1;
        blit->srcSubresource.mipLevel = 0;

        blit->dstOffsets[0].x = 0;
        blit->dstOffsets[0].y = 0;
        blit->dstOffsets[0].z = 0;
        blit->dstOffsets[1].x = dst_description->extent.width;
        blit->dstOffsets[1].y = dst_description->extent.height;
        blit->dstOffsets[1].z = dst_description->extent.depth;
        blit->dstSubresource.aspectMask = dst_image->aspect_mask;
        blit->dstSubresource.baseArrayLayer = 0;
        blit->dstSubresource.layerCount = 1;
        blit->dstSubresource.mipLevel = 0;
    }

    old_src_layout = src_image->current_layout;
    old_dst_layout = dst_image->current_layout;

    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);

    r_vkCmdSetImageLayout(command_buffer, src_handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    r_vkCmdSetImageLayout(command_buffer, dst_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdBlitImage(command_buffer_ptr->command_buffer, src_image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                              dst_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, blit, VK_FILTER_NEAREST);
    r_vkCmdSetImageLayout(command_buffer, src_handle, old_src_layout);
    r_vkCmdSetImageLayout(command_buffer, dst_handle, old_dst_layout);
}

void r_vkCmdPipelineBarrier(union r_command_buffer_h command_buffer, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask, VkDependencyFlags dependency_flags,
                            uint32_t memory_barrier_count, VkMemoryBarrier *memory_barriers,
                            uint32_t buffer_barrier_count, VkBufferMemoryBarrier *buffer_memory_barriers,
                            uint32_t image_barrier_count, VkImageMemoryBarrier *image_memory_barriers)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdPipelineBarrier(command_buffer_ptr->command_buffer, src_stage_mask, dst_stage_mask, dependency_flags, memory_barrier_count, memory_barriers,
                         buffer_barrier_count, buffer_memory_barriers, image_barrier_count, image_memory_barriers);
}

void r_vkCmdSetImageLayout(union r_command_buffer_h command_buffer, struct r_image_handle_t handle, uint32_t new_layout)
{
    VkImageMemoryBarrier memory_barrier = {};
    struct r_image_t *image;
    VkImageCreateInfo *description;

    image = r_GetImagePointer(handle);
    description = r_GetImageDescriptionPointer(handle);

    if(new_layout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        if(r_IsDepthStencilFormat(description->format))
        {
            new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else
        {
            new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
    }

    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.pNext = NULL;
    /* all writes to this texture must become visible before the layout can change */
    memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    /* new layout in place, now the memory becomes accessible */
    memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    memory_barrier.oldLayout = image->current_layout;
    memory_barrier.newLayout = new_layout;
    memory_barrier.srcQueueFamilyIndex = r_device.graphics_queue_family;
    memory_barrier.dstQueueFamilyIndex = r_device.graphics_queue_family;
    memory_barrier.image = image->image;
    memory_barrier.subresourceRange.aspectMask = image->aspect_mask;
    memory_barrier.subresourceRange.baseArrayLayer = 0;
    memory_barrier.subresourceRange.baseMipLevel = 0;
    memory_barrier.subresourceRange.layerCount = 1;
    memory_barrier.subresourceRange.levelCount = 1;
    r_vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,0, 0, NULL, 0, NULL, 1, &memory_barrier);
    image->current_layout = new_layout;
}

void r_vkCmdUpdateBuffer(union r_command_buffer_h command_buffer, struct r_buffer_h buffer, uint32_t offset, uint32_t size, void *data)
{
    struct r_command_buffer_t *command_buffer_ptr;
    struct r_buffer_t *buffer_ptr;
    struct r_staging_buffer_t *staging_buffer;
    uint32_t region_count;
    VkBufferCopy *regions;
    VkBufferCopy *region;
    void *memory;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    buffer_ptr = r_GetBufferPointer(buffer);

    memory = r_GetBufferChunkMappedMemory(buffer_ptr->memory);
    if(memory)
    {
        /* good, memory is mapped, so write directly to it */
        memcpy((char *)memory + offset, data, size);
    }
    else
    {
        /* well, memory is not mapped, so let's see what we got here... */
        if(size <= 16384)
        {
            /* data is small, copy it directly */
            vkCmdUpdateBuffer(command_buffer_ptr->command_buffer, buffer_ptr->buffer, (VkDeviceSize){offset}, (VkDeviceSize){size}, data);
        }
        else
        {
            /* we have some mighty chonky data, so do a full blown copy using staging buffers */
            region_count = size / r_device.staging.staging_buffer_size;
            regions = alloca(sizeof(VkBufferCopy) * (1 + region_count));
            for(uint32_t region_index = 0; region_index < region_count && size; region_count++)
            {
                staging_buffer = r_AllocateStagingBuffer(command_buffer);
                memcpy(staging_buffer->memory, data, r_device.staging.staging_buffer_size);
                region = regions + region_index;
                region->srcOffset = staging_buffer->offset;
                region->dstOffset = offset + r_device.staging.staging_buffer_size * region_index;
                region->size = r_device.staging.staging_buffer_size;
                size -= r_device.staging.staging_buffer_size;
                data = (char *)data + r_device.staging.staging_buffer_size * region_index;
            }

            staging_buffer = r_AllocateStagingBuffer(command_buffer);
            memcpy(staging_buffer->memory, data, size);
            region = regions + region_count;
            region->srcOffset = staging_buffer->offset;
            region->dstOffset = offset + r_device.staging.staging_buffer_size * region_count;
            region->size = size;
            vkCmdCopyBuffer(command_buffer_ptr->command_buffer, r_device.staging.base_staging_buffer, buffer_ptr->buffer, 1 + region_count, regions);
        }
    }
}

void r_vkUpdateDescriptorSets(uint32_t descriptor_write_count, VkWriteDescriptorSet *descriptor_writes)
{
    vkUpdateDescriptorSets(r_device.device, descriptor_write_count, (const VkWriteDescriptorSet *)descriptor_writes, 0, NULL);
}

void r_UpdateUniformBufferDescriptorSet(VkDescriptorSet descriptor_set, uint32_t dst_binding, VkBuffer uniform_buffer, uint32_t offset, uint32_t range)
{
    VkWriteDescriptorSet descriptor_set_write = {};
    VkDescriptorBufferInfo buffer_info = {};

    buffer_info.buffer = uniform_buffer;
    buffer_info.offset = offset;
    buffer_info.range = range;

    descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.dstSet = descriptor_set;
    descriptor_set_write.dstBinding = dst_binding;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(r_device.device, 1, &descriptor_set_write, 0, NULL);
}

void r_UpdateCombinedImageSamplerDescriptorSet(VkDescriptorSet descriptor_set, uint32_t dst_binding, struct r_texture_handle_t texture)
{
    VkWriteDescriptorSet descriptor_set_write = {};
    VkDescriptorImageInfo image_info = {};
    struct r_texture_t *texture_ptr;

    texture_ptr = r_GetTexturePointer(texture);
    image_info.sampler = texture_ptr->sampler;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = texture_ptr->image_view;

    descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_write.descriptorCount = 1;
    descriptor_set_write.dstSet = descriptor_set;
    descriptor_set_write.dstBinding = dst_binding;
    descriptor_set_write.dstArrayElement = 0;
    descriptor_set_write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(r_device.device, 1, &descriptor_set_write, 0, NULL);
}

void r_vkEndCommandBuffer(union r_command_buffer_h command_buffer)
{
    VkEvent *event;
    struct r_command_buffer_t *command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdSetEvent(command_buffer_ptr->command_buffer, command_buffer_ptr->complete_event, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    for(uint32_t event_index = 0; event_index < command_buffer_ptr->events.cursor; event_index++)
    {
        event = get_list_element(&command_buffer_ptr->events, event_index);
        vkCmdSetEvent(command_buffer_ptr->command_buffer, *event, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    }
    vkEndCommandBuffer(command_buffer_ptr->command_buffer);
}

VkResult r_vkQueueSubmit(VkQueue queue, uint32_t submit_count, struct r_submit_info_t *submit_info, VkFence fence)
{
    struct r_command_buffer_t *command_buffer_ptr;
    struct r_submit_info_t *submit;
    uint32_t command_buffer_index;

    for(uint32_t submit_index = 0; submit_index < submit_count; submit_index++)
    {
        submit = submit_info + submit_index;
        for(uint32_t index = 0; index < submit_info[submit_index].command_buffer_count; index++)
        {
            command_buffer_ptr = r_GetCommandBufferPointer(submit->command_buffers[index]);
            add_list_element(&r_device.draw_command_pool.pending_command_buffers, &command_buffer_index);
            submit->command_buffers[command_buffer_index].command_buffer = command_buffer_ptr->command_buffer;
        }
    }

    return vkQueueSubmit(queue, submit_count, (const VkSubmitInfo *)submit_info, fence);
}

void r_vkResetFences(uint32_t fence_count, VkFence *fences)
{
    vkResetFences(r_device.device, fence_count, (const VkFence *)fences);
}

void r_vkWaitForFences(uint32_t fence_count, VkFence *fences, VkBool32 wait_all, uint64_t time_out)
{
    vkWaitForFences(r_device.device, fence_count, (const VkFence *)fences, wait_all, time_out);
}

VkResult r_vkGetEventStatus(VkEvent event)
{
    return vkGetEventStatus(r_device.device, event);
}

void r_vkSetEvent(VkEvent event)
{
    vkSetEvent(r_device.device, event);
}

void r_vkResetEvent(VkEvent event)
{
    vkResetEvent(r_device.device, event);
}







