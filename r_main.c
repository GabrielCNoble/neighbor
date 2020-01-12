#include "r_main.h"
#include "dstuff/file/path.h"
#include "r_vk.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/containers/list.h"
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

struct r_renderer_t r_renderer;

void r_InitRenderer()
{
    struct r_alloc_t free_alloc;
    SDL_Thread *renderer_thread;

    r_renderer.z_far = 1000.0;
    r_renderer.z_near = 0.01;
    r_renderer.fov_y = 0.68;
    r_renderer.width = 800;
    r_renderer.height = 600;

    r_RecomputeViewProjectionMatrix();

    r_renderer.window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, r_renderer.width, r_renderer.height, SDL_WINDOW_OPENGL);
    
    SDL_GL_SetSwapInterval(1);

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
    r_vk_InitRenderer();
    renderer_thread = SDL_CreateThread(r_ExecuteCmds, "renderer thread", NULL);
    SDL_DetachThread(renderer_thread);
    r_InitBuiltinTextures();
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
        alloc_size = alloc->size - alloc->align;

        if(size > alloc_size)
        {
            size = alloc_size;
        }

        /* backend specific mapping function. It's necessary to pass both
        the handle and the alloc, as the backend doesn't use any of the
        interface's functions, and can't get the alloc pointer from the 
        handle. It also needs the handle to know whether it's an vertex
        or and index alloc. */
        memory = r_vk_MapAlloc(handle, alloc);
        memcpy(memory, data, size);
        r_vk_UnmapAlloc(handle);
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
    r_vk_AdjustProjectionMatrix(&r_renderer.projection_matrix);
    r_renderer.view_projection_matrix = r_renderer.view_matrix * r_renderer.projection_matrix;
    // mat4_t_mul(&r_renderer.view_projection_matrix, &r_renderer.view_matrix, &r_renderer.projection_matrix);
}


void r_SetViewMatrix(mat4_t *view_matrix)
{
    memcpy(&r_renderer.view_matrix, view_matrix, sizeof(mat4_t));
    mat4_t_invvm(&r_renderer.view_matrix);
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

void r_InitBuiltinTextures()
{
    struct r_texture_handle_t default_texture;
    struct r_texture_t *texture;
    uint32_t *default_texture_pixels;

    default_texture = r_AllocTexture();
    texture = r_GetTexturePointer(default_texture);

    texture->width = 8;
    texture->height = 8;
    texture->depth = 1;
    texture->name = "default_texture";
    texture->type = R_TEXTURE_TYPE_2D;
    texture->sampler_params.min_filter = R_TEXTURE_FILTER_NEAREST;
    texture->sampler_params.mag_filter = R_TEXTURE_FILTER_NEAREST;
    texture->sampler_params.mip_filter = R_TEXTURE_FILTER_NEAREST;

    default_texture_pixels = (uint32_t*)calloc(sizeof(uint32_t), 8 * 8);
    for(uint32_t y = 0; y < 8; y++)
    {
        for(uint32_t x = 0; x < 8; x++)
        {
            default_texture_pixels[y * 8 + x] = ((x + y) % 2) ? 0xff111111 : 0xff222222;
        }
    }

    r_vk_LoadTextureData((struct r_vk_texture_t *)texture, (unsigned char *)default_texture_pixels);
}
struct r_texture_handle_t r_AllocTexture()
{
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_t *texture;

    handle.index = add_stack_list_element(&r_renderer.textures, &texture);

    texture = (struct r_texture_t*)get_stack_list_element(&r_renderer.textures, handle.index);
    texture->type = R_TEXTURE_TYPE_NONE;
    texture->sampler_params.min_filter = R_TEXTURE_FILTER_LINEAR;
    texture->sampler_params.mag_filter = R_TEXTURE_FILTER_LINEAR;
    texture->sampler_params.mip_filter = R_TEXTURE_FILTER_LINEAR;
    texture->sampler_params.addr_mode_u = R_TEXTURE_ADDRESS_MODE_REPEAT;
    texture->sampler_params.addr_mode_v = R_TEXTURE_ADDRESS_MODE_REPEAT;
    texture->sampler_params.addr_mode_w = R_TEXTURE_ADDRESS_MODE_REPEAT;

    if(handle.index != R_DEFAULT_TEXTURE_INDEX)
    {
        r_vk_InitWithDefaultTexture((struct r_vk_texture_t *)texture);
    }

    return handle;
}

void r_FreeTexture(struct r_texture_handle_t handle)
{
    struct r_texture_t *texture = r_GetTexturePointer(handle);
    
    if(texture && handle.index != R_DEFAULT_TEXTURE_INDEX && handle.index != R_MISSING_TEXTURE_INDEX)
    {
        texture->type = R_TEXTURE_TYPE_INVALID;
        remove_stack_list_element(&r_renderer.textures, handle.index);
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

    file_name = format_path(file_name);

    pixels = stbi_load(file_name, &width, &height, &channels, STBI_rgb_alpha);

    if(pixels)
    {
        printf("texture %s loaded!\n", file_name);
        handle = r_AllocTexture();
        texture = r_GetTexturePointer(handle);

        /* TODO: implement this properly! */
        texture->width = width;
        texture->height = height;
        texture->depth = 1;
        texture->type = R_TEXTURE_TYPE_2D;

        if(texture_name)
        {
            texture->name = strdup(texture_name);
        }
        else
        {
            texture->name = strdup(file_name);
        }
        

        r_vk_LoadTextureData((struct r_vk_texture_t *)texture, pixels);
    }

    return handle;
}

struct r_texture_t *r_GetTexturePointer(struct r_texture_handle_t handle)
{
    struct r_texture_t *texture;
    texture = (struct r_texture_t *)get_stack_list_element(&r_renderer.textures, handle.index);

    if(texture && texture->type == R_TEXTURE_TYPE_INVALID)
    {
        texture = NULL;
    }

    return texture;
}

struct r_texture_t* r_GetDefaultTexturePointer()
{
    return r_GetTexturePointer(R_TEXTURE_HANDLE(R_DEFAULT_TEXTURE_INDEX));
}

struct r_texture_handle_t r_GetTextureHandle(char *name)
{
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_t *texture;

    for(uint32_t i = 0; i < r_renderer.textures.cursor; i++)
    {
        texture = r_GetTexturePointer(R_TEXTURE_HANDLE(i));

        if(texture && !strcmp(texture->name, name))
        {
            handle = R_TEXTURE_HANDLE(i);
            break;
        }
    }

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
    // printf("size: %d     elem_count: %d\n", size, elem_count);

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

    // printf("%d\n", r_renderer.cmd_buffer_data.next_in);

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
                    r_vk_BeginFrame();
                break;

                case R_CMD_TYPE_DRAW:
                    r_Draw(cmd);
                break;

                case R_CMD_TYPE_END_FRAME:
                    r_vk_EndFrame();
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
    r_vk_PushViewMatrix(&cmd_buffer->view_matrix);
    draw_cmds = cmd_buffer->draw_cmds;
    while(cmd_buffer->draw_cmd_count)
    {
        draw_cmd_index = 0;
        cur_material = draw_cmds[0].material;
        while(cur_material == draw_cmds[draw_cmd_index].material)draw_cmd_index++;
        r_vk_Draw(cur_material, &cmd_buffer->view_projection_matrix, draw_cmds, draw_cmd_index);
        draw_cmds += draw_cmd_index;
        cmd_buffer->draw_cmd_count -= draw_cmd_index;
    }
}
