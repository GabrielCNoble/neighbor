#include "r_draw.h"
#include "spr.h"
#include "lib/dstuff/ds_file.h"
#include "lib/dstuff/ds_path.h"
#include "lib/dstuff/ds_mem.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

struct r_d_submission_state_t r_submission_state = {};
struct r_i_submission_state_t r_i_submission_state = {};

struct stack_list_t r_materials;

struct list_t r_uniform_buffers;
struct list_t r_free_uniform_buffers;
struct list_t r_used_uniform_buffers;
VkPhysicalDeviceLimits *r_limits;

struct r_render_pass_handle_t r_render_pass;
struct r_render_pass_handle_t r_i_render_pass;
struct r_framebuffer_h r_framebuffer;
struct r_buffer_h r_i_vertex_buffer;
struct r_buffer_h r_i_index_buffer;
VkQueue r_draw_queue;
VkFence r_draw_fence;

struct r_heap_h r_vertex_heap;
struct r_heap_h r_index_heap;
SDL_Window *r_window;
uint32_t r_window_width;
uint32_t r_window_height;

#define R_VERTEX_BUFFER_MEMORY 8388608
#define R_INDEX_BUFFER_MEMORY 8388608
#define R_UNIFORM_BUFFER_MEMORY 8388608
#define R_I_VERTEX_BUFFER_MEMORY 8388608
#define R_I_INDEX_BUFFER_MEMORY 8388608

struct r_view_t r_view;
VkInstance r_instance;

