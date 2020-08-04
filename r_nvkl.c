#include "r_nvkl.h"
#include "lib/dstuff/ds_mem.h"
#include "lib/dstuff/ds_path.h"
//#include "r_vk.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_ringbuffer.h"
#include "lib/dstuff/ds_list.h"
#include <stdlib.h>
#include <string.h>
#include "lib/stb/stb_image.h"
#include "lib/SDL/include/SDL2/SDL_thread.h"
#include "lib/SDL/include/SDL2/SDL_atomic.h"
#include "lib/SDL/include/SDL2/SDL_vulkan.h"

#define R_SWAPCHAIN_IMAGE_COUNT 2

struct
{
    VkPhysicalDevice physical_device;
    VkInstance instance;
    VkDevice device;
    
    struct 
    {
        uint32_t current_image;
        struct r_image_handle_t *images;
        VkSwapchainKHR swapchain;
        VkSurfaceKHR surface;
        uint32_t surface_width;
        uint32_t surface_height;
        VkFence fence;
    }swapchain;

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

    VkFence draw_fence;
    VkFence transfer_fence;
    
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
    struct stack_list_t framebuffer_descriptions;
    struct stack_list_t framebuffers;
    struct stack_list_t render_passes;
    struct stack_list_t pipelines;
    struct stack_list_t samplers;
    struct stack_list_t shaders;
    struct stack_list_t image_heaps;
    struct stack_list_t buffer_heaps;
    struct stack_list_t typeless_heaps;

    struct r_heap_h texture_heap;
    struct r_heap_h buffer_heap;

    VkPhysicalDeviceLimits physical_device_limits;

}r_device;

uint32_t r_supports_push_descriptors = 0;
PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSet = NULL;

