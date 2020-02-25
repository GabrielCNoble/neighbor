#include "r.h"
#include "dstuff/file/path.h"
//#include "r_vk.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/containers/list.h"
#include <stdlib.h>
#include <string.h>
#include "stb/stb_image.h"

struct r_renderer_t r_renderer;



VkPhysicalDevice r_physical_device;
VkInstance r_instance;
VkDevice r_device;
VkSurfaceKHR r_surface;

uint32_t r_graphics_queue_family_index = 0xffffffff;
//uint32_t r_transfer_queue_family_index = 0xffffffff;
uint32_t r_present_queue_family_index = 0xffffffff;
VkQueue r_graphics_queue;
VkQueue r_present_queue;
VkCommandPool r_command_pool;
VkCommandBuffer r_command_buffers[2];
uint32_t r_supports_push_descriptors = 0;
PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSet = NULL;
VkDeviceMemory r_texture_copy_memory;
VkFence r_transfer_fence;


/* this is Vulkan. Resources may be created concurrently */
SDL_SpinLock r_resource_spinlock;
struct stack_list_t r_textures;
/* necessary for creating image views from a particular texture */
struct stack_list_t r_texture_descriptions;
struct stack_list_t r_framebuffers;
struct stack_list_t r_render_passes;
struct stack_list_t r_samplers;
struct stack_list_t r_shaders;

void r_InitRenderer()
{
    struct r_alloc_t free_alloc;
    SDL_Thread *renderer_thread;
    FILE *file;
    struct r_shader_description_t vertex_description = {};
    struct r_shader_description_t fragment_description = {};

    r_renderer.allocd_blocks[0] = create_stack_list(sizeof(struct r_alloc_t), 512);
    r_renderer.allocd_blocks[1] = create_stack_list(sizeof(struct r_alloc_t), 512);
    r_renderer.free_blocks[0] = create_list(sizeof(struct r_alloc_t), 512);
    r_renderer.free_blocks[1] = create_list(sizeof(struct r_alloc_t), 512);
    r_renderer.cmd_buffer = create_ringbuffer(sizeof(struct r_cmd_t), 8192);
    r_renderer.cmd_buffer_data = create_ringbuffer(R_CMD_DATA_ELEM_SIZE, 8192 * 10);
    r_renderer.submiting_draw_cmd_buffer = NULL;
    r_renderer.materials = create_stack_list(sizeof(struct r_material_t), 256);
    r_renderer.lights = create_stack_list(sizeof(struct r_light_t), 256);
    free_alloc.start = 0;
    free_alloc.size = R_HEAP_SIZE;
    free_alloc.align = 0;
    add_list_element(&r_renderer.free_blocks[0], &free_alloc);
    add_list_element(&r_renderer.free_blocks[1], &free_alloc);
    mat4_t_identity(&r_renderer.view_matrix);
    mat4_t_identity(&r_renderer.projection_matrix);

    r_textures = create_stack_list(sizeof(struct r_texture_t), 512);
    r_texture_descriptions = create_stack_list(sizeof(struct r_texture_description_t), 512);
    r_samplers = create_stack_list(sizeof(struct r_sampler_t), 16);
    r_shaders = create_stack_list(sizeof(struct r_shader_t), 128);

    r_renderer.z_far = 1000.0;
    r_renderer.z_near = 0.01;
    r_renderer.fov_y = 0.68;
    r_renderer.width = 800;
    r_renderer.height = 600;
    r_RecomputeViewProjectionMatrix();

    r_renderer.window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, r_renderer.width, r_renderer.height, SDL_WINDOW_VULKAN);
    SDL_GL_SetSwapInterval(1);
    r_InitVulkan();
    r_CreateDefaultTexture();

    file = fopen("shader.vert.spv", "rb");
    read_file(file, &vertex_description.code, &vertex_description.code_size);
    fclose(file);
    vertex_description.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_description.vertex_binding_count = 1;
    vertex_description.vertex_bindings = (struct r_vertex_binding_t []){
        [0] = {
            .size = sizeof(struct vertex_t),
            .attribs = (struct r_vertex_attrib_t []){
                [0] = {
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = offsetof(struct vertex_t, position),
                },
                [1] = {
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = offsetof(struct vertex_t, normal),
                },
                [2] = {
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = offsetof(struct vertex_t, tex_coords),
                }
            },
            .attrib_count = 3
        }
    };

    r_CreateShader(&vertex_description);
    free(vertex_description.code);

    fopen("shader.frag.spv", "rb");
    read_file(file, &fragment_description.code, &fragment_description.code_size);
    fclose(file);
    fragment_description.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_description.resource_count = 1;
    fragment_description.resources = (struct r_resource_binding_t []){
        [0] = {
            .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .count = 1,
        }
    };
    r_CreateShader(&fragment_description);
    free(fragment_description.code);






//    r_LoadTexture("doggo.png", "doggo0");
//    r_LoadTexture("doggo.png", "doggo1");
//    r_LoadTexture("doggo.png", "doggo2");
//    r_LoadTexture("doggo.png", "doggo3");
//    r_LoadTexture("doggo.png", "doggo4");

    renderer_thread = SDL_CreateThread(r_ExecuteCmds, "renderer thread", NULL);
    SDL_DetachThread(renderer_thread);
//    r_InitBuiltinTextures();
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_InitVulkan()
{
    const char **extensions = NULL;
    uint32_t extension_count = 0;
    uint32_t physical_device_count = 1;
    uint32_t queue_family_property_count;
    uint32_t queue_create_info_count;
    uint32_t present_supported;

    VkExtensionProperties *extension_properties;
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
    extension_properties = alloca(sizeof(VkExtensionProperties) * extension_count);
    extensions = alloca(sizeof(char *) * extension_count);
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_properties);

    for(uint32_t i = 0; i < extension_count; i++)
    {
        extensions[i] = extension_properties[i].extensionName;
    }

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = NULL;
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = (const char * const []){"VK_LAYER_KHRONOS_validation"};
    instance_create_info.enabledExtensionCount = extension_count;
    instance_create_info.ppEnabledExtensionNames = extensions;
    vkCreateInstance(&instance_create_info, NULL, &r_instance);

    SDL_Vulkan_CreateSurface(r_renderer.window, r_instance, &r_surface);

    vkEnumeratePhysicalDevices(r_instance, &physical_device_count, &r_physical_device);
    VkQueueFamilyProperties *queue_family_properties;
    vkGetPhysicalDeviceQueueFamilyProperties(r_physical_device, &queue_family_property_count, NULL);
    queue_family_properties = alloca(sizeof(VkQueueFamilyProperties) * queue_family_property_count);
    vkGetPhysicalDeviceQueueFamilyProperties(r_physical_device, &queue_family_property_count, queue_family_properties);

    for(uint32_t i = 0; i < queue_family_property_count; i++)
    {
        if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            r_graphics_queue_family_index = i;
            break;
        }
    }