void r_DrawInit()
{
    r_window = SDL_CreateWindow("game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, R_DEFAULT_WIDTH, R_DEFAULT_HEIGHT, SDL_WINDOW_VULKAN);
    r_instance = r_CreateInstance();
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(r_window, r_instance, &surface);
    r_CreateDevice(surface);
    
    FILE *file;
    struct r_shader_description_t shader_description = {};
    struct r_render_pass_description_t render_pass_description = {};
    long code_size;
    
    r_view.z_near = 0.1;
    r_view.z_far = 20.0;
    r_view.zoom = 1.0;
//    r_view.position = vec2_t_c(0.0, 0.0);
    r_view.viewport.width = R_DEFAULT_WIDTH;
    r_view.viewport.height = R_DEFAULT_HEIGHT;
    r_view.viewport.x = 0.0;
    r_view.viewport.y = 0.0;
    r_view.viewport.minDepth = 0.0;
    r_view.viewport.maxDepth = 1.0;
    r_view.scissor.offset.x = 0;
    r_view.scissor.offset.y = 0;
    r_view.scissor.extent.width = (int)r_view.viewport.width;
    r_view.scissor.extent.height = (int)r_view.viewport.height;
    
    mat4_t_identity(&r_view.view_transform);
    r_RecomputeInvViewMatrix();
    r_RecomputeProjectionMatrix();

    
    shader_description = (struct r_shader_description_t){
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .vertex_binding_count = 1,
        .vertex_bindings = (struct r_vertex_binding_t []){
            {
                .size = sizeof(struct r_vertex_t),
                .attrib_count = 3,
                .attribs = (struct r_vertex_attrib_t []){
                    {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_vertex_t, position)},
                    {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_vertex_t, normal)},
                    {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_vertex_t, tex_coords)}
                },
            }
        },
        .push_constant_count = 1,
        .push_constants = &(struct r_push_constant_t ){
            .size = sizeof(mat4_t),
            .offset = 0
        }
    };
    file = fopen("./neighbor/shaders/shader.vert.spv", "rb");
    read_file(file, &shader_description.code, &shader_description.code_size);
    fclose(file);
    struct r_shader_t *vertex_shader = r_GetShaderPointer(r_CreateShader(&shader_description));
    mem_Free(shader_description.code);
    
    shader_description = (struct r_shader_description_t){
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .resource_count = 1,
        .resources = (struct r_resource_binding_t []){{.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = 1}}
    };
    fopen("neighbor/shaders/shader.frag.spv", "rb");
    read_file(file, &shader_description.code, &shader_description.code_size);
    fclose(file);
    struct r_shader_t *fragment_shader = r_GetShaderPointer(r_CreateShader(&shader_description));
    mem_Free(shader_description.code);

    render_pass_description = (struct r_render_pass_description_t){
        .attachments = (struct r_attachment_d []){
            {.format = VK_FORMAT_R8G8B8A8_SRGB},
            {.format = VK_FORMAT_D32_SFLOAT}
        },
        .attachment_count = 2,
        .subpass_count = 1,
        .subpasses = (struct r_subpass_description_t[]){
            {
                .color_attachment_count = 1,
                .color_attachments = (VkAttachmentReference []){
                    {.attachment = 0}
                },
                .depth_stencil_attachment = &(VkAttachmentReference){
                    .attachment = 1
                },
                .pipeline_description_count = 1,
                .pipeline_descriptions = (struct r_pipeline_description_t[]){
                    {
                        .shader_count = 2,
                        .shaders = (struct r_shader_t *[]){vertex_shader, fragment_shader}
                    }
                },
            }
        },
    };

    r_render_pass = r_CreateRenderPass(&render_pass_description);

    struct r_framebuffer_description_t framebuffer_description = (struct r_framebuffer_description_t){
        .frame_count = 2,
        .width = R_DEFAULT_WIDTH,
        .height = R_DEFAULT_HEIGHT,
        .render_pass = r_GetRenderPassPointer(r_render_pass)
    };

    r_framebuffer = r_CreateFramebuffer(&framebuffer_description);
    r_draw_queue = r_GetDrawQueue();
    r_draw_fence = r_CreateFence();
    r_limits = r_GetDeviceLimits();

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    buffer_create_info.size = R_I_VERTEX_BUFFER_MEMORY;
    r_i_vertex_buffer = r_CreateBuffer(&buffer_create_info);
    buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_create_info.size = R_I_INDEX_BUFFER_MEMORY;
    r_i_index_buffer = r_CreateBuffer(&buffer_create_info);
    
    r_vertex_heap = r_CreateBufferHeap(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, R_VERTEX_BUFFER_MEMORY);
    r_index_heap = r_CreateBufferHeap(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, R_INDEX_BUFFER_MEMORY);

    r_submission_state.base.draw_cmd_lists = create_stack_list(sizeof(struct r_draw_cmd_list_t), 32);
    r_submission_state.base.pending_draw_cmd_lists = create_list(sizeof(uint32_t), 32);
    r_submission_state.base.current_draw_cmd_list = 0xffffffff;
    r_submission_state.base.draw_cmd_size = sizeof(struct r_draw_cmd_t);
    r_submission_state.draw_calls = create_list(sizeof(struct r_draw_call_data_t), 1024);

    r_i_submission_state.base.draw_cmd_lists = create_stack_list(sizeof(struct r_draw_cmd_list_t), 32);
    r_i_submission_state.base.pending_draw_cmd_lists = create_list(sizeof(uint32_t), 32);
    r_i_submission_state.base.current_draw_cmd_list = 0xffffffff;
    r_i_submission_state.base.draw_cmd_size = sizeof(struct r_i_draw_cmd_t);
    r_i_submission_state.cmd_data = create_list(sizeof(struct r_i_draw_data_slot_t), 2048);
    r_i_submission_state.vertex_cursor = 0;
    r_i_submission_state.index_cursor = 0;
    r_i_submission_state.next_draw_state.pipeline_state.depth_state.write_enable = VK_TRUE;
    r_i_submission_state.next_draw_state.pipeline_state.depth_state.test_enable = VK_TRUE;
    r_i_submission_state.next_draw_state.pipeline_state.depth_state.compare_op = VK_COMPARE_OP_LESS;
    r_i_submission_state.next_draw_state.texture = R_INVALID_TEXTURE_HANDLE;
    r_i_submission_state.next_draw_state.scissor = r_view.scissor;

    r_uniform_buffers = create_list(sizeof(struct r_uniform_buffer_t), 64);
    r_free_uniform_buffers = create_list(sizeof(uint32_t), 64);
    r_used_uniform_buffers = create_list(sizeof(uint32_t), 64);
    
    shader_description = (struct r_shader_description_t){
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .vertex_binding_count = 1,
        .vertex_bindings = &(struct r_vertex_binding_t){
            .size = sizeof(struct r_i_vertex_t),
            .attrib_count = 3,
            .attribs = (struct r_vertex_attrib_t []){
                {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_i_vertex_t, position)},
                {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_i_vertex_t, tex_coords)},
                {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_i_vertex_t, color)},
            }
        },
        .push_constant_count = 1,
        .push_constants = &(struct r_push_constant_t ){.size = sizeof(mat4_t) + sizeof(float), .offset = 0},
    };
    file = fopen("neighbor/shaders/i_draw.vert.spv", "rb");
    read_file(file, &shader_description.code, &shader_description.code_size);
    fclose(file);
    vertex_shader = r_GetShaderPointer(r_CreateShader(&shader_description));

    shader_description = (struct r_shader_description_t){
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .resource_count = 1,
        .resources = &(struct r_resource_binding_t){
            .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .count = 1
        },        
        .push_constant_count = 1,
        .push_constants = &(struct r_push_constant_t){.size = sizeof(uint32_t), .offset = sizeof(mat4_t) + sizeof(float)},
    };

    file = fopen("neighbor/shaders/i_draw.frag.spv", "rb");
    read_file(file, &shader_description.code, &shader_description.code_size);
    fclose(file);
    fragment_shader = r_GetShaderPointer(r_CreateShader(&shader_description));
    
    render_pass_description = (struct r_render_pass_description_t){
        .attachment_count = 2,
        .attachments = (struct r_attachment_d []){
            {.format = VK_FORMAT_R8G8B8A8_SRGB, .initial_layout = VK_IMAGE_LAYOUT_GENERAL},
            {.format = VK_FORMAT_D32_SFLOAT, .initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}
        },
        .subpass_count = 1,
        .subpasses = &(struct r_subpass_description_t){
            .color_attachment_count = 1,
            .color_attachments = (VkAttachmentReference[]){{.attachment = 0}},
            .depth_stencil_attachment = &(VkAttachmentReference){.attachment = 1},
            .pipeline_description_count = 1,
            .pipeline_descriptions = (struct r_pipeline_description_t []){
                {
                    .shader_count = 2,
                    .shaders = (struct r_shader_t *[]){vertex_shader, fragment_shader},
                },
            }
        }
    };
 
    r_i_render_pass = r_CreateRenderPass(&render_pass_description);
    r_materials = create_stack_list(sizeof(struct r_material_t), 128);
    r_CreateDefaultMaterial();
}

void r_DrawShutdown()
{

}

void r_BeginFrame()
{

}

void r_EndFrame()
{
    r_RecomputeInvViewMatrix();
    r_RecomputeProjectionMatrix();
    r_VisibleWorld();
//    r_VisibleEntities();
    r_DispatchPending();
    r_i_DispatchPending();
    r_PresentFramebuffer(r_framebuffer);
}

void r_RecomputeInvViewMatrix()
{
    mat4_t_invvm(&r_view.inv_view_matrix, &r_view.view_transform);
}

void r_RecomputeProjectionMatrix()
{
    mat4_t_persp(&r_view.projection_matrix, 0.68, (float)r_view.viewport.width / (float)r_view.viewport.height, 0.1, 500.0);
}

struct r_view_t *r_GetViewPointer()
{
    return &r_view;
}