VkInstance r_CreateInstance()
{
    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.flags = 0;
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = (const char *[]){"VK_LAYER_KHRONOS_validation"};
    instance_create_info.enabledExtensionCount = 2;
    instance_create_info.ppEnabledExtensionNames = (const char *[]){VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    vkCreateInstance(&instance_create_info, NULL, &r_device.instance);
    
    return r_device.instance;
}

VkInstance r_GetInstance()
{
    return r_device.instance;
}

void r_DestroyInstance()
{
    
}

VkDevice r_CreateDevice(VkSurfaceKHR surface)
{
    uint32_t physical_device_count = 1;
    uint32_t queue_family_property_count = 0;
    uint32_t present_supported = 0;

    vkEnumeratePhysicalDevices(r_device.instance, &physical_device_count, &r_device.physical_device);
    VkQueueFamilyProperties *queue_family_properties;
    vkGetPhysicalDeviceQueueFamilyProperties(r_device.physical_device, &queue_family_property_count, NULL);
    queue_family_properties = mem_Calloc(sizeof(VkQueueFamilyProperties), queue_family_property_count);
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
    mem_Free(queue_family_properties);

    for(uint32_t i = 0; i < queue_family_property_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(r_device.physical_device, i, surface, &present_supported);
        
        if(present_supported)
        {
            r_device.present_queue_family = i;
            break;
        }
    }

    VkDeviceQueueCreateInfo *queue_create_info;
    uint32_t queue_create_info_count = 1 + (r_device.graphics_queue_family != r_device.present_queue_family);
    queue_create_info = mem_Calloc(sizeof(VkDeviceQueueCreateInfo), queue_create_info_count);
    float *queue_priorities = mem_Calloc(sizeof(float), r_device.graphics_queue_count);

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

    VkPhysicalDeviceFeatures features = {};
    features.fillModeNonSolid = VK_TRUE;
    features.wideLines = VK_TRUE;

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = NULL;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = queue_create_info_count;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = (const char *[]){VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = NULL;
    device_create_info.pEnabledFeatures = &features;
    VkResult result = vkCreateDevice(r_device.physical_device, &device_create_info, NULL, &r_device.device);

    mem_Free(queue_create_info);
    mem_Free(queue_priorities);

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
    
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = NULL;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = r_device.graphics_queue_family;
    vkCreateCommandPool(r_device.device, &command_pool_create_info, NULL, &r_device.draw_command_pool.command_pool);
    r_device.draw_command_pool.command_buffers = create_stack_list(sizeof(struct r_command_buffer_t), 128);
    r_device.draw_command_pool.pending_command_buffers = create_list(sizeof(uint32_t), 128);
    
    r_device.transfer_fence = r_CreateFence();
    
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
    r_device.staging.staging_buffers = mem_Calloc(sizeof(struct r_staging_buffer_t), r_device.staging.staging_buffer_count);
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
    r_device.framebuffer_descriptions = create_stack_list(sizeof(struct r_framebuffer_description_t), 16);
    r_device.samplers = create_stack_list(sizeof(struct r_sampler_t), 16);
    r_device.shaders = create_stack_list(sizeof(struct r_shader_t), 128);
    r_device.render_passes = create_stack_list(sizeof(struct r_render_pass_t), 16);
    r_device.pipelines = create_stack_list(sizeof(struct r_pipeline_t), 64);
    r_device.image_heaps = create_stack_list(sizeof(struct r_image_heap_t), 16);
    r_device.buffer_heaps = create_stack_list(sizeof(struct r_buffer_heap_t), 16);

    VkFormat *formats = (VkFormat []){
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_R32_UINT,
    };

    r_device.buffer_heap = r_CreateBufferHeap(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 33554432);
    r_device.texture_heap = r_CreateImageHeap(formats, 4, 268435456);
//    r_device.texture_heap = r_CreateImageHeap(formats, 4, 67108864);

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(r_device.physical_device, &physical_device_properties);
    r_device.physical_device_limits = physical_device_properties.limits;
    
    r_SetSwapchainSurface(surface);
    r_CreateDefaultTexture();
    
    return r_device.device;
}

VkDevice r_GetDevice()
{
    return r_device.device;
}

void r_DestroyDevice()
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
        default:
            return &r_device.typeless_heaps;
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
        heap->move_chunks = create_list(sizeof(struct r_chunk_move_t), 128);
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

    supported_formats = mem_Calloc(sizeof(VkFormat), format_count);

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
    heap->supported_formats = supported_formats;
    heap->supported_format_count = supported_formats_count;
//    memcpy(heap->supported_formats, supported_formats, sizeof(VkFormat) * supported_formats_count);

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

int32_t r_SortChunks(void *a, void *b)
{
    struct r_chunk_t *chunk_a = a;
    struct r_chunk_t *chunk_b = b;
    return (int32_t)(chunk_a->start - chunk_b->start);
}

void r_DefragHeap(struct r_heap_h handle)
{
    struct r_heap_t *heap;
    struct r_chunk_t *first_chunk;
    struct r_chunk_t *second_chunk;
    heap = r_GetHeapPointer(handle);
    
    if(heap)
    {
        uint32_t total_defragd = 0;
        uint32_t prev_free_chunks = heap->free_chunks.cursor;
    
        qsort_list(&heap->free_chunks, r_SortChunks);
        
        for(uint32_t first_index = 0; first_index < heap->free_chunks.cursor; first_index++)
        {
            first_chunk = get_list_element(&heap->free_chunks, first_index);
            
            for(uint32_t second_index = first_index + 1; second_index < heap->free_chunks.cursor; second_index++)
            {
                second_chunk = get_list_element(&heap->free_chunks, second_index);
                
                if(first_chunk->start + first_chunk->size != second_chunk->start - second_chunk->align)
                {
                    first_index = second_index - 1;
                    break;
                } 
                total_defragd += second_chunk->size + second_chunk->align;
                first_chunk->size += second_chunk->size + second_chunk->align;
                second_chunk->size = 0;
            }
        }
        
        for(uint32_t chunk_index = 0; chunk_index < heap->free_chunks.cursor; chunk_index++)
        {
            first_chunk = get_list_element(&heap->free_chunks, chunk_index);
            
            if(!first_chunk->size)
            {
                remove_list_element(&heap->free_chunks, chunk_index);
                chunk_index--;
            }
        }
//        printf("coalesced %d bytes, went from %d to %d free chunks\n", total_defragd, prev_free_chunks, heap->free_chunks.cursor);
    }
}

struct r_chunk_h r_AllocChunk(struct r_heap_h handle, uint32_t size, uint32_t align)
{
    struct r_heap_t *heap;
    struct r_chunk_t *chunk;
    struct r_chunk_t new_chunk;
    struct r_chunk_h chunk_handle = R_INVALID_CHUNK_HANDLE;
    uint32_t chunk_align;
    uint32_t chunk_size;
    uint32_t defragd = 0;
//    uint32_t chunk_start;

    heap = r_GetHeapPointer(handle);

    SDL_AtomicLock(&heap->spinlock);
    
    _try_again:
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
    
    if(chunk_handle.index == R_INVALID_CHUNK_INDEX && !defragd)
    {
        r_DefragHeap(handle);
        defragd = 1;
        goto _try_again;
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

void r_FillBufferChunk(struct r_chunk_h handle, void *data, uint32_t size, uint32_t offset)
{
    struct r_chunk_t *chunk;
    struct r_buffer_heap_t *heap;
    struct r_submit_info_t submit_info = {};
    union r_command_buffer_h command_buffer;
    struct r_command_buffer_t *command_buffer_ptr;
    
    heap = (struct r_buffer_heap_t *)r_GetHeapPointer(handle.heap);
    
    if(heap->mapped_memory)
    {
        /* this heap has its memory mapped, so just memcpy it */
        memcpy((char *)r_GetBufferChunkMappedMemory(handle) + offset, data, size);
    }
    else
    {
        /* welp, no memory mapped, so we'll need to do some more work */
        chunk = r_GetChunkPointer(handle);
        
        command_buffer = r_AllocateCommandBuffer();
        command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
        submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.command_buffer_count = 1;
        submit_info.command_buffers = &command_buffer;

        SDL_AtomicLock(&r_device.transfer_queue->spinlock);
        r_vkBeginCommandBuffer(command_buffer);
        
        if(size < 16384)
        {
            /* the data size is small enough, so it won't consume too much command buffer memory */
            vkCmdUpdateBuffer(command_buffer_ptr->command_buffer, heap->buffer, chunk->start + offset, size, data);
        }
        else
        {
            /* well, the data is fairly big, so we'll need to do things manually */
            uint32_t total_buffer_copy_count = 1 + size / r_device.staging.staging_buffer_size;
            uint32_t buffer_copy_count = total_buffer_copy_count - 1;
            uint32_t current_copy_size = 0;
            VkBufferCopy *buffer_copies = alloca(sizeof(VkBufferCopy) * buffer_copy_count);
            uint32_t copy_size = r_device.staging.staging_buffer_size;
            uint32_t copy_index = 0;
            buffer_copy_count--;
            
            while(copy_index < total_buffer_copy_count)
            {
                for(; copy_index < buffer_copy_count; copy_index++)
                {            
                    /* this loop will run if more than one copy is necessary. It will run
                    for N-1 copies, and each copy will have the size of a staging buffer.
                    Once this loop exits all the copies that are the size of a staging
                    buffer will be finished, and the outer loop will adjust the copy size 
                    to the amount that's left to copy, then increment buffer_copy_count to 
                    make this loop run one more time with the new copy size */
                    struct r_staging_buffer_t *staging_buffer = r_AllocateStagingBuffer(command_buffer);
                    
                    if(!staging_buffer)
                    {
                        /* ran out of staging buffers, so submit what we got so far to reclaim them back */
                        vkCmdCopyBuffer(command_buffer_ptr->command_buffer, r_device.staging.base_staging_buffer, heap->buffer, 
                                buffer_copy_count - copy_index, buffer_copies);
                        r_vkEndCommandBuffer(command_buffer);
                        vkResetFences(r_device.device, 1, &r_device.transfer_fence);
                        r_vkQueueSubmit(r_device.transfer_queue->queue, 1, &submit_info, r_device.transfer_fence);
                        vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);
                        r_vkBeginCommandBuffer(command_buffer);
                        staging_buffer = r_AllocateStagingBuffer(command_buffer);
                    }
                    
                    VkBufferCopy *buffer_copy = buffer_copies + copy_index;
                    buffer_copy->size = copy_size;
                    buffer_copy->dstOffset = chunk->start + current_copy_size;
                    buffer_copy->srcOffset = staging_buffer->offset;
                    memcpy(staging_buffer->memory, data + current_copy_size, copy_size);
                    current_copy_size += copy_size;
                }
                
                copy_size = size - current_copy_size;
                buffer_copy_count++;
            }
        }
        
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

void r_FillBuffer(struct r_buffer_h handle, void *data, uint32_t size, uint32_t offset)
{
    struct r_buffer_t *buffer;
    buffer = r_GetBufferPointer(handle);
    if(buffer)
    {
        r_FillBufferChunk(buffer->memory, data, size, offset);
    }
}

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
        if(image->image != VK_NULL_HANDLE)
        {
            vkDestroyImage(r_device.device, image->image, NULL);
        }
        
        if(image->memory.index != R_INVALID_CHUNK_INDEX)
        {
            r_FreeChunk(image->memory);
        }
        image->current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
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
    vkDeviceWaitIdle(r_device.device);
}

void *r_MapImageMemory(struct r_image_handle_t handle)
{
    struct r_image_t *image;
    struct r_image_heap_t *heap;
    struct r_chunk_t *chunk;
    void *memory = NULL;
    image = r_GetImagePointer(handle);
    
    if(image)
    {
        heap = r_GetHeapPointer(image->memory.heap);
        chunk = r_GetChunkPointer(image->memory);
        vkMapMemory(r_device.device, heap->memory, chunk->start, chunk->size, 0, &memory);
    }
    
    return memory;
}

void r_CreateDefaultTexture()
{
    struct r_texture_h default_texture;
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
    default_texture_pixels = mem_Calloc(pixel_pitch, description.extent.width * description.extent.height);
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
    mem_Free(default_texture_pixels);
}

struct r_texture_h r_CreateTexture(struct r_texture_description_t *description)
{
    struct r_texture_h handle = R_INVALID_TEXTURE_HANDLE;
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
        
        r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_GENERAL);

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
        
    }

    return handle;
}

void r_DestroyTexture(struct r_texture_h handle)
{
    struct r_texture_t *texture = r_GetTexturePointer(handle);
    struct r_image_t *image;
    if(texture && handle.index != R_DEFAULT_TEXTURE_INDEX /*&& handle.index != R_MISSING_TEXTURE_INDEX*/ )
    {
        vkDestroyImageView(r_device.device, texture->image_view, NULL);
        if(texture->event != VK_NULL_HANDLE)
        {
            vkDestroyEvent(r_device.device, texture->event, NULL);
        }
        texture->event = VK_NULL_HANDLE;
        r_DestroyImage(texture->image);
        remove_stack_list_element(&r_device.textures, handle.index);
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

struct r_texture_h r_LoadTexture(char *file_name, char *texture_name)
{
    unsigned char *pixels;
    int width;
    int height;
    int channels;
    struct r_texture_h handle = R_INVALID_TEXTURE_HANDLE;
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

struct r_texture_t *r_GetTexturePointer(struct r_texture_h handle)
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

struct r_texture_h r_GetDefaultTextureHandle()
{
    return R_TEXTURE_HANDLE(R_DEFAULT_TEXTURE_INDEX);
}

struct r_texture_h r_GetTextureHandle(char *name)
{
    struct r_texture_t *texture;

    for(uint32_t i = 0; i < r_device.textures.cursor; i++)
    {
        texture = r_GetTexturePointer(R_TEXTURE_HANDLE(i));

        if(texture && !strcmp(texture->name, name))
        {
            return R_TEXTURE_HANDLE(i);
        }
    }

    return R_INVALID_TEXTURE_HANDLE;
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
            shader->push_constants = mem_Calloc(description->push_constant_count, sizeof(struct r_push_constant_t));
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
            shader->vertex_bindings = (struct r_vertex_binding_t *)mem_Calloc(1, alloc_size);
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
            shader->resources = mem_Calloc(description->resource_count, sizeof(struct r_resource_binding_t));
            memcpy(shader->resources, description->resources, sizeof(struct r_resource_binding_t) * description->resource_count);

            layout_bindings = mem_Calloc(sizeof(VkDescriptorSetLayoutBinding), description->resource_count);
            for(uint32_t i = 0; i < description->resource_count; i++)
            {
                layout_binding = layout_bindings + i;
                resource_binding = description->resources + i;

                layout_binding->binding = i;
                layout_binding->descriptorType = resource_binding->descriptor_type;
                layout_binding->descriptorCount = resource_binding->count;
                layout_binding->pImmutableSamplers = NULL;
                layout_binding->stageFlags = description->stage;
//                layout_binding->
            }

            VkDescriptorSetLayoutCreateInfo layout_create_info = {};
            layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_create_info.pNext = NULL;
            layout_create_info.flags = 0;
            layout_create_info.bindingCount = description->resource_count;
            layout_create_info.pBindings = layout_bindings;
            vkCreateDescriptorSetLayout(r_device.device, &layout_create_info, NULL, &shader->descriptor_set_layout);
            mem_Free(layout_bindings);
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

VkPipelineColorBlendAttachmentState default_color_blend_attachment = {
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = 1.0,
    .dstAlphaBlendFactor = 1.0,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
};

VkPipelineColorBlendStateCreateInfo default_color_blend_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_NO_OP,
    .blendConstants = {1.0, 1.0, 1.0, 1.0}
};

struct r_render_pass_handle_t r_CreateRenderPass(struct r_render_pass_description_t *description)
{
    struct r_render_pass_handle_t handle = R_INVALID_RENDER_PASS_HANDLE;
    struct r_render_pass_t *render_pass;
    struct r_subpass_description_t *subpass_descriptions;
    VkRenderPassCreateInfo render_pass_create_info = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

    uint32_t color_attachment_count = 0;
    uint32_t pipeline_count = 0;

    VkPipelineColorBlendStateCreateInfo color_blend_state = default_color_blend_state;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .depthCompareOp = VK_COMPARE_OP_LESS,
    };

    handle.index = add_stack_list_element(&r_device.render_passes, NULL);
    render_pass = (struct r_render_pass_t *)get_stack_list_element(&r_device.render_passes, handle.index);
    
    subpass_descriptions = mem_Calloc(sizeof(struct r_subpass_description_t), description->subpass_count);
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

    VkAttachmentReference *color_attachment_references = mem_Calloc(sizeof(VkAttachmentReference), color_attachment_count * description->subpass_count);
    VkAttachmentReference *color_attachment_reference = color_attachment_references;
    VkAttachmentReference *depth_stencil_attachment_references = mem_Calloc(sizeof(VkAttachmentReference), description->subpass_count);
    VkAttachmentReference *depth_stencil_attachment_reference = depth_stencil_attachment_references;
    
    color_blend_state.pAttachments = mem_Calloc(sizeof(VkPipelineColorBlendAttachmentState), color_attachment_count);
    color_blend_state.attachmentCount = color_attachment_count;

    /* since missing attachment state will be all the same, just pre fill everything with the same
    values upfront */
    for(uint32_t attachment_index = 0; attachment_index < color_attachment_count; attachment_index++)
    {
        /* drop the pesky const */
        VkPipelineColorBlendAttachmentState *color_blend_attachment_state = (VkPipelineColorBlendAttachmentState *)color_blend_state.pAttachments +
            attachment_index;
        memcpy(color_blend_attachment_state, &default_color_blend_attachment, sizeof(VkPipelineColorBlendAttachmentState));
    }

    for(uint32_t subpass_index = 0; subpass_index < description->subpass_count; subpass_index++)
    {
        struct r_subpass_description_t *subpass_description = subpass_descriptions + subpass_index;
        subpass_description->pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
        pipeline_count += subpass_description->pipeline_description_count;

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
                        color_attachment_reference->layout = VK_IMAGE_LAYOUT_GENERAL;
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

    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = NULL;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = description->attachment_count;
    render_pass_create_info.pAttachments = mem_Calloc(sizeof(VkAttachmentDescription), description->attachment_count);

    if(description->attachment_count)
    {
        for(uint32_t attachment_index = 0; attachment_index < description->attachment_count; attachment_index++)
        {
            struct r_attachment_d *attachment_description = description->attachments + attachment_index;
            VkAttachmentDescription *render_pass_attachment_description = (VkAttachmentDescription *)render_pass_create_info.pAttachments + attachment_index;
            
            render_pass_attachment_description->format = attachment_description->format;
            render_pass_attachment_description->samples = attachment_description->samples;
            render_pass_attachment_description->initialLayout = attachment_description->initial_layout;
            render_pass_attachment_description->finalLayout = attachment_description->final_layout;
            render_pass_attachment_description->loadOp = attachment_description->load_op;
            render_pass_attachment_description->storeOp = attachment_description->store_op;
            render_pass_attachment_description->stencilLoadOp = attachment_description->stencil_load_op;
            render_pass_attachment_description->stencilStoreOp = attachment_description->stencil_store_op;
            
            if(!render_pass_attachment_description->samples)
            {
                /* probably zero initialized. No samples means one sample */
                render_pass_attachment_description->samples = VK_SAMPLE_COUNT_1_BIT;
            }

            if(render_pass_attachment_description->finalLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                /* fill in the final layout with a sane value if we
                didn't get one */
                if(r_IsDepthStencilFormat(render_pass_attachment_description->format))
                {
                    depth_stencil_state.depthTestEnable = VK_TRUE;
                    depth_stencil_state.depthWriteEnable = VK_TRUE;
                    render_pass_attachment_description->finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                else
                {
                    render_pass_attachment_description->finalLayout = VK_IMAGE_LAYOUT_GENERAL;
                }
            }
        }
    }

    render_pass_create_info.subpassCount = description->subpass_count;
    render_pass_create_info.pSubpasses = mem_Calloc(sizeof(VkSubpassDescription), description->subpass_count);

    for(uint32_t subpass_index = 0; subpass_index < description->subpass_count; subpass_index++)
    {
        struct r_subpass_description_t *subpass_description = subpass_descriptions + subpass_index;
        VkSubpassDescription *subpass = (VkSubpassDescription *)render_pass_create_info.pSubpasses + subpass_index;
        memcpy(subpass, subpass_description, sizeof(VkSubpassDescription));
    }

    vkCreateRenderPass(r_device.device, &render_pass_create_info, NULL, &render_pass->render_pass);
    render_pass->subpass_count = description->subpass_count;
    render_pass->subpasses = mem_Calloc(sizeof(struct r_subpass_t), render_pass->subpass_count);
    render_pass->pipelines = mem_Calloc(sizeof(struct r_pipeline_h), pipeline_count);
    pipeline_count = 0;
    for(uint32_t subpass_index = 0; subpass_index < description->subpass_count; subpass_index++)
    {
        struct r_subpass_t *subpass = render_pass->subpasses + subpass_index;
        subpass->pipeline_count = 0;
        subpass->pipeline_count = description->subpasses[subpass_index].pipeline_description_count;
        subpass->pipelines = render_pass->pipelines + pipeline_count;
        pipeline_count += subpass->pipeline_count;
        for(uint32_t pipeline_index = 0; pipeline_index < subpass->pipeline_count; pipeline_index++)
        {
            struct r_pipeline_description_t pipeline_description = {};
            memcpy(&pipeline_description, description->subpasses[subpass_index].pipeline_descriptions + pipeline_index, sizeof(struct r_pipeline_description_t));
            pipeline_description.subpass_index = subpass_index;
            pipeline_description.render_pass = render_pass->render_pass;
            pipeline_description.color_blend_state = &color_blend_state;
            if(!pipeline_description.depth_stencil_state)
            {
                pipeline_description.depth_stencil_state = &depth_stencil_state;
            }
            subpass->pipelines[pipeline_index] = r_CreatePipeline(&pipeline_description);
        }
    }
    render_pass->pipeline_count = pipeline_count;
    render_pass->attachments = mem_Calloc(description->attachment_count, sizeof(struct r_attachment_d));
    render_pass->attachment_count = description->attachment_count;
    memcpy(render_pass->attachments, description->attachments, sizeof(struct r_attachment_d) * description->attachment_count);

    return handle;
}

void r_DestroyRenderPass(struct r_render_pass_handle_t handle)
{

}

struct r_render_pass_t *r_GetRenderPassPointer(struct r_render_pass_handle_t handle)
{
    struct r_render_pass_t *render_pass;
    render_pass = get_stack_list_element(&r_device.render_passes, handle.index);
    if(render_pass && render_pass->render_pass == VK_NULL_HANDLE)
    {
        render_pass = NULL;
    }

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

struct r_pipeline_t *r_GetPipelinePointerByState(struct r_render_pass_handle_t render_pass_handle, uint32_t subpass_index, struct r_pipeline_state_t *pipeline_state)
{
    struct r_pipeline_t *pipeline;
    struct r_pipeline_h pipeline_handle;
    struct r_render_pass_t *render_pass;
    struct r_subpass_t *subpass;
    uint32_t offset = 0;
    
    VkPipelineColorBlendStateCreateInfo blend_state;
    VkPipelineColorBlendAttachmentState *attachments;
    uint32_t color_attachment_count = 0;
    
    render_pass = r_GetRenderPassPointer(render_pass_handle);
    subpass = render_pass->subpasses + subpass_index;
    
    for(uint32_t pipeline_index = 0; pipeline_index < subpass->pipeline_count; pipeline_index++)
    {
        pipeline = r_GetPipelinePointer(subpass->pipelines[pipeline_index]);
        if(!memcmp(&pipeline->state, pipeline_state, sizeof(struct r_pipeline_state_t)))
        {
            return pipeline;
        }
    }
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = (VkPipelineInputAssemblyStateCreateInfo){
        .topology = pipeline_state->input_assembly_state.topology
    };
    
    VkPipelineRasterizationStateCreateInfo rasterization_state = (VkPipelineRasterizationStateCreateInfo){
        .cullMode = pipeline_state->rasterizer_state.cull_mode,
        .frontFace = pipeline_state->rasterizer_state.front_face,
        .polygonMode = pipeline_state->rasterizer_state.polygon_mode,
    };
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = (VkPipelineDepthStencilStateCreateInfo){
        .depthTestEnable = pipeline_state->depth_state.test_enable,
        .depthWriteEnable = pipeline_state->depth_state.write_enable,
        .depthCompareOp = pipeline_state->depth_state.compare_op,
        
        .stencilTestEnable = pipeline_state->stencil_state.test_enable,
        .front = (VkStencilOpState){
            .failOp = pipeline_state->stencil_state.front.fail_op,
            .passOp = pipeline_state->stencil_state.front.pass_op,
            .depthFailOp = pipeline_state->stencil_state.front.depth_fail_op,
            .compareOp = pipeline_state->stencil_state.front.compare_op            
        },
        .back = (VkStencilOpState){
            .failOp = pipeline_state->stencil_state.back.fail_op,
            .passOp = pipeline_state->stencil_state.back.pass_op,
            .depthFailOp = pipeline_state->stencil_state.back.depth_fail_op,
            .compareOp = pipeline_state->stencil_state.back.compare_op            
        }
    };
    
    attachments = alloca(sizeof(VkPipelineColorBlendAttachmentState) * render_pass->attachment_count);
    for(uint32_t attachment_index = 0; attachment_index < render_pass->attachment_count; attachment_index++)
    {
        VkPipelineColorBlendAttachmentState *attachment = attachments + attachment_index;
        memcpy(attachment, &default_color_blend_attachment, sizeof(VkPipelineColorBlendAttachmentState));
        
        if(!r_IsDepthStencilFormat(render_pass->attachments[attachment_index].format))
        {
            color_attachment_count++;
        }
    }
    
    VkPipelineColorBlendStateCreateInfo color_blend_state = default_color_blend_state;
    color_blend_state.attachmentCount = color_attachment_count;
    color_blend_state.pAttachments = attachments;
    
    pipeline = r_GetPipelinePointer(subpass->pipelines[0]);
    
    struct r_pipeline_description_t description = {
        .shaders = (struct r_shader_t *[]){
            pipeline->vertex_shader,
            pipeline->fragment_shader
        },
        .shader_count = 2,
        .color_blend_state = &color_blend_state,
        .rasterization_state = &rasterization_state,
        .depth_stencil_state = &depth_stencil_state,
        .input_assembly_state = &input_assembly_state,
        .render_pass = render_pass->render_pass,
        .subpass_index = subpass_index,
    };
    
    subpass->pipeline_count++;
    render_pass->pipeline_count++;
    pipeline_handle = r_CreatePipeline(&description);
    render_pass->pipelines = mem_Realloc(render_pass->pipelines, sizeof(struct r_pipeline_h) * (render_pass->pipeline_count + 1));
    offset = render_pass->pipeline_count;
    
    uint32_t index = render_pass->subpass_count - 1;
    uint32_t stop_index = subpass_index;
    
    for(uint32_t half = 0; half < 2; half++)
    {
        for(; index >= stop_index + 1; index--)
        {
            /* move the handles of all the subpasses before and after subpass_index */
            subpass = render_pass->subpasses + index;
            offset -= subpass->pipeline_count;
            subpass->pipelines = render_pass->pipelines + offset;
            for(uint32_t pipeline_index = subpass->pipeline_count - 1; pipeline_index > 0; pipeline_index--)
            {
                subpass->pipelines[pipeline_index] = subpass->pipelines[pipeline_index - 1];
            }
        }
        
        subpass = render_pass->subpasses + subpass_index;
        offset -= subpass->pipeline_count;
        subpass->pipelines = render_pass->pipelines + offset;
        subpass->pipelines[subpass->pipeline_count - 1] = pipeline_handle;
        index--;
        
        if(!stop_index)
        {
            /* subpass_index is the first on the list, so there's no
            second half to update */
            break;
        }
        
        stop_index = 0;
    }
    
    return r_GetPipelinePointer(pipeline_handle);
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_pipeline_h r_CreatePipeline(struct r_pipeline_description_t *description)
{
    struct r_pipeline_h handle = R_INVALID_PIPELINE_HANDLE;
    struct r_pipeline_description_t description_copy = {};
    struct r_pipeline_t *pipeline = NULL;

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    VkPipelineLayoutCreateInfo layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };

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
        .cullMode = VK_CULL_MODE_BACK_BIT,
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
//        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
//        .logicOpEnable = VK_FALSE,
//        .logicOp = VK_LOGIC_OP_NO_OP,
//        .blendConstants = {1.0, 1.0, 1.0, 1.0}
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
        .maxSets = 1024,
    };

    VkPipelineVertexInputStateCreateInfo input_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    if(description)
    {
        handle.index = add_stack_list_element(&r_device.pipelines, NULL);
        pipeline = get_stack_list_element(&r_device.pipelines, handle.index);
        memcpy(&description_copy, description, sizeof(struct r_pipeline_description_t));
        description = &description_copy;

        for(uint32_t shader_index = 0; shader_index < description->shader_count; shader_index++)
        {
            struct r_shader_t *shader = description->shaders[shader_index];
            layout_create_info.setLayoutCount += shader->descriptor_set_layout != VK_NULL_HANDLE;
            if(shader->resource_count > descriptor_pool_create_info.poolSizeCount)
            {
                descriptor_pool_create_info.poolSizeCount = shader->resource_count;
            }
            layout_create_info.pushConstantRangeCount += shader->push_constant_count;
        }
        
        if(layout_create_info.pushConstantRangeCount)
        {
            layout_create_info.pPushConstantRanges = mem_Calloc(sizeof(VkPushConstantRange), layout_create_info.pushConstantRangeCount);
            layout_create_info.pushConstantRangeCount = 0;
        }
        
        layout_create_info.pSetLayouts = mem_Calloc(sizeof(VkDescriptorSetLayout), layout_create_info.setLayoutCount);
        layout_create_info.setLayoutCount = 0;

        if(descriptor_pool_create_info.poolSizeCount)
        {
            descriptor_pool_create_info.pPoolSizes = mem_Calloc(sizeof(VkDescriptorPoolSize), descriptor_pool_create_info.poolSizeCount);
            descriptor_pool_create_info.poolSizeCount = 0;
        }
        
        pipeline_create_info.pStages = mem_Calloc(sizeof(VkPipelineShaderStageCreateInfo), description->shader_count);

        for(uint32_t shader_index = 0; shader_index < description->shader_count; shader_index++)
        {
            struct r_shader_t *shader = description->shaders[shader_index];
            if(shader->descriptor_set_layout != VK_NULL_HANDLE)
            {
                VkDescriptorSetLayout *layout = (VkDescriptorSetLayout *)layout_create_info.pSetLayouts + layout_create_info.setLayoutCount;
                *layout = shader->descriptor_set_layout;
                layout_create_info.setLayoutCount++;
            }
            if(shader->push_constant_count)
            {
                for(uint32_t range_index = 0; range_index < shader->push_constant_count; range_index++)
                {
                    VkPushConstantRange *range = (VkPushConstantRange *)layout_create_info.pPushConstantRanges + layout_create_info.pushConstantRangeCount;
                    layout_create_info.pushConstantRangeCount++;
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

        vkCreatePipelineLayout(r_device.device, &layout_create_info, NULL, &pipeline->layout);
        description->vertex_input_state = &input_state_create_info;

        input_state_create_info.vertexBindingDescriptionCount = pipeline->vertex_shader->vertex_binding_count;
        input_state_create_info.pVertexBindingDescriptions = mem_Calloc(sizeof(VkVertexInputBindingDescription), pipeline->vertex_shader->vertex_binding_count);
        input_state_create_info.pVertexAttributeDescriptions = mem_Calloc(sizeof(VkVertexInputAttributeDescription), pipeline->vertex_shader->vertex_attrib_count);
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

        if(description->input_assembly_state)
        {
            memcpy(&input_assembly_state, description->input_assembly_state, sizeof(VkPipelineInputAssemblyStateCreateInfo));
            input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        }
        description->input_assembly_state = &input_assembly_state;

        if(!description->viewport_state)
        {
            /* by default assume outside code will provide viewport and
            scissor rects info */
            description->viewport_state = &viewport_state;
        }

        if(description->rasterization_state)
        {
            memcpy(&rasterization_state, description->rasterization_state, sizeof(VkPipelineRasterizationStateCreateInfo));
            rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

            if(rasterization_state.lineWidth == 0.0)
            {
                rasterization_state.lineWidth = 1.0;
            }

        }
        description->rasterization_state = &rasterization_state;


        if(!description->multisample_state)
        {
            description->multisample_state = &multisample_state;
        }

        if(description->depth_stencil_state)
        {
            memcpy(&depth_stencil_state, description->depth_stencil_state, sizeof(VkPipelineDepthStencilStateCreateInfo));
            depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        }
        description->depth_stencil_state = &depth_stencil_state;

        memcpy(&color_blend_state, description->color_blend_state, sizeof(VkPipelineColorBlendStateCreateInfo));
        description->color_blend_state = &color_blend_state;
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        if(description->dynamic_state)
        {
            memcpy(&dynamic_state, description->dynamic_state, sizeof(VkPipelineDynamicStateCreateInfo));
            dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        }
        description->dynamic_state = &dynamic_state;

        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pVertexInputState = description->vertex_input_state;
        pipeline_create_info.pInputAssemblyState = description->input_assembly_state;
        pipeline_create_info.pTessellationState = description->tesselation_state;
        pipeline_create_info.pViewportState = description->viewport_state;
        pipeline_create_info.pRasterizationState = description->rasterization_state;
        pipeline_create_info.pMultisampleState = description->multisample_state;
        pipeline_create_info.pDepthStencilState = description->depth_stencil_state;
        pipeline_create_info.pColorBlendState = description->color_blend_state;
        pipeline_create_info.pDynamicState = description->dynamic_state;
        pipeline_create_info.layout = pipeline->layout;
        pipeline_create_info.renderPass = description->render_pass;
        pipeline_create_info.subpass = description->subpass_index;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;

        vkCreateGraphicsPipelines(r_device.device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline->pipeline);

        pipeline->state.depth_state.compare_op = description->depth_stencil_state->depthCompareOp;
        pipeline->state.depth_state.test_enable = description->depth_stencil_state->depthTestEnable;
        pipeline->state.depth_state.write_enable = description->depth_stencil_state->depthWriteEnable;
        
        pipeline->state.stencil_state.test_enable = description->depth_stencil_state->stencilTestEnable;
        pipeline->state.stencil_state.front.compare_op = description->depth_stencil_state->front.compareOp;
        pipeline->state.stencil_state.front.pass_op = description->depth_stencil_state->front.passOp;
        pipeline->state.stencil_state.front.fail_op = description->depth_stencil_state->front.failOp;
        pipeline->state.stencil_state.front.depth_fail_op = description->depth_stencil_state->front.depthFailOp;
        pipeline->state.stencil_state.back.compare_op = description->depth_stencil_state->back.compareOp;
        pipeline->state.stencil_state.back.pass_op = description->depth_stencil_state->back.passOp;
        pipeline->state.stencil_state.back.fail_op = description->depth_stencil_state->back.failOp;
        pipeline->state.stencil_state.back.depth_fail_op = description->depth_stencil_state->back.depthFailOp;
        
        pipeline->state.input_assembly_state.topology = description->input_assembly_state->topology;
        
        pipeline->state.rasterizer_state.polygon_mode = description->rasterization_state->polygonMode;
        pipeline->state.rasterizer_state.front_face = description->rasterization_state->frontFace;
        pipeline->state.rasterizer_state.cull_mode = description->rasterization_state->cullMode;
        
        if(layout_create_info.pPushConstantRanges)
        {
            mem_Free((VkPushConstantRange *)layout_create_info.pPushConstantRanges);
        }
        if(layout_create_info.pSetLayouts)
        {
            mem_Free((VkDescriptorSetLayout *)layout_create_info.pSetLayouts);
        }
        if(descriptor_pool_create_info.pPoolSizes)
        {
            mem_Free((VkDescriptorPoolSize *)descriptor_pool_create_info.pPoolSizes);
        }
        mem_Free((VkPipelineShaderStageCreateInfo *)pipeline_create_info.pStages);
        mem_Free((VkVertexInputAttributeDescription *)input_state_create_info.pVertexAttributeDescriptions);
        mem_Free((VkVertexInputBindingDescription *)input_state_create_info.pVertexBindingDescriptions);
    }
    return handle;
}

struct r_pipeline_t *r_GetPipelinePointer(struct r_pipeline_h pipeline_handle)
{
    struct r_pipeline_t *pipeline = NULL;
    pipeline = get_stack_list_element(&r_device.pipelines, pipeline_handle.index);
    if(pipeline && pipeline->pipeline == VK_NULL_HANDLE)
    {
        pipeline = NULL;
    }
    return pipeline;
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_framebuffer_h r_CreateFramebuffer(struct r_framebuffer_description_t *description)
{
    struct r_framebuffer_h handle = R_INVALID_FRAMEBUFFER_HANDLE;
    struct r_framebuffer_description_t *description_copy;
    struct r_framebuffer_t *framebuffer;
    struct r_texture_description_t texture_description = {};
    struct r_texture_h *textures;
    struct r_texture_t *texture;
    VkFramebufferCreateInfo framebuffer_create_info = {};
    VkAttachmentDescription *attachment_description;

    if(description)
    {
        handle.index = add_stack_list_element(&r_device.framebuffers, NULL);
        add_stack_list_element(&r_device.framebuffer_descriptions, NULL);
        framebuffer = get_stack_list_element(&r_device.framebuffers, handle.index);
        description_copy = get_stack_list_element(&r_device.framebuffer_descriptions, handle.index);
        memcpy(description_copy, description, sizeof(struct r_framebuffer_description_t));
        
        if(!description->attachments)
        {
            description_copy->attachment_count = description_copy->render_pass->attachment_count;
            description_copy->attachments = mem_Calloc(sizeof(struct r_attachment_d), description_copy->attachment_count);
            memcpy(description_copy->attachments, description_copy->render_pass->attachments, 
                sizeof(struct r_attachment_d) * description_copy->attachment_count);
        }
        else
        {
            description_copy->attachments = mem_Calloc(sizeof(struct r_attachment_d), description_copy->attachment_count);
            memcpy(description_copy->attachments, description->attachments, sizeof(struct r_attachment_d) * description->attachment_count);
        }
        
        framebuffer->buffers = mem_Calloc(description_copy->frame_count, sizeof(VkFramebuffer));
        framebuffer->texture_count = description_copy->attachment_count * description_copy->frame_count;
        framebuffer->textures = mem_Calloc(framebuffer->texture_count, sizeof(struct r_texture_h));   
        
        r_InitializeFramebuffer(framebuffer, description_copy);
    }

    return handle;
}

void r_InitializeFramebuffer(struct r_framebuffer_t *framebuffer, struct r_framebuffer_description_t *description)
{
    VkFramebufferCreateInfo framebuffer_create_info = {};
    struct r_texture_description_t texture_description = {};

    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = NULL;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = description->render_pass->render_pass;
    framebuffer_create_info.attachmentCount = description->attachment_count;
    framebuffer_create_info.pAttachments = mem_Calloc(sizeof(VkImageView), description->attachment_count);
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
        struct r_texture_h *textures = framebuffer->textures + frame_index * description->attachment_count;
        for(uint32_t attachment_index = 0; attachment_index < description->attachment_count; attachment_index++)
        {
            struct r_attachment_d *attachment_description = description->attachments + attachment_index;
            texture_description.format = attachment_description->format;
            texture_description.tiling = attachment_description->tiling;
            if(r_IsDepthStencilFormat(attachment_description->format))
            {
                texture_description.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            else
            {
                texture_description.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            
            texture_description.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            
            textures[attachment_index] = r_CreateTexture(&texture_description);
            struct r_texture_t *texture = r_GetTexturePointer(textures[attachment_index]);
            *(VkImageView *)(framebuffer_create_info.pAttachments + attachment_index) = texture->image_view;
            
            if(r_IsDepthStencilFormat(attachment_description->format))
            {
                r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            }
            else
            {
                r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_GENERAL);
            }
            
            texture->event = r_CreateEvent();
        }
        vkCreateFramebuffer(r_device.device, &framebuffer_create_info, NULL, framebuffer->buffers + frame_index);
    }

    mem_Free((void *)framebuffer_create_info.pAttachments);
}

void r_DestroyFramebuffer(struct r_framebuffer_h handle)
{
    
}

struct r_framebuffer_t *r_GetFramebufferPointer(struct r_framebuffer_h handle)
{
    struct r_framebuffer_t *framebuffer;
    framebuffer = get_stack_list_element(&r_device.framebuffers, handle.index);
    if(framebuffer && !framebuffer->texture_count)
    {
        framebuffer = NULL;
    }
    return framebuffer;
}

struct r_framebuffer_description_t *r_GetFramebufferDescriptionPointer(struct r_framebuffer_h handle)
{
    struct r_framebuffer_description_t *description = NULL;
    struct r_framebuffer_t *framebuffer;
    framebuffer = r_GetFramebufferPointer(handle);
    
    if(framebuffer)
    {
        description = get_stack_list_element(&r_device.framebuffer_descriptions, handle.index);
    }
    
    return description;
}

void r_ResizeFramebuffer(struct r_framebuffer_h handle, uint32_t width, uint32_t height)
{
    struct r_framebuffer_t *framebuffer;
    struct r_framebuffer_description_t *description;
    
    framebuffer = r_GetFramebufferPointer(handle);
    if(framebuffer)
    {
        description = r_GetFramebufferDescriptionPointer(handle);
        description->width = width;
        description->height = height;
        
        for(uint32_t frame_index = 0; frame_index < description->frame_count; frame_index++)
        {
            struct r_texture_h *textures = framebuffer->textures + description->attachment_count * frame_index;
            
            for(uint32_t texture_index = 0; texture_index < description->attachment_count; texture_index++)
            {
                r_DestroyTexture(textures[texture_index]);
            }
            
            vkDestroyFramebuffer(r_device.device, framebuffer->buffers[frame_index], NULL);
        }
        
        r_InitializeFramebuffer(framebuffer, description);
    }
}

void r_PresentFramebuffer(struct r_framebuffer_h handle)
{
    VkPresentInfoKHR present_info = {};
    struct r_submit_info_t submit_info = {.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    union r_command_buffer_h command_buffer;
    struct r_framebuffer_t *framebuffer;
    struct r_texture_t *texture;
//    struct r_swapchain_t *swapchain;
    struct r_image_handle_t swapchain_image;
    uint32_t current_image_index;
    struct r_image_t *texture_image_ptr;
    struct r_image_t *swapchain_image_ptr;

    framebuffer = r_GetFramebufferPointer(handle);
    texture = r_GetTexturePointer(framebuffer->textures[framebuffer->current_buffer]);

    current_image_index = r_NextSwapchainImage();
    swapchain_image = r_device.swapchain.images[current_image_index];

    texture_image_ptr = r_GetImagePointer(texture->image);
    swapchain_image_ptr = r_GetImagePointer(swapchain_image);

    command_buffer = r_AllocateCommandBuffer();

    r_vkBeginCommandBuffer(command_buffer);
    r_vkCmdSetImageLayout(command_buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    r_vkCmdSetImageLayout(command_buffer, swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    r_vkCmdBlitImage(command_buffer, texture->image, swapchain_image, NULL);
    r_vkCmdSetImageLayout(command_buffer, texture->image, VK_IMAGE_LAYOUT_GENERAL);
    r_vkCmdSetImageLayout(command_buffer, swapchain_image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    r_vkEndCommandBuffer(command_buffer);

    submit_info.command_buffer_count = 1;
    submit_info.command_buffers = &command_buffer;
    vkResetFences(r_device.device, 1, &r_device.transfer_fence);
    r_vkQueueSubmit(r_device.transfer_queue->queue, 1, &submit_info, r_device.transfer_fence);
    vkWaitForFences(r_device.device, 1, &r_device.transfer_fence, VK_TRUE, 0xffffffffffffffff);
    vkQueueWaitIdle(r_device.transfer_queue->queue);

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &r_device.swapchain.swapchain;
    present_info.pImageIndices = &current_image_index;
    vkQueuePresentKHR(r_device.present_queue->queue, &present_info);
    vkQueueWaitIdle(r_device.present_queue->queue);
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_SetSwapchainSurface(VkSurfaceKHR surface)
{
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    VkSurfaceFormatKHR surface_format;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    uint32_t present_mode_count;
    VkPresentModeKHR *present_modes;
    
    VkPresentModeKHR test_present_modes[] = {
        VK_PRESENT_MODE_MAILBOX_KHR,
        VK_PRESENT_MODE_FIFO_RELAXED_KHR,
        VK_PRESENT_MODE_IMMEDIATE_KHR,
    };
    
    VkImageCreateInfo image_create_info = {};
    VkImage swapchain_images[R_SWAPCHAIN_IMAGE_COUNT];
    
    /* The main purpose of this function is to allow changing the size of 
    the window. I'm not 100% sure this is the right way of handling it. When
    the device gets created, it's necessary to find out which queue supports
    presenting operations for the surface used during initialization. Here we
    destroy that surface and use the new one passed in. However, what if the
    present queue we're using doesn't support presenting for the new surface?
    Does that mean the correct way of handling window resizing is by destroying
    and recreating the whole VkDevice? */
    
    if(r_device.swapchain.swapchain == VK_NULL_HANDLE)
    {
        r_device.swapchain.images = mem_Calloc(2, sizeof(struct r_image_handle_t));
        r_device.swapchain.fence = r_CreateFence();
    }
    else
    {
        vkDestroySwapchainKHR(r_device.device, r_device.swapchain.swapchain, NULL);
        vkDestroySurfaceKHR(r_device.instance, r_device.swapchain.surface, NULL);
        for(uint32_t image_index = 0; image_index < R_SWAPCHAIN_IMAGE_COUNT; image_index++)
        {
            struct r_image_t *image = r_GetImagePointer(r_device.swapchain.images[image_index]);
            /* since we overwrote the image returned by r_CreateImage with the one created
            for the swapchain, we set it to null here to avoid r_DestroyImage trying to 
            destroy it, since it's the implementation's job to get rid of the swapchain image.
            We do need to get rid of OUR image resource, however. */
            image->image = VK_NULL_HANDLE;
            r_DestroyImage(r_device.swapchain.images[image_index]);
        }
    }
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(r_device.physical_device, surface, &(uint32_t){1}, &surface_format);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_device.physical_device, surface, &surface_capabilities);
    vkGetPhysicalDeviceSurfaceSupportKHR(r_device.physical_device, r_device.present_queue_family, surface, &(VkBool32){0});
    
//    printf("format: %d\ncolor space: %d\n", surface_format.format, surface_format.colorSpace);
//    printf("composite alpha flags: %d\n", surface_capabilities.supportedCompositeAlpha);
//    printf("%d x %d\n", surface_capabilities.currentExtent.width, surface_capabilities.currentExtent.height);
    
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = 2;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = surface_capabilities.currentExtent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    
    vkGetPhysicalDeviceSurfacePresentModesKHR(r_device.physical_device, surface, &present_mode_count, NULL);
    present_modes = alloca(sizeof(VkPresentModeKHR) * present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(r_device.physical_device, surface, &present_mode_count, present_modes);
    
    for(uint32_t mode_test_index = 0; mode_test_index < sizeof(test_present_modes) / sizeof(test_present_modes[0]); mode_test_index++)
    {
        for(uint32_t present_mode_index = 0; present_mode_index < present_mode_count; present_mode_index++)
        {
            if(present_modes[present_mode_index] == test_present_modes[mode_test_index])
            {
                swapchain_create_info.presentMode = present_modes[present_mode_index];
                mode_test_index = 0xfffffffe;
                break;
            }
        }
    }
    
    vkCreateSwapchainKHR(r_device.device, &swapchain_create_info, NULL, &r_device.swapchain.swapchain);
    r_device.swapchain.surface = surface; 
    r_device.swapchain.surface_width = surface_capabilities.currentExtent.width;
    r_device.swapchain.surface_height = surface_capabilities.currentExtent.height; 
    
    vkGetSwapchainImagesKHR(r_device.device, r_device.swapchain.swapchain, &(uint32_t){R_SWAPCHAIN_IMAGE_COUNT}, NULL);
    vkGetSwapchainImagesKHR(r_device.device, r_device.swapchain.swapchain, &(uint32_t){R_SWAPCHAIN_IMAGE_COUNT}, swapchain_images);

    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = surface_format.format;
    image_create_info.extent.width = surface_capabilities.currentExtent.width;
    image_create_info.extent.height = surface_capabilities.currentExtent.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    for(uint32_t image_index = 0; image_index < R_SWAPCHAIN_IMAGE_COUNT; image_index++)
    {
        /* this is a "hack", not a terrible one, but not pretty either. This is to allow
        us to use all the image related functions in the swapchain images without having
        to have a separate version of it that takes a VkImage */
        r_device.swapchain.images[image_index] = r_CreateImage(&image_create_info);
        struct r_image_t *image = r_GetImagePointer(r_device.swapchain.images[image_index]);
        vkDestroyImage(r_device.device, image->image, NULL);
        r_FreeChunk(image->memory);
        image->memory = R_INVALID_CHUNK_HANDLE;
        image->image = swapchain_images[image_index]; 
    }
}

uint32_t r_NextSwapchainImage()
{
    uint32_t image_index;
    vkResetFences(r_device.device, 1, &r_device.swapchain.fence);
    vkAcquireNextImageKHR(r_device.device, r_device.swapchain.swapchain, 0xffffffffffffffff, VK_NULL_HANDLE, r_device.swapchain.fence, &image_index);
    vkWaitForFences(r_device.device, 1, &r_device.swapchain.fence, VK_TRUE, 0xffffffffffffffff);
    r_device.swapchain.current_image = image_index;
    return image_index;
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

void r_vkCmdBindIndexBuffer(union r_command_buffer_h command_buffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType index_type)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdBindIndexBuffer(command_buffer_ptr->command_buffer, buffer, offset, index_type);
}

void r_vkCmdBeginRenderPass(union r_command_buffer_h command_buffer, struct r_render_pass_begin_info_t *begin_info, VkSubpassContents subpass_contents)
{
    struct r_command_buffer_t *command_buffer_ptr;
    struct r_framebuffer_t *framebuffer;
    struct r_framebuffer_description_t *description;
    struct r_render_pass_t *render_pass;
    VkRenderPassBeginInfo render_pass_begin_info = {};
    VkImageMemoryBarrier memory_barrier = {};
    
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    framebuffer = r_GetFramebufferPointer(begin_info->framebuffer);
    description = r_GetFramebufferDescriptionPointer(begin_info->framebuffer);
    render_pass = r_GetRenderPassPointer(begin_info->render_pass);
    
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass->render_pass;
    render_pass_begin_info.clearValueCount = begin_info->clear_value_count;
    render_pass_begin_info.pClearValues = begin_info->clear_values;
    render_pass_begin_info.renderArea = begin_info->render_area;
    render_pass_begin_info.framebuffer = framebuffer->buffers[0];
    
    command_buffer_ptr->render_pass = begin_info->render_pass;
    command_buffer_ptr->framebuffer = begin_info->framebuffer;
    
//    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//    memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
//    memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
//    memory_barrier.subresourceRange.levelCount = 1;
//    memory_barrier.subresourceRange.layerCount = 1;
//    memory_barrier.subresourceRange.baseMipLevel = 0;
//    memory_barrier.subresourceRange.baseArrayLayer = 0;
    
//    for(uint32_t attachment_index = 0; attachment_index < description->attachment_count; attachment_index++)
//    {
//        struct r_texture_t *texture;
//        struct r_image_t *image;
//        
//        texture = r_GetTexturePointer(framebuffer->textures[attachment_index]);
//        image = r_GetImagePointer(texture->image);
//        
//        if(r_IsDepthStencilFormat(description->attachments[attachment_index].format))
//        {
//            
//        }
//        else
//        {
//            if(image->current_layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
//            {
////                memory_barrier.image = image->image;
////                memory_barrier.oldLayout = image->current_layout;
////                memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
////                memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
////                vkCmdPipelineBarrier(command_buffer_ptr->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
////                                                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
////                                                                         0, 0, NULL, 0, NULL, 1, &memory_barrier);
////                
////                image->current_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//                r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//            }
//        }
//    }
    
    vkCmdBeginRenderPass(command_buffer_ptr->command_buffer, &render_pass_begin_info, subpass_contents);
}

void r_vkCmdEndRenderPass(union r_command_buffer_h command_buffer)
{
    struct r_command_buffer_t *command_buffer_ptr;
    struct r_framebuffer_t *framebuffer;
    struct r_texture_t *texture;
    
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    framebuffer = r_GetFramebufferPointer(command_buffer_ptr->framebuffer);
    
    vkCmdEndRenderPass(command_buffer_ptr->command_buffer);
    command_buffer_ptr->render_pass = R_INVALID_RENDER_PASS_HANDLE;
    command_buffer_ptr->framebuffer = R_INVALID_FRAMEBUFFER_HANDLE;
    
    for(uint32_t texture_index = 0; texture_index < framebuffer->texture_count; texture_index++)
    {
        texture = r_GetTexturePointer(framebuffer->textures[texture_index]);
        vkResetEvent(r_device.device, texture->event);
        vkCmdSetEvent(command_buffer_ptr->command_buffer, texture->event, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                                                                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    }
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

void r_vkCmdSetPointSize(union r_command_buffer_h command_buffer, float size)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
}

void r_vkCmdPushConstants(union r_command_buffer_h command_buffer, VkPipelineLayout layout, VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size, void *data)
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

void r_vkCmdDrawIndexed(union r_command_buffer_h command_buffer, uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdDrawIndexed(command_buffer_ptr->command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
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

void r_vkCmdPipelineImageBarrier(union r_command_buffer_h command_buffer, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask, VkDependencyFlags dependency_flags, uint32_t barrier_count, VkImageMemoryBarrier *barriers)
{
    struct r_command_buffer_t *command_buffer_ptr;
    
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdPipelineBarrier(command_buffer_ptr->command_buffer, src_stage_mask, dst_stage_mask, dependency_flags, 0, NULL, 0, NULL, barrier_count, barriers);
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
    memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    /* new layout in place, now the memory becomes accessible */
    memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    memory_barrier.oldLayout = image->current_layout;
    memory_barrier.newLayout = new_layout;
    memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.image = image->image;
    memory_barrier.subresourceRange.aspectMask = image->aspect_mask;
    memory_barrier.subresourceRange.baseArrayLayer = 0;
    memory_barrier.subresourceRange.baseMipLevel = 0;
    memory_barrier.subresourceRange.layerCount = 1;
    memory_barrier.subresourceRange.levelCount = 1;
    r_vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0, 0, NULL, 0, NULL, 1, &memory_barrier);
    image->current_layout = new_layout;
}

void r_vkCmdClearAttachments(union r_command_buffer_h command_buffer, uint32_t attachment_count, VkClearAttachment *attachments, uint32_t rect_count, VkClearRect *rects)
{
    struct r_command_buffer_t *command_buffer_ptr;
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    vkCmdClearAttachments(command_buffer_ptr->command_buffer, attachment_count, attachments, rect_count, rects);
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

void r_vkUpdateDescriptorSets2(uint32_t descriptor_write_count, VkWriteDescriptorSet *descriptor_writes, struct r_image_handle_t image)
{
    struct r_image_t *image_ptr;
    
    image_ptr = r_GetImagePointer(image);
    if(image_ptr->current_layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        r_SetImageLayout(image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
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

void r_UpdateCombinedImageSamplerDescriptorSet(VkDescriptorSet descriptor_set, uint32_t dst_binding, struct r_texture_h texture)
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
//    uint32_t command_buffer_index;

    for(uint32_t submit_index = 0; submit_index < submit_count; submit_index++)
    {
        submit = submit_info + submit_index;
        for(uint32_t index = 0; index < submit_info[submit_index].command_buffer_count; index++)
        {
            command_buffer_ptr = r_GetCommandBufferPointer(submit->command_buffers[index]);
            add_list_element(&r_device.draw_command_pool.pending_command_buffers, &submit->command_buffers[index].index);
            submit->command_buffers[index].command_buffer = command_buffer_ptr->command_buffer;
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

void r_vkMapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **data)
{
    vkMapMemory(r_device.device, memory, offset, size, flags, data);
}