//    for(uint32_t i = 0; i < queue_family_property_count; i++)
//    {
//        /* the spec states that every command that is valid in a transfer
//        capable queue is also valid in a graphics capable queue, which means
//        that a graphics queue is able to perform transfer operations. However,
//        since we'll wait until the command buffer used to transfer data finishes
//        executing,  */
//        if(queue_family_properties[i].queueCount & VK_QUEUE_TRANSFER_BIT)
//        {
//            r_transfer_queue_family_index = i;
//            if(r_transfer_queue_family_index != r_graphics_queue_family_index)
//            {
//                break;
//            }
//        }
//    }

    for(uint32_t i = 0; i < queue_family_property_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(r_physical_device, i, r_surface, &present_supported);
        if(present_supported)
        {
            r_present_queue_family_index = i;
            break;
        }
    }

    VkDeviceQueueCreateInfo *queue_create_info;
    queue_create_info_count = 1 + (r_graphics_queue_family_index != r_present_queue_family_index);
    queue_create_info = alloca(sizeof(VkDeviceQueueCreateInfo) * queue_create_info_count);

    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].pNext = NULL;
    queue_create_info[0].flags = 0;
    /* all the queues means all the pleasure... */
    queue_create_info[0].queueCount = queue_family_properties[r_graphics_queue_family_index].queueCount;
    queue_create_info[0].queueFamilyIndex = r_graphics_queue_family_index;

    if(queue_create_info_count > 1)
    {
        queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[1].pNext = NULL;
        queue_create_info[1].flags = 0;
        /* a single queue for presenting is enough */
        queue_create_info[1].queueCount = 1;
        queue_create_info[1].queueFamilyIndex = r_present_queue_family_index;
    }

    vkEnumerateDeviceExtensionProperties(r_physical_device, NULL, &extension_count, NULL);
    extensions = alloca(sizeof(char **) * extension_count);
    extension_properties = alloca(sizeof(VkExtensionProperties) * extension_count);
    vkEnumerateDeviceExtensionProperties(r_physical_device, NULL, &extension_count, extension_properties);

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
    vkCreateDevice(r_physical_device, &device_create_info, NULL, &r_device);

    if(r_supports_push_descriptors)
    {
        vkCmdPushDescriptorSet = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(r_device, "vkCmdPushDescriptorSetKHR");
    }

    vkGetDeviceQueue(r_device, r_graphics_queue_family_index, 0, &r_graphics_queue);
    vkGetDeviceQueue(r_device, r_present_queue_family_index, 0, &r_present_queue);

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0;
    vkCreateFence(r_device, &fence_create_info, NULL, &r_transfer_fence);

    /*
    =================================================================
    =================================================================
    =================================================================
    */

    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = NULL;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = r_graphics_queue_family_index;
    vkCreateCommandPool(r_device, &command_pool_create_info, NULL, &r_command_pool);

    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.commandPool = r_command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 2;
    vkAllocateCommandBuffers(r_device, &allocate_info, r_command_buffers);

    /*
    =================================================================
    =================================================================
    =================================================================
    */

    VkImageCreateInfo copy_image_create_info = {};
    copy_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    copy_image_create_info.pNext = NULL;
    copy_image_create_info.flags = 0;
    copy_image_create_info.imageType = VK_IMAGE_TYPE_2D;
    copy_image_create_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    copy_image_create_info.extent.width = 2048;
    copy_image_create_info.extent.height = 2048;
    copy_image_create_info.extent.depth = 1;
    copy_image_create_info.mipLevels = 1;
    copy_image_create_info.arrayLayers = 1;
    copy_image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    copy_image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
    copy_image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    copy_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    copy_image_create_info.queueFamilyIndexCount = 0;
    copy_image_create_info.pQueueFamilyIndices = NULL;
    copy_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage copy_image;
    vkCreateImage(r_device, &copy_image_create_info, NULL, &copy_image);

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(r_device, copy_image, &memory_requirements);

    VkMemoryAllocateInfo texture_memory_allocate_info = {};
    texture_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texture_memory_allocate_info.pNext = NULL;
    texture_memory_allocate_info.allocationSize = memory_requirements.size;
    texture_memory_allocate_info.memoryTypeIndex = r_MemoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(r_device, &texture_memory_allocate_info, NULL, &r_texture_copy_memory);
    vkDestroyImage(r_device, copy_image, NULL);
}

/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(r_physical_device, &memory_properties);

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


/*
=================================================================
=================================================================
=================================================================
*/

struct r_pipeline_handle_t r_CreatePipeline(struct r_pipeline_description_t* description)
{
    struct r_pipeline_handle_t handle = R_INVALID_PIPELINE_HANDLE;
//    struct r_pipeline_t* pipeline;
//    struct r_vertex_binding_t *bindings;
//
//    handle.index = add_stack_list_element(&r_renderer.pipelines, NULL);
//    pipeline = (struct r_pipeline_t*)get_stack_list_element(&r_renderer.pipelines, handle.index);
//
//    memcpy(&pipeline->description, description, sizeof(struct r_pipeline_description_t));
    // pipeline->description.input_state.bindings = (struct r_vertex_binding_t*)calloc(description->input_state.binding_count,
        // sizeof(struct r_vertex_binding_t));
    // bindings = pipeline->description.input_state.bindings;
    // for(uint32_t i = 0; i < pipeline->description.input_state.binding_count; i++)
    // {
    //     memcpy(bindings + i, description->input_state.bindings + i, sizeof(struct r_vertex_binding_t));
    //     bindings[i].attribs = (struct r_vertex_attrib_t*)calloc(bindings[i].attrib_count, sizeof(struct r_vertex_attrib_t));
    //     memcpy(bindings[i].attribs, description->input_state.bindings[i].attribs, sizeof(struct r_vertex_attrib_t) * bindings[i].attrib_count);
    // }

//    r_vk_CreatePipeline(pipeline);