void r_SetWindowSize(uint32_t width, uint32_t height)
{
    VkSurfaceKHR surface;
    r_window_width = width;
    r_window_height = height;
    
    r_view.viewport.width = width;
    r_view.viewport.height = height;
    r_view.scissor.extent.width = width;
    r_view.scissor.extent.height = height;
    
    SDL_SetWindowSize(r_window, width, height);
    r_ResizeFramebuffer(r_framebuffer, width, height);
    SDL_Vulkan_CreateSurface(r_window, r_instance, &surface);
    r_SetSwapchainSurface(surface);
    r_RecomputeProjectionMatrix();
}

void r_Fullscreen(uint32_t enable)
{
    VkSurfaceKHR surface;
    int width;
    int height;
    if(enable)
    {
        SDL_SetWindowFullscreen(r_window, SDL_WINDOW_FULLSCREEN);
        width = 1360;
        height = 768;
//        SDL_SetWindowSize(r_window, 1366, 768);
    }
    else
    {
        SDL_SetWindowFullscreen(r_window, 0);
        width = 800;
        height = 600;
//        SDL_SetWindowSize(r_window, 800, 600);
    }
    r_SetWindowSize(width, height);
}

struct r_framebuffer_h r_CreateDrawableFramebuffer(uint32_t width, uint32_t height)
{
    struct r_framebuffer_h handle;
    struct r_framebuffer_description_t *backbuffer_description;
    struct r_framebuffer_description_t description;
    
    backbuffer_description = r_GetFramebufferDescriptionPointer(r_framebuffer);
    
    description = *backbuffer_description;
    description.width = width;
    description.height = height;
    
    handle = r_CreateFramebuffer(&description);
    
    return handle;
}

struct r_framebuffer_h r_GetBackbufferHandle()
{
    return r_framebuffer;
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_CreateDefaultMaterial()
{
    struct r_material_t *default_material = r_GetMaterialPointer(r_CreateMaterial());
    default_material->name = strdup("_default_material_");
}

struct r_material_h r_CreateMaterial()
{
    struct r_material_h handle;
    struct r_material_t *material;
    
    handle.index = add_stack_list_element(&r_materials, NULL);
    material = get_stack_list_element(&r_materials, handle.index);
    
    material->diffuse_texture = r_GetDefaultTextureHandle();
    material->normal_texture = R_INVALID_TEXTURE_HANDLE;
    
    return handle;
}

void r_DestroyMaterial(struct r_material_h handle)
{
    struct r_material_t *material;
    
    /* the default material can't be destroyed */
    if(handle.index != R_DEFAULT_MATERIAL_INDEX)
    {
        material = r_GetMaterialPointer(handle);
        if(material)
        {
            material->diffuse_texture = R_INVALID_TEXTURE_HANDLE;
            material->normal_texture = R_INVALID_TEXTURE_HANDLE;
            remove_stack_list_element(&r_materials, handle.index);
        }        
    }
}

struct r_material_t *r_GetMaterialPointer(struct r_material_h handle)
{
    struct r_material_t *material;
//    static struct r_material_t default_material;
    
    material = get_stack_list_element(&r_materials, handle.index);
    if(material && material->diffuse_texture.index == R_INVALID_TEXTURE_INDEX)
    {
        material = NULL;
    }
    
//    if(handle.index == R_DEFAULT_MATERIAL_INDEX)
//    {
//        /* this is to avoid having the default material mutated somehow */
//        memcpy(&default_material, material, sizeof(struct r_material_t));
//        material = &default_material;
//    }
    
    return material;
}

struct r_material_h r_GetMaterialHandle(char *name)
{
    struct r_material_t *material;
    for(uint32_t index = 0; index < r_materials.cursor; index++)
    {
        material = r_GetMaterialPointer(R_MATERIAL_HANDLE(index));
        if(material && !strcmp(material->name, name))
        {
            return R_MATERIAL_HANDLE(index);
        }
    }
    
