#include "r_main.h"
#include "r_vk.h"
#include "stack_list.h"
#include "list.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

struct r_renderer_t r_renderer;

void r_InitRenderer()
{
    struct r_alloc_t free_alloc;

    r_renderer.window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
    
    SDL_GL_SetSwapInterval(1);

    r_renderer.allocd_blocks[0] = create_stack_list(sizeof(struct r_alloc_t), 512);
    r_renderer.allocd_blocks[1] = create_stack_list(sizeof(struct r_alloc_t), 512);

    r_renderer.free_blocks[0] = create_list(sizeof(struct r_alloc_t), 512);
    r_renderer.free_blocks[1] = create_list(sizeof(struct r_alloc_t), 512);

    r_renderer.cmd_buffer = create_ringbuffer(sizeof(struct r_cmd_t), 1024);
    r_renderer.cmd_buffer_data = create_ringbuffer(R_CMD_DATA_ELEM_SIZE, 4096); 

    r_renderer.temp_draw_batch = calloc(sizeof(struct r_draw_batch_t) + sizeof(struct r_draw_cmd_t) * (R_MAX_TEMP_DRAW_BATCH_SIZE - 1), 1);
    r_renderer.temp_draw_batch->material = R_INVALID_MATERIAL_HANDLE;

    free_alloc.start = 0;
    free_alloc.size = R_HEAP_SIZE;
    free_alloc.align = 0;

    r_renderer.z_far = 100.0;
    r_renderer.z_near = 0.01;

    add_list_element(&r_renderer.free_blocks[0], &free_alloc);
    add_list_element(&r_renderer.free_blocks[1], &free_alloc);

    mat4_t_identity(&r_renderer.view_matrix);
    mat4_t_identity(&r_renderer.projection_matrix);

    r_InitVkRenderer();
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
    align--;

    free_blocks = &r_renderer.free_blocks[index_alloc];

    for(uint32_t i = 0; i < free_blocks->cursor; i++)
    {
        alloc = get_list_element(free_blocks, i);
        /* align the start of the allocation to the desired alignment */
        start = (alloc->start + align) & (~align);
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
                add_stack_list_element(&r_renderer.allocd_blocks[index_alloc], alloc);
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

                add_stack_list_element(&r_renderer.allocd_blocks[index_alloc], &new_alloc);

                /* chop a chunk of this free block */
                alloc->start += block_size;
                alloc->size -= block_size;
            }
            
            handle.alloc_index = i;
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

// void r_SetProjectionMatrix(mat4_t *projection_matrix)
// {
//     memcpy(&r_renderer.projection_matrix, projection_matrix, sizeof(mat4_t));
//     r_vk_SetProjectionMatrix(&r_renderer.projection_matrix);
//     r_renderer.outdated_view_projection_matrix = 1;
// }

// void r_SetViewMatrix(mat4_t *view_matrix)
// {
//     memcpy(&r_renderer.view_matrix, view_matrix, sizeof(mat4_t));
//     mat4_t_invvm(&r_renderer.view_matrix);
//     r_renderer.outdated_view_projection_matrix = 1;
// }

// void r_SetModelMatrix(mat4_t *model_matrix)
// {
//     memcpy(&r_renderer.model_matrix, model_matrix, sizeof(mat4_t));
// }

// void r_UpdateMatrices()
// {
//     mat4_t model_view_projection_matrix;

//     if(r_renderer.outdated_view_projection_matrix)
//     {
//         mat4_t_mul(&r_renderer.view_projection_matrix, &r_renderer.view_matrix, &r_renderer.projection_matrix);
//         r_renderer.outdated_view_projection_matrix = 0;
//     }

//     mat4_t_mul(&r_renderer.model_view_projection_matrix, &r_renderer.model_matrix, &r_renderer.view_projection_matrix);
// }

void r_SetViewProjectionMatrix(mat4_t *view_matrix, mat4_t *projection_matrix)
{
    if(view_matrix)
    {
        memcpy(&r_renderer.view_matrix, view_matrix, sizeof(mat4_t));
        mat4_t_invvm(&r_renderer.view_matrix);
    }

    if(projection_matrix)
    {
        memcpy(&r_renderer.projection_matrix, projection_matrix, sizeof(mat4_t));
        r_vk_SetProjectionMatrix(&r_renderer.projection_matrix);
    }

    mat4_t_mul(&r_renderer.view_projection_matrix, &r_renderer.view_matrix, &r_renderer.projection_matrix);
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_texture_handle_t r_AllocTexture()
{
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_t *texture;

    handle.index = add_stack_list_element(&r_renderer.textures, NULL);
    texture = (struct r_texture_t *)get_stack_list_element(&r_renderer.textures, handle.index);
    texture->type = R_TEXTURE_TYPE_NONE;

    return handle;
}

void r_FreeTexture(struct r_texture_handle_t handle)
{
    struct r_texture_t *texture = r_GetTexturePointer(handle);

    if(texture)
    {
        texture->type = R_TEXTURE_TYPE_INVALID;
        remove_stack_list_element(&r_renderer.textures, handle.index);
    }
}

struct r_texture_handle_t r_LoadTexture(char *file_name)
{
    unsigned char *pixels;
    int width;
    int height;
    int channels;
    struct r_texture_handle_t handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_t *texture;

    pixels = stbi_load(file_name, &width, &height, &channels, STBI_rgb_alpha);

    if(pixels)
    {
        handle = r_AllocTexture();
        texture = r_GetTexturePointer(handle);

        /* TODO: implement this properly! */
        texture->width = width;
        texture->height = height;
        texture->depth = 1;
        texture->type = R_TEXTURE_TYPE_2D;

        r_vk_InitalizeTexture((struct r_vk_texture_t *)texture, pixels);
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

void r_SetTexture(struct r_texture_handle_t handle, uint32_t sampler_index)
{
    struct r_texture_t *texture = r_GetTexturePointer(handle);
    if(texture && sampler_index < R_SAMPLER_COUNT)
    {
        r_vk_SetTexture((struct r_vk_texture_t *)texture, sampler_index);
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

void r_QueueDrawCmd(mat4_t *model_matrix, struct r_material_handle_t material, struct r_alloc_handle_t alloc)
{
    // if(!r_renderer.temp_draw_batch->draw_cmd_count)
    // {
    //     r_renderer.temp_draw_batch->view_projection_matrix = r_renderer.view_projection_matrix;
    // }

    // struct r_draw_cmd_t *draw_cmd = r_renderer.temp_draw_batch->draw_cmds + r_renderer.temp_draw_batch->draw_cmd_count;
    // struct r_alloc_t *alloc = r_GetAllocPointer(alloc);

    // memcpy(&draw_cmd->model_matrix, model_matrix, sizeof(mat4_t));
    // draw_cmd->vertex_count = 
}

void r_QueueCmd(uint32_t type, void *data, uint32_t data_size)
{
    struct r_cmd_t cmd;

    cmd.cmd_type = type;

    if(data && data_size)
    {
        cmd.data = r_AllocCmdData(data_size);
        memcpy(cmd.data, data, data_size);
    }

    SDL_AtomicLock(&r_renderer.cmd_buffer_lock);
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

void r_ExecuteCmds()
{
    struct r_cmd_t *cmd;

    while(r_renderer.cmd_buffer.next_in != r_renderer.cmd_buffer.next_out)
    {
        cmd = r_NextCmd();

        switch(cmd->cmd_type)
        {
            case R_CMD_TYPE_BEGIN_FRAME:
                r_vk_BeginFrame();
            break;

            case R_CMD_TYPE_DRAW:
                r_vk_Draw(cmd);
            break;

            case R_CMD_TYPE_END_FRAME:
                r_vk_EndFrame();
            break;
        }

        r_AdvanceCmd();
    }
}