    return handle;
}

void r_DestroyPipeline(struct r_pipeline_handle_t handle)
{

}

void r_BindPipeline(struct r_pipeline_handle_t handle)
{

}

struct r_pipeline_t *r_GetPipelinePointer(struct r_pipeline_handle_t handle)
{
    return NULL;
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_CreateDefaultTexture()
{
    struct r_texture_handle_t default_texture;
    struct r_texture_description_t description = {};;
    struct r_texture_t *texture;
    uint32_t *default_texture_pixels;

    description.width = 8;
    description.height = 8;
    description.name = "default_texture";
    description.format = VK_FORMAT_R8G8B8A8_UNORM;
    description.sampler_params.min_filter = VK_FILTER_NEAREST;
    description.sampler_params.mag_filter = VK_FILTER_NEAREST;
    description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    default_texture = r_CreateTexture(&description);

    default_texture_pixels = (uint32_t*)calloc(sizeof(uint32_t), 8 * 8);
    for(uint32_t y = 0; y < 8; y++)
    {
        for(uint32_t x = 0; x < 8; x++)
        {
            default_texture_pixels[y * 8 + x] = ((x + y) % 2) ? 0xff111111 : 0xff222222;
        }
    }

    r_LoadTextureData(default_texture, default_texture_pixels);
}
struct r_texture_handle_t r_AllocTexture()
{
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_description_t *description;

    SDL_AtomicLock(&r_resource_spinlock);
    handle.index = add_stack_list_element(&r_textures, NULL);
    add_stack_list_element(&r_texture_descriptions, NULL);
    SDL_AtomicUnlock(&r_resource_spinlock);

    description = get_stack_list_element(&r_texture_descriptions, handle.index);
    description->aspect_mask = 0;

    return handle;
}

void r_FreeTexture(struct r_texture_handle_t handle)
{
    struct r_texture_t *texture = r_GetTexturePointer(handle);
    struct r_texture_description_t *description;

    if(texture && handle.index != R_DEFAULT_TEXTURE_INDEX && handle.index != R_MISSING_TEXTURE_INDEX)
    {
        description = r_GetTextureDescriptionPointer(handle);

        vkDestroyImage(r_device, texture->image, NULL);
        vkDestroyImageView(r_device, texture->image_view, NULL);
        vkFreeMemory(r_device, texture->memory, NULL);

        texture->image = VK_NULL_HANDLE;
        description->aspect_mask = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
        SDL_AtomicLock(&r_resource_spinlock);
        remove_stack_list_element(&r_textures, handle.index);
        remove_stack_list_element(&r_texture_descriptions, handle.index);
        SDL_AtomicUnlock(&r_resource_spinlock);
    }
}

struct r_texture_handle_t r_CreateTexture(struct r_texture_description_t *description)
{
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_t *texture;
    struct r_texture_description_t *texture_description;
    uint32_t usage = 0;
    VkImageCreateInfo image_create_info = {};
    VkImageViewCreateInfo image_view_create_info = {};
    VkMemoryRequirements memory_requirements;
    VkMemoryAllocateInfo allocate_info = {};

    if(description)
    {
        handle = r_AllocTexture();
        texture = r_GetTexturePointer(handle);
        texture_description = r_GetTextureDescriptionPointer(handle);
        memcpy(texture_description, description, sizeof(struct r_texture_description_t));

        if(!texture_description->type)
        {
            if(!texture_description->height)
            {
                texture_description->type = VK_IMAGE_TYPE_1D;
                texture_description->height = 1;
                texture_description->depth = 1;
            }
            else if(!texture_description->depth)
            {
                texture_description->type = VK_IMAGE_TYPE_2D;
                texture_description->depth = 1;
            }
            else
            {
                texture_description->type = VK_IMAGE_TYPE_3D;
            }
        }

        if(!texture_description->mip_levels)
        {
            texture_description->mip_levels = 1;
        }

        if(!texture_description->layers)
        {
            texture_description->layers = 1;
        }


        usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        switch(texture_description->format)
        {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
                texture_description->aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
                usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            break;

            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                texture_description->aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            break;

            default:
                texture_description->aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
                usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            break;
        }

        texture->current_layout = VK_IMAGE_LAYOUT_UNDEFINED;

        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = NULL;
        image_create_info.flags = 0;
        image_create_info.imageType = texture_description->type;
        image_create_info.format = texture_description->format;
        image_create_info.extent.width = texture_description->width;
        image_create_info.extent.height = texture_description->height;
        image_create_info.extent.depth = texture_description->depth;
        image_create_info.mipLevels = texture_description->mip_levels;
        image_create_info.arrayLayers = texture_description->layers;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        /* code responsible for uploading texel data to this image will
        deal with this */
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = usage;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.queueFamilyIndexCount = 0;
        image_create_info.pQueueFamilyIndices = NULL;
        image_create_info.initialLayout = texture->current_layout;
        vkCreateImage(r_device, &image_create_info, NULL, &texture->image);

        /* images must have memory bound to them before creating a view */
        vkGetImageMemoryRequirements(r_device, texture->image, &memory_requirements);
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.pNext = NULL;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = r_MemoryTypeFromProperties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        vkAllocateMemory(r_device, &allocate_info, NULL, &texture->memory);
        vkBindImageMemory(r_device, texture->image, texture->memory, 0);

        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = NULL;
        image_view_create_info.flags = 0;
        image_view_create_info.image = texture->image;

        switch(texture_description->type)
        {
            case VK_IMAGE_TYPE_1D:
                if(texture_description->layers > 1)
                {
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                }
                else
                {
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_1D;
                }
            break;

            case VK_IMAGE_TYPE_2D:
                if(texture_description->layers > 1)
                {
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                }
                else
                {
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                }
            break;
        }

        image_view_create_info.format = texture_description->format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;

        image_view_create_info.subresourceRange.aspectMask = texture_description->aspect_mask;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = texture_description->mip_levels;
        image_view_create_info.subresourceRange.layerCount = texture_description->layers;
        vkCreateImageView(r_device, &image_view_create_info, NULL, &texture->image_view);

        texture->sampler = r_TextureSampler(&texture_description->sampler_params);
    }