    return R_INVALID_MATERIAL_HANDLE;
}

struct r_material_h r_GetDefaultMaterialHandle()
{
    return R_MATERIAL_HANDLE(R_DEFAULT_MATERIAL_INDEX);
}

struct r_material_t *r_GetDefaultMaterialPointer()
{
    return r_GetMaterialPointer(R_MATERIAL_HANDLE(R_DEFAULT_MATERIAL_INDEX));
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_chunk_h r_AllocVerts(uint32_t count)
{
    return r_AllocChunk(r_vertex_heap, sizeof(struct r_vertex_t) * count, sizeof(struct r_vertex_t));
}

void r_FillVertsChunk(struct r_chunk_h chunk, struct r_vertex_t *vertices, uint32_t count)
{
    r_FillBufferChunk(chunk, vertices, sizeof(struct r_vertex_t) * count, 0);
}

struct r_chunk_h r_AllocIndexes(uint32_t count)
{
    return r_AllocChunk(r_index_heap, sizeof(uint32_t) * count, sizeof(uint32_t));
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_BeginDrawCmdSubmission(struct r_submission_state_t *submission_state, struct r_begin_submission_info_t *begin_info)
{
    struct r_draw_cmd_list_t *cmd_list;

    if(submission_state->current_draw_cmd_list == 0xffffffff)
    {
        submission_state->current_draw_cmd_list = add_stack_list_element(&submission_state->draw_cmd_lists, NULL);
        cmd_list = get_stack_list_element(&submission_state->draw_cmd_lists, submission_state->current_draw_cmd_list);
        if(!cmd_list->draw_cmds.size)
        {
            cmd_list->draw_cmds = create_list(submission_state->draw_cmd_size, 1024);
        }
    }
    
    memcpy(&cmd_list->begin_info, begin_info, sizeof(struct r_begin_submission_info_t));
    mat4_t_mul(&cmd_list->view_projection_matrix, &begin_info->inv_view_matrix, &begin_info->projection_matrix);
    cmd_list->draw_cmds.cursor = 0;
}

uint32_t r_EndDrawCmdSubmission(struct r_submission_state_t *submission_state)
{
    uint32_t draw_cmd_list = submission_state->current_draw_cmd_list;
    if(submission_state->current_draw_cmd_list != 0xffffffff)
    {
        draw_cmd_list = submission_state->current_draw_cmd_list;
        add_list_element(&submission_state->pending_draw_cmd_lists, &draw_cmd_list);
        submission_state->current_draw_cmd_list = 0xffffffff;
    }
    return draw_cmd_list;
}


void r_BeginSubmission(struct r_begin_submission_info_t *begin_info)
{
    r_BeginDrawCmdSubmission((struct r_submission_state_t *)&r_submission_state, begin_info);
}

int32_t r_CmpDrawCmds(void *a, void *b)
{
    struct r_draw_cmd_t *cmd_a = (struct r_draw_cmd_t *)a;
    struct r_draw_cmd_t *cmd_b = (struct r_draw_cmd_t *)b;
    ptrdiff_t diff = cmd_a->material - cmd_b->material;
    return (int32_t)diff;
}

void r_EndSubmission()
{
    uint32_t draw_cmd_list_index;
    struct r_draw_cmd_list_t *cmd_list;

    draw_cmd_list_index = r_EndDrawCmdSubmission((struct r_submission_state_t *)&r_submission_state);
    cmd_list = get_stack_list_element(&r_submission_state.base.draw_cmd_lists, draw_cmd_list_index);
    qsort_list(&cmd_list->draw_cmds, r_CmpDrawCmds);
}

void r_Draw(uint32_t start, uint32_t count, struct r_material_t *material, mat4_t *transform)
{
    struct r_draw_cmd_list_t *cmd_list;
    struct r_draw_cmd_t *draw_cmd;
    
    if(count)
    {
        cmd_list = get_stack_list_element(&r_submission_state.base.draw_cmd_lists, r_submission_state.base.current_draw_cmd_list);
        draw_cmd = get_list_element(&cmd_list->draw_cmds, add_list_element(&cmd_list->draw_cmds, NULL));
        
        draw_cmd->material = material;
        draw_cmd->start = start;
        draw_cmd->count = count;
        memcpy(&draw_cmd->transform, transform, sizeof(mat4_t));
    }
}

void r_DispatchPending()
{
    struct r_render_pass_t *render_pass;
    struct r_framebuffer_t *framebuffer;
    struct r_framebuffer_t *default_framebuffer;
    struct r_framebuffer_description_t *default_framebuffer_description;
    struct r_buffer_heap_t *heap;
    VkWriteDescriptorSet descriptor_set_write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
//    VkClearAttachment clear_attachments[2];
//    VkClearRect clear_rects[2];
    VkViewport viewport;

    struct r_submit_info_t submit_info = {};
    submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.command_buffer_count = r_submission_state.base.pending_draw_cmd_lists.cursor;
    submit_info.command_buffers = alloca(sizeof(union r_command_buffer_h) * submit_info.command_buffer_count);
    submit_info.command_buffer_count = 0;
    
    heap = r_GetHeapPointer(r_vertex_heap);
    render_pass = r_GetRenderPassPointer(r_render_pass);
    default_framebuffer = r_GetFramebufferPointer(r_framebuffer);
    default_framebuffer_description = r_GetFramebufferDescriptionPointer(r_framebuffer);
    
    VkClearAttachment clear_attachments[] = {
        [0] = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .clearValue = (VkClearValue){
                .color.float32[0] = 0.1,
                .color.float32[1] = 0.1,
                .color.float32[2] = 0.3,
                .color.float32[3] = 1.0,
            },
            .colorAttachment = 0
        },
        [1] = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .clearValue = (VkClearValue){
                .depthStencil.depth = 1.0
            },
        }
    };
    
