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

    r_renderer.window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);

    r_renderer.allocd_blocks[0] = create_stack_list(sizeof(struct r_alloc_t), 512);
    r_renderer.allocd_blocks[1] = create_stack_list(sizeof(struct r_alloc_t), 512);

    r_renderer.free_blocks[0] = create_list(sizeof(struct r_alloc_t), 512);
    r_renderer.free_blocks[1] = create_list(sizeof(struct r_alloc_t), 512);

    r_renderer.draw_cmd_buffer = create_ringbuffer(sizeof(struct r_cmd_t), 1024);

    free_alloc.start = 0;
    free_alloc.size = R_HEAP_SIZE;
    free_alloc.align = 0;

    r_renderer.z_far = 100.0;
    r_renderer.z_near = 0.01;

    add_list_element(&r_renderer.free_blocks[0], &free_alloc);
    add_list_element(&r_renderer.free_blocks[1], &free_alloc);

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
        start = (alloc->start + align) & (~align);
        block_size = alloc->size - (start - alloc->start);

        if(block_size >= size)
        {
            align = start - alloc->start;

            if(block_size == size)
            {
                alloc->align = align;
                add_stack_list_element(&r_renderer.allocd_blocks[index_alloc], alloc);
                remove_list_element(free_blocks, i);
            }
            else
            {
                block_size = size + align;

                new_alloc.start = alloc->start;
                new_alloc.size = block_size;
                new_alloc.align = align;

                add_stack_list_element(&r_renderer.allocd_blocks[index_alloc], &new_alloc);

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

void r_Free(struct r_alloc_handle_t handle)
{

}

/*
=================================================================
=================================================================
=================================================================
*/

void r_SetProjectionMatrix(mat4_t *projection_matrix)
{
    memcpy(&r_renderer.projection_matrix, projection_matrix, sizeof(mat4_t));
    r_vk_SetProjectionMatrix(&r_renderer.projection_matrix);
    r_renderer.outdated_view_projection_matrix = 1;
}

void r_SetViewMatrix(mat4_t *view_matrix)
{
    memcpy(&r_renderer.view_matrix, view_matrix, sizeof(mat4_t));
    mat4_t_invvm(&r_renderer.view_matrix);
    r_renderer.outdated_view_projection_matrix = 1;
}

void r_SetModelMatrix(mat4_t *model_matrix)
{
    memcpy(&r_renderer.model_matrix, model_matrix, sizeof(mat4_t));
}

void r_UpdateMatrices()
{
    mat4_t model_view_projection_matrix;

    if(r_renderer.outdated_view_projection_matrix)
    {
        mat4_t_mul(&r_renderer.view_projection_matrix, &r_renderer.view_matrix, &r_renderer.projection_matrix);
        r_renderer.outdated_view_projection_matrix = 0;
    }

    mat4_t_mul(&model_view_projection_matrix, &r_renderer.model_matrix, &r_renderer.view_projection_matrix);
    r_vk_UpdateModelViewProjectionMatrix(&model_view_projection_matrix);
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

void r_QueueDrawCmd(struct r_cmd_t *draw_cmd)
{
    SDL_AtomicLock(&r_renderer.draw_cmd_buffer_lock);
    add_ringbuffer_element(&r_renderer.draw_cmd_buffer, draw_cmd);
    SDL_AtomicUnlock(&r_renderer.draw_cmd_buffer_lock);
}

struct r_cmd_t *r_NextCmd()
{
    return (struct r_cmd_t *)peek_ringbuffer_element(&r_renderer.draw_cmd_buffer);
}

void r_AdvanceCmd()
{
    SDL_AtomicLock(&r_renderer.draw_cmd_buffer_lock);
    get_ringbuffer_element(&r_renderer.draw_cmd_buffer);
    SDL_AtomicUnlock(&r_renderer.draw_cmd_buffer_lock);
}

void r_ExecuteDrawCmd(struct r_cmd_t *draw_cmd)
{
    
}