    return handle;
}

struct r_sampler_t *r_TextureSampler(struct r_sampler_params_t *params)
{
    struct r_sampler_t *sampler = NULL;
    uint32_t sampler_index;

    for(sampler_index = 0; sampler_index < r_samplers.cursor; sampler_index++)
    {
        sampler = (struct r_sampler_t *)get_stack_list_element(&r_samplers, sampler_index);

        if(!memcmp(&sampler->params, params, sizeof(struct r_sampler_params_t)))
        {
            break;
        }
    }

    if(sampler_index >= r_samplers.cursor)
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

        sampler_index = add_stack_list_element(&r_samplers, NULL);
        sampler = (struct r_sampler_t *)get_stack_list_element(&r_samplers, sampler_index);

        vkCreateSampler(r_device, &sampler_create_info, NULL, &sampler->sampler);
        memcpy(&sampler->params, params, sizeof(struct r_sampler_params_t));
    }

    return sampler;
}

void r_SetImageLayout(VkImage image, uint32_t aspect_mask, uint32_t old_layout, uint32_t new_layout)
{
    VkImageMemoryBarrier memory_barrier = {};
    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.pNext = NULL;
    /* all writes to this texture must become visible before the layout can change */
    memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    /* new layout in place, now the memory becomes accessible */
    memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    memory_barrier.oldLayout = old_layout;
    memory_barrier.newLayout = new_layout;
    memory_barrier.srcQueueFamilyIndex = r_graphics_queue_family_index;
    memory_barrier.dstQueueFamilyIndex = r_graphics_queue_family_index;
    memory_barrier.image = image;
    memory_barrier.subresourceRange.aspectMask = aspect_mask;
    memory_barrier.subresourceRange.baseArrayLayer = 0;
    memory_barrier.subresourceRange.baseMipLevel = 0;
    memory_barrier.subresourceRange.layerCount = 1;
    memory_barrier.subresourceRange.levelCount = 1;

    vkCmdPipelineBarrier(r_command_buffers[1], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &memory_barrier);
}

void r_SetTextureLayout(struct r_texture_handle_t handle, uint32_t layout)
{
    struct r_texture_t *texture;
    struct r_texture_description_t *description;

    texture = r_GetTexturePointer(handle);
    if(texture)
    {
        description = r_GetTextureDescriptionPointer(handle);
        r_SetImageLayout(texture->image, description->aspect_mask, texture->current_layout, layout);
        texture->current_layout = layout;
    }
}

void r_BlitImageToTexture(struct r_texture_handle_t handle, VkImage image)
{
    struct r_texture_t *texture;
    struct r_texture_description_t *description;
    VkFilter filter;
    VkImageBlit region = {};
    texture = r_GetTexturePointer(handle);

    if(texture)
    {
        description = r_GetTextureDescriptionPointer(handle);

        region.srcSubresource.aspectMask = description->aspect_mask;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource = region.srcSubresource;

        region.srcOffsets[1].x = description->width;
        region.srcOffsets[1].y = description->height;
        region.srcOffsets[1].z = description->depth;
        region.dstOffsets[1] = region.srcOffsets[1];

        switch(description->format)
        {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                filter = VK_FILTER_NEAREST;
            break;

            default:
                filter = VK_FILTER_LINEAR;
            break;
        }


        vkCmdBlitImage(r_command_buffers[1], image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->image,
                                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, filter);
    }
}