    VkClearRect clear_rects[] = {
        [0] = {
            .rect = (VkRect2D){
                .offset.x = 0,
                .offset.y = 0,
            },
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        [1] = {
            .rect = (VkRect2D){
                .offset.x = 0,
                .offset.y = 0,
            },
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    
    struct r_render_pass_begin_info_t render_pass_begin_info = {};
    render_pass_begin_info.render_pass = r_render_pass;
    
    uint32_t cmd_list_index;

    for(uint32_t pending_index = 0; pending_index < r_submission_state.base.pending_draw_cmd_lists.cursor; pending_index++)
    {
        cmd_list_index = *(uint32_t *)get_list_element(&r_submission_state.base.pending_draw_cmd_lists, pending_index);
        struct r_draw_cmd_list_t *cmd_list = get_stack_list_element(&r_submission_state.base.draw_cmd_lists, cmd_list_index);
        union r_command_buffer_h command_buffer = r_AllocateCommandBuffer();
        struct r_pipeline_t *pipeline = r_GetPipelinePointer(render_pass->pipelines[0]);
        
        r_vkBeginCommandBuffer(command_buffer);
        r_vkCmdBindVertexBuffers(command_buffer, 0, 1, &heap->buffer, &(VkDeviceSize){0});
        r_vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
        
        r_vkCmdSetViewport(command_buffer, 0, 1, &cmd_list->begin_info.viewport);
        r_vkCmdSetScissor(command_buffer, 0, 1, &cmd_list->begin_info.scissor);
        
        if(cmd_list->begin_info.framebuffer.index == R_INVALID_FRAMEBUFFER_INDEX)
        {
            framebuffer = default_framebuffer;
        }
        else
        {
            framebuffer = r_GetFramebufferPointer(cmd_list->begin_info.framebuffer);
        }
        
        render_pass_begin_info.framebuffer = cmd_list->begin_info.framebuffer;
        render_pass_begin_info.render_area = cmd_list->begin_info.scissor;
    
        r_vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        
        if(cmd_list->begin_info.clear_framebuffer)
        {
            clear_rects[0].rect.extent.width = cmd_list->begin_info.viewport.width;
            clear_rects[0].rect.extent.height = cmd_list->begin_info.viewport.height;
            
            clear_rects[1].rect.extent.width = cmd_list->begin_info.viewport.width;
            clear_rects[1].rect.extent.height = cmd_list->begin_info.viewport.height;
            
            r_vkCmdClearAttachments(command_buffer, 2, clear_attachments, 2, clear_rects);
        }
        
        struct r_material_t *current_material = NULL;
                
        for(uint32_t draw_cmd_index = 0; draw_cmd_index < cmd_list->draw_cmds.cursor; draw_cmd_index++)
        {
            struct r_draw_cmd_t *draw_cmd = get_list_element(&cmd_list->draw_cmds, draw_cmd_index);
            
            if(draw_cmd->material != current_material)
            {
                VkDescriptorSet descriptor_set = r_AllocateDescriptorSet(command_buffer, pipeline, VK_SHADER_STAGE_FRAGMENT_BIT);
                struct r_texture_t *texture = r_GetTexturePointer(draw_cmd->material->diffuse_texture);
                
                VkDescriptorImageInfo image_info = {};
                image_info.sampler = texture->sampler;
                image_info.imageView = texture->image_view;
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                
                descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor_set_write.dstSet = descriptor_set;
                descriptor_set_write.dstBinding = 0;
                descriptor_set_write.dstArrayElement = 0;
                descriptor_set_write.pImageInfo = &image_info;
                descriptor_set_write.descriptorCount = 1;
                
                r_vkUpdateDescriptorSets(1, &descriptor_set_write);
                r_vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &descriptor_set, 0, NULL);
                current_material = draw_cmd->material;
            }
            mat4_t model_view_projection_matrix;
            mat4_t_mul(&model_view_projection_matrix, &draw_cmd->transform, &cmd_list->view_projection_matrix);
            r_vkCmdPushConstants(command_buffer, pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), &model_view_projection_matrix);
            r_vkCmdDraw(command_buffer, draw_cmd->count, 1, draw_cmd->start, 0);
        }

        r_vkCmdEndRenderPass(command_buffer);
        r_vkEndCommandBuffer(command_buffer);

        submit_info.command_buffers[submit_info.command_buffer_count] = command_buffer;
        submit_info.command_buffer_count++;
        remove_stack_list_element(&r_submission_state.base.draw_cmd_lists, cmd_list_index);
    }
    r_submission_state.base.pending_draw_cmd_lists.cursor = 0;
    r_vkResetFences(1, &r_draw_fence);
    r_vkQueueSubmit(r_draw_queue, 1, &submit_info, r_draw_fence);
    r_vkWaitForFences(1, &r_draw_fence, 1, 0xffffffffffffffff);

    r_submission_state.base.pending_draw_cmd_lists.cursor = 0;
}

struct r_uniform_buffer_t *r_AllocateUniformBuffer(union r_command_buffer_h command_buffer)
{
    struct r_uniform_buffer_t *uniform_buffer = NULL;
    struct r_buffer_t *buffer;
    VkBufferCreateInfo buffer_create_info = {};
    uint32_t *buffer_index;
    uint32_t new_buffer_index;
    VkResult result;
    uint32_t recycled_buffers = 0;

    for(uint32_t index = 0; index < r_used_uniform_buffers.cursor; index++)
    {
        buffer_index = get_list_element(&r_used_uniform_buffers, index);
        uniform_buffer = get_list_element(&r_uniform_buffers, *buffer_index);
        result = r_vkGetEventStatus(uniform_buffer->event);

        if(result == VK_EVENT_SET)
        {
            r_vkResetEvent(uniform_buffer->event);
            remove_list_element(&r_used_uniform_buffers, index);
            add_list_element(&r_free_uniform_buffers, buffer_index);
            recycled_buffers++;
        }
    }

    buffer_index = get_list_element(&r_free_uniform_buffers, 0);

    if(!buffer_index)
    {
        new_buffer_index = add_list_element(&r_uniform_buffers, NULL);
        uniform_buffer = get_list_element(&r_uniform_buffers, new_buffer_index);
        buffer_create_info.size = r_limits->maxUniformBufferRange;
        buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uniform_buffer->buffer = r_CreateBuffer(&buffer_create_info);
        uniform_buffer->event = r_CreateEvent();
        buffer = r_GetBufferPointer(uniform_buffer->buffer);
        uniform_buffer->memory = r_GetBufferChunkMappedMemory(buffer->memory);
        uniform_buffer->vk_buffer = buffer->buffer;
        add_list_element(&r_used_uniform_buffers, &new_buffer_index);
    }
    else
    {
        buffer = get_list_element(&r_uniform_buffers, *buffer_index);
        add_list_element(&r_used_uniform_buffers, buffer_index);
        remove_list_element(&r_free_uniform_buffers, 0);
    }

    r_AppendEvent(command_buffer, uniform_buffer->event);