void r_LoadTextureData(struct r_texture_handle_t handle, void *data)
{
    struct r_texture_t *texture;
    struct r_texture_description_t *description;
    void *memory;
    uint32_t offset;
    texture = r_GetTexturePointer(handle);
    VkImageCreateInfo copy_texture_create_info = {};
    VkImage copy_texture;

    if(texture && data)
    {
        description = r_GetTextureDescriptionPointer(handle);

        copy_texture_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        copy_texture_create_info.pNext = NULL;
        copy_texture_create_info.flags = 0;
        copy_texture_create_info.imageType = description->type;
        copy_texture_create_info.format = description->format;
        copy_texture_create_info.extent.width = description->width;
        copy_texture_create_info.extent.height = description->height;
        copy_texture_create_info.extent.depth = description->depth;
        copy_texture_create_info.mipLevels = description->mip_levels;
        copy_texture_create_info.arrayLayers = description->layers;
        copy_texture_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        copy_texture_create_info.tiling = VK_IMAGE_TILING_LINEAR;
        copy_texture_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        copy_texture_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        copy_texture_create_info.queueFamilyIndexCount = 0;
        copy_texture_create_info.pQueueFamilyIndices = NULL;
        copy_texture_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vkCreateImage(r_device, &copy_texture_create_info, NULL, &copy_texture);

        VkMemoryRequirements memory_requirements = {};
        vkGetImageMemoryRequirements(r_device, copy_texture, &memory_requirements);
        vkBindImageMemory(r_device, copy_texture, r_texture_copy_memory, 0);

        VkSubresourceLayout subresource_layout;
        VkImageSubresource subresource = {};
        subresource.arrayLayer = 0;
        subresource.aspectMask = description->aspect_mask;
        subresource.mipLevel = 0;
        vkGetImageSubresourceLayout(r_device, copy_texture, &subresource, &subresource_layout);

        vkMapMemory(r_device, r_texture_copy_memory, 0, memory_requirements.size, 0, &memory);
        for(uint32_t row = 0; row < description->height; row++)
        {
            offset = row * subresource_layout.rowPitch;
            memcpy((char *)memory + offset, (char *)data + offset, subresource_layout.rowPitch);
        }
        vkUnmapMemory(r_device, r_texture_copy_memory);

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = NULL;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = NULL;

        vkResetCommandBuffer(r_command_buffers[1], 0);
        vkBeginCommandBuffer(r_command_buffers[1], &begin_info);
        r_SetImageLayout(copy_texture, description->aspect_mask, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        r_SetTextureLayout(handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        r_BlitImageToTexture(handle, copy_texture);
        r_SetTextureLayout(handle, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vkEndCommandBuffer(r_command_buffers[1]);

        vkResetFences(r_device, 1, &r_transfer_fence);
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = r_command_buffers + 1;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = NULL;
        vkQueueSubmit(r_graphics_queue, 1, &submit_info, r_transfer_fence);
        vkWaitForFences(r_device, 1, &r_transfer_fence, VK_TRUE, 0xffffffff);
        vkDestroyImage(r_device, copy_texture, NULL);
    }
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
        description.width = width;
        description.height = height;
        description.format = VK_FORMAT_R8G8B8A8_UNORM;
        description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.mag_filter = VK_FILTER_LINEAR;
        description.sampler_params.min_filter = VK_FILTER_LINEAR;
        description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if(texture_name)
        {
            description.name = strdup(texture_name);
        }
        else
        {
            description.name = strdup(file_name);
        }

        handle = r_CreateTexture(&description);

        r_LoadTextureData(handle, pixels);

        printf("texture %s loaded!\n", description.name);
    }

    return handle;
}

struct r_texture_t *r_GetTexturePointer(struct r_texture_handle_t handle)
{
    struct r_texture_description_t *description;
    struct r_texture_t *texture = NULL;
    description = r_GetTextureDescriptionPointer(handle);

    if(description)
    {
        texture = (struct r_texture_t *)get_stack_list_element(&r_textures, handle.index);
    }

    return texture;
}

struct r_texture_description_t *r_GetTextureDescriptionPointer(struct r_texture_handle_t handle)
{
    struct r_texture_description_t *description;
    description = get_stack_list_element(&r_texture_descriptions, handle.index);

    if(description && description->aspect_mask == (uint8_t)VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM)
    {
        description = NULL;
    }

    return description;
}

struct r_texture_t* r_GetDefaultTexturePointer()
{
    return r_GetTexturePointer(R_TEXTURE_HANDLE(R_DEFAULT_TEXTURE_INDEX));
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

void r_SetTexture(struct r_texture_handle_t handle, uint32_t sampler_index)
{
    // struct r_texture_t *texture = r_GetTexturePointer(handle);

    // if(texture && sampler_index < R_SAMPLER_COUNT)
    // {
    //     // r_vk_SetTexture((struct r_vk_texture_t *)texture, sampler_index);
    //     r_renderer.material_state.textures[sampler_index] = texture;
    //     r_renderer.material_state.outdated_textures = 1;
    // }
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
    struct r_vertex_attrib_t *attrib;
    struct r_vertex_binding_t *binding;
    struct r_resource_binding_t *resource_binding;
    uint32_t attrib_count;
    uint32_t alloc_size;
    VkDescriptorSetLayoutBinding *layout_bindings;
    VkDescriptorSetLayoutBinding *layout_binding;

    if(description)
    {
        handle.index = add_stack_list_element(&r_shaders, NULL);
        shader = (struct r_shader_t *)get_stack_list_element(&r_shaders, handle.index);
        memset(shader, 0, sizeof(struct r_shader_t));

        if(description->push_constant_count)
        {
            shader->push_constants = (struct r_push_constant_t *)calloc(description->push_constant_count,
                                                                    sizeof(struct r_push_constant_t));
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
            vkCreateDescriptorSetLayout(r_device, &layout_create_info, NULL, &shader->descriptor_set_layout);
        }

        VkShaderModuleCreateInfo module_create_info = {};
        module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_create_info.pNext = NULL;
        module_create_info.flags = 0;
        module_create_info.codeSize = description->code_size;
        module_create_info.pCode = description->code;
        vkCreateShaderModule(r_device, &module_create_info, NULL, &shader->module);
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
//    shader = (struct r_shader_t *)get_stack_list_element(&r_renderer.shaders, handle.index);
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
//    struct r_render_pass_t *render_pass;
//    struct r_attachment_description_t *attachment;
//    struct r_attachment_reference_t *reference;
//    struct r_subpass_description_t subpass = {};
//    uint32_t attachment_size;
//    uint32_t depth_stencil_index = 0xffffffff;
//    uint32_t depth_stencil_layout = R_LAYOUT_UNDEFINED;
//
//    handle.index = add_stack_list_element(&r_renderer.render_passes, NULL);
//    render_pass = (struct r_render_pass_t *)get_stack_list_element(&r_renderer.render_passes, handle.index);
//    memcpy(&render_pass->description, description, sizeof(struct r_render_pass_description_t));
//
//    attachment_size = sizeof(struct r_attachment_description_t) * description->attachment_count;
//    render_pass->description.attachments = (struct r_attachment_description_t *)calloc(1, attachment_size);
//    memcpy(render_pass->description.attachments, description->attachments, attachment_size);
//
//    if(render_pass->description.subpass_count)
//    {
////        render_pass->description.subpasses = (struct r_subpass_description_t *)calloc(description->subpass_count, sizeof(struct r_subpass_description_t));
////        memcpy(render_pass->description.subpasses, description->subpasses, sizeof(struct r_subpass_description_t) * description->subpass_count);
//    }
//    else
//    {
//        /* no subpasses given, so create one that uses all attachments,
//        and don't cause any layout transitions */
//
//
//        /* since this subpass is being created only for
//        convenience of further ahead code a local var will be
//        used, since it is not meant to be accessed after this
//        function returns */
//        render_pass->description.subpasses = &subpass;
//        render_pass->description.subpass_count = 1;
//
//        /* a single block of memory for all color references and
//        depth_stencil reference */
//        subpass.color_references = (struct r_attachment_reference_t *)alloca(sizeof(struct r_attachment_reference_t) *
//                                                                description->attachment_count);
//
//        subpass.color_reference_count = 0;
//        for(uint32_t i = 0; i < description->attachment_count; i++)
//        {
//            attachment = render_pass->description.attachments + i;
//            switch(attachment->format)
//            {
//                case R_FORMAT_D32_SFLOAT:
//                case R_FORMAT_D24_UNORM_S8_UINT:
//                    depth_stencil_index = i;
//                    continue;
//                break;
//            }
//
//            reference = subpass.color_references + subpass.color_reference_count;
//            subpass.color_reference_count++;
//            reference->attachment = i;
//            /* this will assume that the desired layout for the color
//            attachment will be the optimal one */
//            reference->layout = R_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//            if(attachment->final_layout == R_LAYOUT_UNDEFINED)
//            {
//                /* the Vulkan spec states that the final layout of an attachment
//                must not be undefined, so we fix it here in case that happens */
//                attachment->final_layout = R_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//            }
//        }
//
//        if(depth_stencil_index != 0xffffffff)
//        {
//            attachment = render_pass->description.attachments + depth_stencil_index;
//            reference = subpass.color_references + subpass.color_reference_count;
//            reference->attachment = depth_stencil_index;
//            reference->layout = R_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//            subpass.depth_stencil_reference = reference;
//
//            if(attachment->final_layout == R_LAYOUT_UNDEFINED)
//            {
//                /* the Vulkan spec states that the final layout of an attachment
//                must not be undefined, so we fix it here in case that happens */
//                attachment->final_layout = R_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//            }
//        }
//    }
//
//    r_vk_CreateRenderPass(render_pass);

    return handle;
}

void r_DestroyRenderPass(struct r_render_pass_handle_t handle)
{

}

struct r_render_pass_t *r_GetRenderPassPointer(struct r_render_pass_handle_t handle)
{
    struct r_render_pass_t *render_pass;
//
//    render_pass = (struct r_render_pass_t *)get_stack_list_element(&r_renderer.render_passes, handle.index);
//
//    if(render_pass && !(render_pass->description.attachment_count && render_pass->description.subpass_count))
//    {
//        render_pass = NULL;
//    }
//
    return render_pass;
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_framebuffer_handle_t r_CreateFramebuffer(struct r_framebuffer_description_t *description)
{
    struct r_framebuffer_handle_t handle = R_INVALID_FRAMEBUFFER_HANDLE;
//    struct r_framebuffer_t *framebuffer;
//    if(description)
//    {
//        handle.index = add_stack_list_element(&r_renderer.framebuffers, NULL);
//        framebuffer = (struct r_framebuffer_t *)get_stack_list_element(&r_renderer.render_passes, handle.index);
//        memcpy(&framebuffer->description, description, sizeof(struct r_framebuffer_description_t));
//
//        framebuffer->description.attachments = (struct r_attachment_description_t *)calloc(description->attachment_count,
//                                                                        sizeof(struct r_attachment_description_t));
//
//        memcpy(framebuffer->description.attachments, description->attachments, sizeof(struct r_attachment_description_t) *
//                                                                        description->attachment_count);
//
//
//
//    }

    return handle;
}

void r_DestroyFramebuffer(struct r_framebuffer_handle_t handle)
{

}

struct r_framebuffer_t *r_GetFramebufferPointer(struct r_framebuffer_handle_t handle)
{
    struct r_framebuffer_t *framebuffer;

//    framebuffer = (struct r_framebuffer_t *)get_stack_list_element(&r_renderer.framebuffers, handle.index);
//    if(framebuffer && !framebuffer->description.attachment_count)
//    {
//        framebuffer = NULL;
//    }

    return framebuffer;
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_alloc_handle_t r_Alloc(uint32_t size, uint32_t align, uint32_t index_alloc)
{
    struct r_alloc_t *alloc;
    struct r_alloc_t new_alloc;
    struct r_alloc_handle_t handle = R_INVALID_ALLOC_HANDLE;
    uint32_t start;
    uint32_t block_size;
    struct list_t *free_blocks;

    index_alloc = index_alloc && 1;
    // align--;

    free_blocks = &r_renderer.free_blocks[index_alloc];

    for(uint32_t i = 0; i < free_blocks->cursor; i++)
    {
        alloc = (struct r_alloc_t *)get_list_element(free_blocks, i);
        /* align the start of the allocation to the desired alignment */
        // start = (alloc->start + align) & (~align);

        start = alloc->start;

        while(start % align)
        {
            start += align - (start % align);
        }

        /* subtract the bytes before the aligned start from
        the total size */
        block_size = alloc->size - (start - alloc->start);

        if(block_size >= size)
        {
            /* compute the how many bytes of alignment we need */
            align = start - alloc->start;

            if(block_size == size)
            {
                /* taking away the aligment bytes, this buffer is just the right size for
                the allocation required, so just move this allocation header from the
                free list to the allocation list*/
                alloc->align = align;
                handle.alloc_index = add_stack_list_element(&r_renderer.allocd_blocks[index_alloc], alloc);
                remove_list_element(free_blocks, i);
            }
            else
            {
                /* this buffer is bigger than the requested
                size, even after adjusting for alignment */
                block_size = size + align;

                /* we'll create a new allocation header to add
                in the allocations list */
                new_alloc.start = alloc->start;
                new_alloc.size = block_size;
                new_alloc.align = align;

                handle.alloc_index = add_stack_list_element(&r_renderer.allocd_blocks[index_alloc], &new_alloc);

                /* chop a chunk of this free block */
                alloc->start += block_size;
                alloc->size -= block_size;
            }

            // handle.alloc_index = i;
            handle.is_index = index_alloc;
            break;
        }
    }

    return handle;
}

struct r_alloc_t *r_GetAllocPointer(struct r_alloc_handle_t handle)
{
    struct r_alloc_t *alloc;
    alloc = (struct r_alloc_t *)get_stack_list_element(&r_renderer.allocd_blocks[handle.is_index], handle.alloc_index);

    if(alloc && !alloc->size)
    {
        /* zero sized allocs are considered invalid */
        alloc = NULL;
    }

    return alloc;
}

void r_Free(struct r_alloc_handle_t handle)
{
    struct r_alloc_t *alloc = r_GetAllocPointer(handle);

    if(alloc)
    {
        add_list_element(&r_renderer.free_blocks[handle.is_index], alloc);

        /* zero sized allocs are considered invalid */
        alloc->size = 0;
    }
}

void r_Memcpy(struct r_alloc_handle_t handle, void *data, uint32_t size)
{
    void *memory;
    struct r_alloc_t *alloc;
    uint32_t alloc_size;

    alloc = r_GetAllocPointer(handle);

    if(alloc)
    {
//        alloc_size = alloc->size - alloc->align;
//
//        if(size > alloc_size)
//        {
//            size = alloc_size;
//        }
//
//        /* backend specific mapping function. It's necessary to pass both
//        the handle and the alloc, as the backend doesn't use any of the
//        interface's functions, and can't get the alloc pointer from the
//        handle. It also needs the handle to know whether it's an vertex
//        or and index alloc. */
//        memory = r_vk_MapAlloc(handle, alloc);
//        memcpy(memory, data, size);
//        r_vk_UnmapAlloc(handle);
    }
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_SetZPlanes(float z_near, float z_far)
{
    r_renderer.z_near = z_near;
    r_renderer.z_far = z_far;
    r_RecomputeViewProjectionMatrix();
}

void r_SetFovY(float fov_y)
{
    r_renderer.fov_y = fov_y;
    r_RecomputeViewProjectionMatrix();
}

void r_RecomputeViewProjectionMatrix()
{
    mat4_t_persp(&r_renderer.projection_matrix, r_renderer.fov_y,
        (float)r_renderer.width / (float)r_renderer.height, r_renderer.z_near, r_renderer.z_far);
//    r_vk_AdjustProjectionMatrix(&r_renderer.projection_matrix);
    mat4_t_mul(&r_renderer.view_projection_matrix, &r_renderer.view_matrix, &r_renderer.projection_matrix);
}


void r_SetViewMatrix(mat4_t *view_matrix)
{
    memcpy(&r_renderer.view_matrix, view_matrix, sizeof(mat4_t));
    mat4_t_invvm(&r_renderer.view_matrix, &r_renderer.view_matrix);
    r_RecomputeViewProjectionMatrix();
}

void r_GetWindowSize(uint32_t *width, uint32_t *height)
{
    *width = r_renderer.width;
    *height = r_renderer.height;
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_material_handle_t r_AllocMaterial()
{
    struct r_material_handle_t handle = R_INVALID_MATERIAL_HANDLE;
    struct r_material_t *material;

    handle.index = add_stack_list_element(&r_renderer.materials, NULL);

    material = (struct r_material_t*)get_stack_list_element(&r_renderer.materials, handle.index);
    material->flags = 0;
    material->diffuse_texture = NULL;
    material->normal_texture = NULL;

    return handle;
}

void r_FreeMaterial(struct r_material_handle_t handle)
{
    struct r_material_t *material;

    material = r_GetMaterialPointer(handle);

    if(material)
    {
        material->flags = R_MATERIAL_FLAG_INVALID;
        free(material->name);
        remove_stack_list_element(&r_renderer.materials, handle.index);
    }
}

struct r_material_t *r_GetMaterialPointer(struct r_material_handle_t handle)
{
    struct r_material_t *material;
    material = (struct r_material_t*)get_stack_list_element(&r_renderer.materials, handle.index);

    if(material && (material->flags & R_MATERIAL_FLAG_INVALID))
    {
        material = NULL;
    }

    return material;
}

struct r_material_handle_t r_GetMaterialHandle(char *name)
{
    struct r_material_handle_t handle = R_INVALID_MATERIAL_HANDLE;
    struct r_material_t *material;

    for(uint32_t i = 0; i < r_renderer.materials.cursor; i++)
    {
        material = r_GetMaterialPointer(R_MATERIAL_HANDLE(i));

        if(material && !strcmp(name, material->name))
        {
            handle = R_MATERIAL_HANDLE(i);
            break;
        }
    }

    return handle;
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
=================================================================
=================================================================
=================================================================
*/

void *r_AllocCmdData(uint32_t size)
{
    uint32_t elem_count;
    void *data;
    elem_count = ((size + R_CMD_DATA_ELEM_SIZE - 1) & (~(R_CMD_DATA_ELEM_SIZE - 1))) / R_CMD_DATA_ELEM_SIZE;
    /* this is not a safe allocation scheme, and it doesn't have to. Data
    WILL be overwritten if the producer end is queueing stuff too fast and
    the consumer end is taking too long to use the data */

    if(elem_count > r_renderer.cmd_buffer_data.next_in - r_renderer.cmd_buffer_data.buffer_size)
    {
        r_renderer.cmd_buffer_data.next_in = 0;
    }

    data = (char *)r_renderer.cmd_buffer_data.buffer + r_renderer.cmd_buffer_data.next_in * R_CMD_DATA_ELEM_SIZE;
    r_renderer.cmd_buffer_data.next_in += elem_count;
    r_renderer.cmd_buffer_data.next_out = r_renderer.cmd_buffer_data.next_in;

    return data;
}

void *r_AtomicAllocCmdData(uint32_t size)
{
    void *data;

    SDL_AtomicLock(&r_renderer.cmd_buffer_lock);
    data = r_AllocCmdData(size);
    SDL_AtomicUnlock(&r_renderer.cmd_buffer_lock);

    return data;
}

void r_BeginSubmission(mat4_t *view_projection_matrix, mat4_t *view_matrix)
{
    r_renderer.submiting_draw_cmd_buffer = (struct r_draw_cmd_buffer_t*)r_AtomicAllocCmdData(sizeof(struct r_draw_cmd_buffer_t));
    r_renderer.submiting_draw_cmd_buffer->draw_cmd_count = 0;
    memcpy(&r_renderer.submiting_draw_cmd_buffer->view_projection_matrix, view_projection_matrix, sizeof(mat4_t));
    memcpy(&r_renderer.submiting_draw_cmd_buffer->view_matrix, view_matrix, sizeof(mat4_t));
}

void r_SubmitDrawCmd(mat4_t *model_matrix, struct r_material_t *material, uint32_t start, uint32_t count)
{
    struct r_draw_cmd_t *draw_cmd;

    if(r_renderer.submiting_draw_cmd_buffer->draw_cmd_count >= R_DRAW_CMD_BUFFER_DRAW_CMDS)
    {
        r_EndSubmission();
        r_BeginSubmission(&r_renderer.submiting_draw_cmd_buffer->view_projection_matrix, &r_renderer.submiting_draw_cmd_buffer->view_matrix);
    }
    draw_cmd = r_renderer.submiting_draw_cmd_buffer->draw_cmds + r_renderer.submiting_draw_cmd_buffer->draw_cmd_count;
    draw_cmd->material = material;
    draw_cmd->range.start = start;
    draw_cmd->range.count = count;
    memcpy(&draw_cmd->model_matrix, model_matrix, sizeof(mat4_t));
    r_renderer.submiting_draw_cmd_buffer->draw_cmd_count++;
}

void r_EndSubmission()
{
    if(r_renderer.submiting_draw_cmd_buffer->draw_cmd_count)
    {
        r_QueueCmd(R_CMD_TYPE_DRAW, r_renderer.submiting_draw_cmd_buffer, 0);
    }
}


// void r_SortDrawCmds(struct r_udraw_cmd_buffer_t *udraw_cmd_buffer)
// {
//     // struct r_udraw_cmd_buffer_t *udraw_cmd_buffer;
//     struct r_udraw_cmd_t *udraw_cmd;
//     struct r_draw_cmd_t *draw_cmd;
//     struct r_draw_cmd_buffer_t *draw_cmd_buffer = NULL;
//     struct r_material_t *current_material = NULL;

//     // udraw_cmd_buffer = (struct r_udraw_cmd_buffer_t *)cmd->data;
//     qsort(udraw_cmd_buffer->draw_cmds, udraw_cmd_buffer->draw_cmd_count, sizeof(struct r_udraw_cmd_t), r_CmpDrawCmd);

//     // for(uint32_t i = 0; i < udraw_cmd_buffer->draw_cmd_count; i++)
//     // {
//     //     printf("material %s\n", udraw_cmd_buffer->draw_cmds[i].material->name);
//     // }

//     draw_cmd_buffer = r_AtomicAllocCmdData(sizeof(struct r_draw_cmd_buffer_t) + sizeof(struct r_draw_cmd_t) * (R_MAX_DRAW_CMDS - 1));
//     draw_cmd_buffer->draw_cmd_count = 0;
//     draw_cmd_buffer->view_projection_matrix = udraw_cmd_buffer->view_projection_matrix;
//     draw_cmd_buffer->view_matrix = udraw_cmd_buffer->view_matrix;
//     draw_cmd_buffer->material = udraw_cmd_buffer->draw_cmds[0].material;
//     current_material = draw_cmd_buffer->material;

//     for(uint32_t i = 0; i < udraw_cmd_buffer->draw_cmd_count; i++)
//     {
//         udraw_cmd = udraw_cmd_buffer->draw_cmds + i;

//         if(udraw_cmd->material != current_material ||
//             draw_cmd_buffer->draw_cmd_count >= R_MAX_DRAW_CMDS)
//         {
//             current_material = udraw_cmd->material;
//             r_QueueCmd(R_CMD_TYPE_DRAW, draw_cmd_buffer, 0);

//             draw_cmd_buffer = r_AtomicAllocCmdData(sizeof(struct r_draw_cmd_buffer_t) + sizeof(struct r_draw_cmd_t) * (R_MAX_DRAW_CMDS - 1));
//             draw_cmd_buffer->material = current_material;
//             draw_cmd_buffer->view_projection_matrix = udraw_cmd_buffer->view_projection_matrix;
//             draw_cmd_buffer->view_matrix = udraw_cmd_buffer->view_matrix;
//             draw_cmd_buffer->draw_cmd_count = 0;
//         }

//         draw_cmd = draw_cmd_buffer->draw_cmds + draw_cmd_buffer->draw_cmd_count;
//         draw_cmd->model_matrix = udraw_cmd->model_matrix;
//         draw_cmd->range = udraw_cmd->range;
//         draw_cmd_buffer->draw_cmd_count++;
//     }

//     if(draw_cmd_buffer->draw_cmd_count)
//     {
//         r_QueueCmd(R_CMD_TYPE_DRAW, draw_cmd_buffer, 0);
//     }
// }

void r_QueueCmd(uint32_t type, void *data, uint32_t data_size)
{
    struct r_cmd_t cmd;

    cmd.cmd_type = type;

    SDL_AtomicLock(&r_renderer.cmd_buffer_lock);

    if(data && data_size)
    {
        cmd.data = r_AllocCmdData(data_size);
        memcpy(cmd.data, data, data_size);
    }
    else
    {
        cmd.data = data;
    }

    add_ringbuffer_element(&r_renderer.cmd_buffer, &cmd);

    SDL_AtomicUnlock(&r_renderer.cmd_buffer_lock);
}
struct r_cmd_t *r_NextCmd()
{
    return (struct r_cmd_t *)peek_ringbuffer_element(&r_renderer.cmd_buffer);
}

void r_AdvanceCmd()
{
    SDL_AtomicLock(&r_renderer.cmd_buffer_lock);
    get_ringbuffer_element(&r_renderer.cmd_buffer);
    SDL_AtomicUnlock(&r_renderer.cmd_buffer_lock);
}

int r_ExecuteCmds(void *data)
{
    struct r_cmd_t *cmd;
    while(1)
    {
        while(r_renderer.cmd_buffer.next_in != r_renderer.cmd_buffer.next_out)
        {
            cmd = r_NextCmd();

            switch(cmd->cmd_type)
            {
                case R_CMD_TYPE_BEGIN_FRAME:
//                    r_vk_BeginFrame();
                break;

                case R_CMD_TYPE_DRAW:
                    r_Draw(cmd);
                break;

                case R_CMD_TYPE_END_FRAME:
//                    r_vk_EndFrame();
                break;
            }

            r_AdvanceCmd();
        }
    }
}

void r_WaitEmptyQueue()
{
    while(r_renderer.cmd_buffer.next_in != r_renderer.cmd_buffer.next_out);
}

int r_CmpDrawCmd(const void *a, const void *b)
{
    return (ptrdiff_t)((struct r_draw_cmd_t *)a)->material - (ptrdiff_t)((struct r_draw_cmd_t *)b)->material;
}

void r_Draw(struct r_cmd_t *cmd)
{
    struct r_draw_cmd_buffer_t *cmd_buffer;
    struct r_draw_cmd_t *draw_cmds;
    uint32_t draw_cmd_index;
    struct r_material_t *cur_material;

    cmd_buffer = (struct r_draw_cmd_buffer_t *)cmd->data;
    qsort(cmd_buffer->draw_cmds, cmd_buffer->draw_cmd_count, sizeof(struct r_draw_cmd_t), r_CmpDrawCmd);
//    r_vk_PushViewMatrix(&cmd_buffer->view_matrix);
    draw_cmds = cmd_buffer->draw_cmds;
    while(cmd_buffer->draw_cmd_count)
    {
        draw_cmd_index = 0;
        cur_material = draw_cmds[0].material;
        while(cur_material == draw_cmds[draw_cmd_index].material)draw_cmd_index++;
//        r_vk_Draw(cur_material, &cmd_buffer->view_projection_matrix, draw_cmds, draw_cmd_index);
        draw_cmds += draw_cmd_index;
        cmd_buffer->draw_cmd_count -= draw_cmd_index;
    }
}

/*
=================================================================================
=================================================================================
=================================================================================
*/

void r_DrawPoint(vec3_t* position, vec3_t* color)
{

}