    return uniform_buffer;
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_i_BeginSubmission(struct r_begin_submission_info_t *begin_info)
{
    r_BeginDrawCmdSubmission((struct r_submission_state_t *)&r_i_submission_state, begin_info);
    memset(&r_i_submission_state.current_draw_state, 0, sizeof(struct r_i_draw_state_t));
}

void r_i_EndSubmission()
{
    r_EndDrawCmdSubmission((struct r_submission_state_t *)&r_i_submission_state);
}

void r_i_DispatchPending()
{
    union r_command_buffer_h command_buffer;
    struct r_framebuffer_t *framebuffer;
    struct r_render_pass_t *render_pass;
    struct r_buffer_t *vertex_buffer;
    struct r_buffer_t *index_buffer;

    uint32_t vertex_start;
    uint32_t index_start;
    uint32_t count;
    uint32_t indexed;
    VkRect2D scissor;
    VkViewport viewport;

    struct r_render_pass_begin_info_t render_pass_begin_info = {};
    struct r_submit_info_t submit_info = {};
    
    framebuffer = r_GetFramebufferPointer(r_framebuffer);
    vertex_buffer = r_GetBufferPointer(r_i_vertex_buffer);
    index_buffer = r_GetBufferPointer(r_i_index_buffer);
//    render_pass = r_GetRenderPassPointer(r_i_render_pass);
    
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    render_pass_begin_info.render_pass = r_i_render_pass;
//    render_pass_begin_info.framebuffer = framebuffer->buffers[0];
//    render_pass_begin_info.renderArea = r_view.scissor;
    

    command_buffer = r_AllocateCommandBuffer();
    r_vkBeginCommandBuffer(command_buffer);
    r_vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer->buffer, (VkDeviceSize[]){0});
    r_vkCmdBindIndexBuffer(command_buffer, index_buffer->buffer, (VkDeviceSize){0}, VK_INDEX_TYPE_UINT32);
//    r_vkCmdSetViewport(command_buffer, 0, 1, &r_view.viewport);
//    r_vkCmdSetScissor(command_buffer, 0, 1, &r_view.scissor);
//    r_vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    for(uint32_t pending_index = 0; pending_index < r_i_submission_state.base.pending_draw_cmd_lists.cursor; pending_index++)
    {
        uint32_t *cmd_list_index = get_list_element(&r_i_submission_state.base.pending_draw_cmd_lists, pending_index);
        struct r_draw_cmd_list_t *cmd_list = get_stack_list_element(&r_i_submission_state.base.draw_cmd_lists, *cmd_list_index);
        struct r_pipeline_t *current_pipeline = NULL;
        struct r_texture_h current_texture = R_INVALID_TEXTURE_HANDLE;
        
        if(cmd_list->begin_info.framebuffer.index == R_INVALID_FRAMEBUFFER_INDEX)
        {
            framebuffer = r_GetFramebufferPointer(r_framebuffer);
        }
        else
        {
            framebuffer = r_GetFramebufferPointer(cmd_list->begin_info.framebuffer);
        }
        
        render_pass_begin_info.framebuffer = cmd_list->begin_info.framebuffer;
        render_pass_begin_info.render_area = cmd_list->begin_info.scissor;
        
        r_vkCmdSetViewport(command_buffer, 0, 1, &cmd_list->begin_info.viewport);
        r_vkCmdSetScissor(command_buffer, 0, 1, &cmd_list->begin_info.scissor);
        r_vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        
        for(uint32_t draw_cmd_index = 0; draw_cmd_index < cmd_list->draw_cmds.cursor; draw_cmd_index++)
        {
            struct r_i_draw_cmd_t *draw_cmd = get_list_element(&cmd_list->draw_cmds, draw_cmd_index);
            switch(draw_cmd->type)
            {
                case R_I_DRAW_CMD_DRAW:
                {
                    struct r_i_draw_cmd_data_t *data = (struct r_i_draw_cmd_data_t *)draw_cmd->data;
                    vertex_start = data->vertex_start;
                    index_start = data->index_start;
                    count = data->count;
                    indexed = data->indexed;
                }
                break;
                
                case R_I_DRAW_CMD_UPLOAD_DATA:
                {
                    struct r_i_upload_buffer_data_t *data = (struct r_i_upload_buffer_data_t *)draw_cmd->data;
                    uint32_t offset = data->start * data->size;
                    uint32_t size = data->count * data->size;
                    
                    if(data->index_data)
                    {
                        r_vkCmdUpdateBuffer(command_buffer, r_i_index_buffer, offset, size, data->data);
                    }
                    else
                    {
                        r_vkCmdUpdateBuffer(command_buffer, r_i_vertex_buffer, offset, size, data->data);
                    }
                    continue;
                }
                break;

                case R_I_DRAW_CMD_SET_DRAW_STATE:
                {
                    struct r_i_set_draw_state_data_t *draw_state = (struct r_i_set_draw_state_data_t*)draw_cmd->data;
                    struct r_pipeline_t *next_pipeline;
                    uint32_t udpate_texture_state = 0;
                    
                    r_vkCmdSetScissor(command_buffer, 0, 1, &draw_state->draw_state.scissor);
                    r_vkCmdSetLineWidth(command_buffer, draw_state->draw_state.line_width); 

                    next_pipeline = r_GetPipelinePointerByState(r_i_render_pass, 0, &draw_state->draw_state.pipeline_state);
                    
                    if(next_pipeline != current_pipeline)
                    {
                        current_pipeline = next_pipeline;
                        r_vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline->pipeline);
                        r_vkCmdPushConstants(command_buffer, current_pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), &cmd_list->view_projection_matrix);
                        udpate_texture_state = 1;
                    }
                    
                    r_vkCmdPushConstants(command_buffer, current_pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(mat4_t), sizeof(float), &draw_state->draw_state.point_size);
                    
                    udpate_texture_state |= draw_state->draw_state.texture.index != current_texture.index;
                    
                    if(udpate_texture_state)
                    {
                        current_texture = draw_state->draw_state.texture;
                        uint32_t texturing = draw_state->draw_state.texture.index != R_INVALID_TEXTURE_INDEX;
                        struct r_texture_t *texture = r_GetTexturePointer(draw_state->draw_state.texture);
                        
                        r_vkCmdPushConstants(command_buffer, current_pipeline->layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(mat4_t) + sizeof(float), sizeof(uint32_t), &texturing);
                        /* even if no texturing is being used a descriptor set has to be bound... */
                        VkDescriptorSet descriptor_set = r_AllocateDescriptorSet(command_buffer, current_pipeline, VK_SHADER_STAGE_FRAGMENT_BIT);
                         
                        if(!texture)
                        {
                            texture = r_GetDefaultTexturePointer();
                        }

                        struct r_image_t *image = r_GetImagePointer(texture->image);
                        
                        VkDescriptorImageInfo image_info;
                        VkWriteDescriptorSet descriptor_write = {};
                        
                        image_info.imageView = texture->image_view;
                        image_info.imageLayout = image->current_layout;
                        image_info.sampler = texture->sampler;
                        
                        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        descriptor_write.dstSet = descriptor_set;
                        descriptor_write.dstBinding = 0;
                        descriptor_write.dstArrayElement = 0;
                        descriptor_write.descriptorCount = 1;
                        descriptor_write.pImageInfo = &image_info;
//                        r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                        r_vkUpdateDescriptorSets(1, &descriptor_write);                        
                        r_vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline->layout, 0, 1, &descriptor_set, 0, NULL);
                    }
                    
                    continue;
                }
                break;
            }
            
            if(indexed)
            {
                r_vkCmdDrawIndexed(command_buffer, count, 1, index_start, vertex_start, 0);
            }
            else
            {
                r_vkCmdDraw(command_buffer, count, 1, vertex_start, 0);
            }
        }
        r_vkCmdEndRenderPass(command_buffer);
        remove_stack_list_element(&r_i_submission_state.base.draw_cmd_lists, *cmd_list_index);
    }

    r_vkEndCommandBuffer(command_buffer);

    submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.command_buffer_count = 1;
    submit_info.command_buffers = &command_buffer;
    r_vkResetFences(1, &r_draw_fence);
    r_vkQueueSubmit(r_draw_queue, 1, &submit_info, r_draw_fence);
    r_vkWaitForFences(1, &r_draw_fence, VK_TRUE, 0xffffffffffffffff);

    r_i_submission_state.base.pending_draw_cmd_lists.cursor = 0;
    r_i_submission_state.cmd_data.cursor = 0;
    r_i_submission_state.vertex_cursor = 0;
    r_i_submission_state.index_cursor = 0;
}

void r_i_SetDrawState(struct r_i_draw_state_t *draw_state)
{
    struct r_i_set_draw_state_data_t *draw_state_data;
    draw_state_data = r_i_AllocateDrawCmdData(sizeof(struct r_i_set_draw_state_data_t));
    draw_state_data->draw_state = *draw_state;
    r_i_SubmitCmd(R_I_DRAW_CMD_SET_DRAW_STATE, draw_state_data);
    r_i_submission_state.current_draw_state = *draw_state;
}

void r_i_SetDepthWrite(uint32_t enable)
{
    r_i_submission_state.next_draw_state.pipeline_state.depth_state.write_enable = enable && 1;
}

void r_i_SetDepthTest(uint32_t enable)
{
    r_i_submission_state.next_draw_state.pipeline_state.depth_state.test_enable = enable && 1;
}

void r_i_SetDepthCompareOp(VkCompareOp op)
{
    r_i_submission_state.next_draw_state.pipeline_state.depth_state.compare_op = op;
}

void r_i_SetPrimitiveTopology(VkPrimitiveTopology topology)
{
    r_i_submission_state.next_draw_state.pipeline_state.input_assembly_state.topology = topology;
}

void r_i_SetPolygonMode(VkPolygonMode polygon_mode)
{
    r_i_submission_state.next_draw_state.pipeline_state.rasterizer_state.polygon_mode = polygon_mode;
}

void r_i_SetCullMode(VkCullModeFlags cull_mode)
{
    r_i_submission_state.next_draw_state.pipeline_state.rasterizer_state.cull_mode = cull_mode;
}

void r_i_SetTexture(struct r_texture_h texture)
{
    r_i_submission_state.next_draw_state.texture = texture;
}

void r_i_SetScissor(uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height)
{
    r_i_submission_state.next_draw_state.scissor.extent.width = width;
    r_i_submission_state.next_draw_state.scissor.extent.height = height;
    r_i_submission_state.next_draw_state.scissor.offset.x = offset_x;
    r_i_submission_state.next_draw_state.scissor.offset.y = offset_y;
}

void r_i_SetLineWidth(float width)
{
    r_i_submission_state.next_draw_state.line_width = width;
}

void r_i_SetPointSize(float size)
{
    r_i_submission_state.next_draw_state.point_size = size;
}



void *r_i_AllocateDrawCmdData(uint32_t size)
{
    uint32_t data_slots;
    uint32_t available_slots;
    uint32_t data_index;
    if(size > R_I_DRAW_DATA_SLOT_SIZE)
    {
        /* the data passed in is bigger than R_D_DRAW_DATA_SIZE, so we'll
        need to accomodate it. */


        /* how many slots in the list this data takes */
        data_slots = 1 + size / R_I_DRAW_DATA_SLOT_SIZE;
        /* how many slots the current buffer of the list has available */
        available_slots = (r_i_submission_state.cmd_data.buffer_size - r_i_submission_state.cmd_data.cursor % r_i_submission_state.cmd_data.buffer_size);
        if(data_slots > available_slots)
        {
            while(available_slots)
            {
                /* the current buffer in the list doesn't have enough space to hold this data
                contiguously, so we'll "pad" it until it gets to the start of the next buffer,
                in which case it'll probably be enough. */
                add_list_element(&r_i_submission_state.cmd_data, NULL);
                available_slots--;
            }
        }

        /* capture the index of the first slot. This is where the allocation for this draw
        cmd begins */
        data_index = add_list_element(&r_i_submission_state.cmd_data, NULL);
        data_slots--;
        while(data_slots)
        {
            /* data_size is bigger than the slot size, so we'll need to allocate some more
            slots to accommodate it */
            add_list_element(&r_i_submission_state.cmd_data, NULL);
            data_slots--;
        }

    }
    else
    {
        data_index = add_list_element(&r_i_submission_state.cmd_data, NULL);
    }

    return get_list_element(&r_i_submission_state.cmd_data, data_index);
}

void r_i_SubmitCmd(uint32_t type, void *data)
{
    struct r_i_draw_cmd_t *draw_cmd;
    struct r_draw_cmd_list_t *cmd_list;
//    uint32_t next_pipeline = R_I_PIPELINE_LAST;
    
    if(type < R_I_DRAW_CMD_DRAW_LAST)
    {
        if(memcmp(&r_i_submission_state.current_draw_state, &r_i_submission_state.next_draw_state, sizeof(struct r_i_draw_state_t)))
        {
            r_i_SetDrawState(&r_i_submission_state.next_draw_state);
        }
    }

    cmd_list = get_stack_list_element(&r_i_submission_state.base.draw_cmd_lists, r_i_submission_state.base.current_draw_cmd_list);
    draw_cmd = get_list_element(&cmd_list->draw_cmds, add_list_element(&cmd_list->draw_cmds, NULL));
    draw_cmd->type = type;
    draw_cmd->data = data;
}

uint32_t r_i_UploadVertices(struct r_i_vertex_t *vertices, uint32_t count)
{
    return r_i_UploadBufferData(vertices, sizeof(struct r_i_vertex_t), count, 0);
}

uint32_t r_i_UploadIndices(uint32_t *indices, uint32_t count)
{
    return r_i_UploadBufferData(indices, sizeof(uint32_t), count, 1);
}

uint32_t r_i_UploadBufferData(void *data, uint32_t size, uint32_t count, uint32_t index_data)
{
    struct r_i_upload_buffer_data_t *upload_data;
    uint32_t data_size;
    
    if(!count || !size || !data)
    {
        return;
    }
    
    upload_data = r_i_AllocateDrawCmdData(sizeof(struct r_i_upload_buffer_data_t) + size * count);
    upload_data->size = size;
    upload_data->count = count;
    upload_data->index_data = index_data;
    
    upload_data->data = upload_data + 1;
    memcpy(upload_data->data, data, size * count);
    
    if(index_data)
    {
        upload_data->start = r_i_submission_state.index_cursor;
        r_i_submission_state.index_cursor += count;
    }
    else
    {
        upload_data->start = r_i_submission_state.vertex_cursor;
        r_i_submission_state.vertex_cursor += count;
    }
    
    r_i_SubmitCmd(R_I_DRAW_CMD_UPLOAD_DATA, upload_data);
    return upload_data->start;
}

void r_i_Draw(uint32_t vertex_start, uint32_t index_start, uint32_t count, uint32_t indexed)
{
    struct r_i_draw_cmd_data_t *data;
    
    data = r_i_AllocateDrawCmdData(sizeof(struct r_i_draw_cmd_data_t));
    data->vertex_start = vertex_start;
    data->index_start = index_start;
    data->count = count;
    data->indexed = indexed;
    
    r_i_SubmitCmd(R_I_DRAW_CMD_DRAW, data);
}

void r_i_DrawImmediate(struct r_i_vertex_t *vertices, uint32_t vertex_count, uint32_t *indices, uint32_t index_count)
{
    uint32_t vertex_start;
    uint32_t index_start;
    uint32_t count = vertex_count;
    
    vertex_start = r_i_UploadVertices(vertices, vertex_count);
    if(indices)
    {
        index_start = r_i_UploadIndices(indices, index_count);
        count = index_count;
    }
    
    r_i_Draw(vertex_start, index_start, count, indices);
}

void r_i_DrawLines(uint32_t vertex_start, uint32_t index_start, uint32_t count, uint32_t indexed, uint32_t line_strip)
{
//    r_i_SetPolygonMode(VK_POLYGON_MODE_LINE);
//    if(line_strip)
//    {
//        r_i_SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
//    }
//    else
//    {
//        r_i_SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
//    }
//    r_i_Draw(0, vertex_start, index_start, count, indexed);
}

void r_i_DrawLinesImmediate(struct r_i_vertex_t *verts, uint32_t vert_count, float size, uint32_t line_strip)
{
//    uint32_t first_vertex;
//    
//    if(!vert_count)
//    {
//        return;
//    }
//    
//    first_vertex = r_i_UploadVertices(verts, vert_count, 1);
//    
//    r_i_DrawLines(first_vertex, 0, vert_count, 0, line_strip);
}

void r_i_DrawLine(struct r_i_vertex_t *from, struct r_i_vertex_t *to, float size)
{
//    r_i_DrawLines((struct r_i_vertex_t []){*from, *to}, 2, size, 0);
}

void r_i_DrawLineStrip(struct r_i_vertex_t *verts, uint32_t vert_count, float size)
{
//    r_i_DrawLines(verts, vert_count, size, 1);
}

void r_i_DrawTris(uint32_t vertex_start, uint32_t index_start, uint32_t count, uint32_t indexed, uint32_t fill)
{    
    if(!count)
    {
        return;
    }
    
    r_i_SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    
    if(fill)
    {
        r_i_SetPolygonMode(VK_POLYGON_MODE_FILL);
    }
    else
    {
        r_i_SetPolygonMode(VK_POLYGON_MODE_LINE);
    }
    r_i_Draw(vertex_start, index_start, count, indexed);
}

void r_i_DrawTrisImmediate(struct r_i_vertex_t *verts, uint32_t vert_count, uint32_t *indices, uint32_t indice_count, uint32_t fill)
{
    uint32_t first_vertex;
    uint32_t first_index;
    uint32_t indexed;
    uint32_t count;
    
    if(!vert_count && !indice_count)
    {
        return;
    }
    
    first_vertex = r_i_UploadVertices(verts, vert_count);
      
    if(indices)
    {
        indexed = 1;
        first_index = r_i_UploadIndices(indices, indice_count);
        count = indice_count;
    }
    else
    {
        indexed = 0;
        count = vert_count;
    }
    
    r_i_DrawTris(first_vertex, first_index, count, indexed, fill);
}

void r_i_DrawTriLine(struct r_i_vertex_t *verts)
{
//    r_i_DrawTrisImmediate(verts, 3, NULL, 0, 0);
}

void r_i_DrawTriFill(struct r_i_vertex_t *verts)
{
//    r_i_DrawTrisImmediate(verts, 3, NULL, 0, 1);
}

void r_i_DrawPoints(uint32_t first_vertex, uint32_t count, float size)
{
//    r_i_SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    
}

void r_i_DrawPoint(vec3_t *point, vec3_t *color, float size)
{
    
}




