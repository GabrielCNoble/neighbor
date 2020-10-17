#include "r_draw.h"
//#include "spr.h"
#include "lib/SDL/include/SDL.h"
#include "lib/dstuff/ds_file.h"
#include "lib/dstuff/ds_path.h"
#include "lib/dstuff/ds_mem.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

/*
    NOTE: so, the consensus so far is to group normal draw commands with immediate
    draw commands, and have two lists of indices to pending draw command lists, one
    for draw commands submitted externally, not visible to the renderer, and the other,
    visible to the renderer, which is filled by the renderer after visibility computation.
    For the immediate draw command data, there's going to be a heap, that's going to be
    reset at the end of every frame.
    
    TODO:
        -   decide what to do if there's multiple draw command lists that refer to the same
        view and framebuffer. Should them be merged? Should them go to hell?
*/

struct r_submission_state_t r_submission_state = {};
//struct r_i_submission_state_t r_i_submission_state = {};


struct list_t r_uniform_buffers;
struct list_t r_free_uniform_buffers;
struct list_t r_used_uniform_buffers;
VkPhysicalDeviceLimits *r_limits;

struct r_render_pass_handle_t r_render_pass;
struct r_render_pass_handle_t r_i_render_pass;
struct r_framebuffer_h r_framebuffer;
//struct r_buffer_h r_i_vertex_buffer;
//struct r_buffer_h r_i_index_buffer;
struct r_queue_h r_draw_queue;
//VkFence r_draw_fence;

//struct r_heap_h r_vertex_heap;
//struct r_heap_h r_index_heap;
struct r_chunk_h r_i_vertex_chunk;
struct r_chunk_h r_i_index_chunk;
SDL_Window *r_window;
uint32_t r_window_width;
uint32_t r_window_height;

#define R_VERTEX_BUFFER_MEMORY 67108864
#define R_INDEX_BUFFER_MEMORY 67108864
#define R_UNIFORM_BUFFER_MEMORY 8388608
#define R_I_MAX_VERTEX_COUNT 100000
#define R_I_MAX_INDEX_COUNT 100000
//#define R_I_VERTEX_BUFFER_MEMORY 8388608
//#define R_I_INDEX_BUFFER_MEMORY 8388608

struct r_view_t r_view;
VkInstance r_instance;

void r_DrawInit()
{
    r_window_width = R_DEFAULT_WIDTH;
    r_window_height = R_DEFAULT_HEIGHT;
    r_window = SDL_CreateWindow("game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, R_DEFAULT_WIDTH, R_DEFAULT_HEIGHT, SDL_WINDOW_VULKAN);
    r_instance = r_CreateInstance();
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(r_window, r_instance, &surface);
    r_CreateDevice(surface);
    
    r_CreateDefaultTexture();
    r_CreateDefaultMaterial();
    
    FILE *file;
    struct r_shader_description_t shader_description = {};
    struct r_render_pass_description_t render_pass_description = {};
    long code_size;
    
    r_view.z_near = 0.1;
    r_view.z_far = 200.0;
    r_view.fov_y = 0.68;
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
    r_RecomputeInvViewMatrix(&r_view);
    r_RecomputeProjectionMatrix(&r_view);

    
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
    file = fopen("./neighbor/shaders/d_lit.vert.spv", "rb");
    read_file(file, &shader_description.code, &shader_description.code_size);
    fclose(file);
    struct r_shader_t *vertex_shader = r_GetShaderPointer(r_CreateShader(&shader_description));
    mem_Free(shader_description.code);
    
    shader_description = (struct r_shader_description_t){
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .resource_count = 1,
        .resources = (struct r_resource_binding_t []){{.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = 1}}
    };
    fopen("neighbor/shaders/d_lit.frag.spv", "rb");
    read_file(file, &shader_description.code, &shader_description.code_size);
    fclose(file);
    struct r_shader_t *fragment_shader = r_GetShaderPointer(r_CreateShader(&shader_description));
    mem_Free(shader_description.code);

    render_pass_description = (struct r_render_pass_description_t){
        .attachments = (struct r_attachment_d []){
            {.format = VK_FORMAT_R8G8B8A8_UNORM},
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
//    r_draw_queue = r_GetDrawQueue();
//    r_draw_fence = r_CreateFence();
    r_limits = r_GetDeviceLimits();

//    VkBufferCreateInfo buffer_create_info = {};
//    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
//    buffer_create_info.size = R_I_VERTEX_BUFFER_MEMORY;
//    r_i_vertex_buffer = r_CreateBuffer(&buffer_create_info);
//    buffer_create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
//    buffer_create_info.size = R_I_INDEX_BUFFER_MEMORY;
//    r_i_index_buffer = r_CreateBuffer(&buffer_create_info);
    
//    r_vertex_heap = r_CreateBufferHeap(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, R_VERTEX_BUFFER_MEMORY);
//    r_index_heap = r_CreateBufferHeap(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, R_INDEX_BUFFER_MEMORY);
    
    r_i_vertex_chunk = r_AllocVertexChunk(sizeof(struct r_i_vertex_t), R_I_MAX_VERTEX_COUNT, sizeof(struct r_i_vertex_t));
    r_i_index_chunk = r_AllocIndexChunk(sizeof(uint32_t), R_I_MAX_INDEX_COUNT, sizeof(uint32_t));
    
    r_submission_state.draw_cmd_lists = create_stack_list(sizeof(struct r_draw_cmd_list_t), 512);
    r_submission_state.immediate_draw_data_heap = ds_create_heap(33554432);
    r_submission_state.immediate_draw_data_buffer = mem_Calloc(1, 33554432);
    r_submission_state.ready_draw_cmd_lists = create_ringbuffer(sizeof(uint32_t), 512);
    r_submission_state.index_cursor = 0;
    r_submission_state.vertex_cursor = 0;
    
//    r_submission_state.base.draw_cmd_lists = create_stack_list(sizeof(struct r_draw_cmd_list_t), 32);
//    r_submission_state.base.pending_draw_cmd_lists = create_list(sizeof(uint32_t), 32);
//    r_submission_state.base.current_draw_cmd_list = 0xffffffff;
//    r_submission_state.base.draw_cmd_size = sizeof(struct r_draw_cmd_t);
//    r_submission_state.draw_calls = create_list(sizeof(struct r_draw_call_data_t), 1024);

//    r_i_submission_state.base.draw_cmd_lists = create_stack_list(sizeof(struct r_draw_cmd_list_t), 32);
//    r_i_submission_state.base.pending_draw_cmd_lists = create_list(sizeof(uint32_t), 32);
//    r_i_submission_state.base.current_draw_cmd_list = 0xffffffff;
//    r_i_submission_state.base.draw_cmd_size = sizeof(struct r_i_draw_cmd_t);
//    r_i_submission_state.cmd_data = create_list(sizeof(struct r_i_draw_data_slot_t), 8192);
//    r_i_submission_state.vertex_cursor = 0;
//    r_i_submission_state.index_cursor = 0;
//    r_i_submission_state.next_draw_state.pipeline_state.depth_state.write_enable = VK_TRUE;
//    r_i_submission_state.next_draw_state.pipeline_state.depth_state.test_enable = VK_TRUE;
//    r_i_submission_state.next_draw_state.pipeline_state.depth_state.compare_op = VK_COMPARE_OP_LESS;
//    r_i_submission_state.next_draw_state.texture = R_INVALID_TEXTURE_HANDLE;
//    r_i_submission_state.next_draw_state.scissor = r_view.scissor;

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
            {.format = VK_FORMAT_R8G8B8A8_UNORM, .initial_layout = VK_IMAGE_LAYOUT_GENERAL, .final_layout = VK_IMAGE_LAYOUT_GENERAL},
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
}

void r_DrawShutdown()
{

}

void r_BeginFrame()
{
//    r_i_submission_state.base.pending_draw_cmd_lists.cursor = 0;
//    r_i_submission_state.base.draw_cmd_lists.cursor = 0;
//    r_i_submission_state.cmd_data.cursor = 0;
//    r_i_submission_state.vertex_cursor = 0;
//    r_i_submission_state.index_cursor = 0;
    
//    r_submission_state.draw_calls.cursor = 0;
//    r_submission_state.base.draw_cmd_lists.cursor = 0;
//    r_submission_state.base.pending_draw_cmd_lists.cursor = 0;

    r_submission_state.draw_cmd_lists.cursor = 0;
    r_submission_state.index_cursor = 0;
    r_submission_state.vertex_cursor = 0;
    reset_ringbuffer(&r_submission_state.ready_draw_cmd_lists);
    ds_reset_heap(&r_submission_state.immediate_draw_data_heap);
}

void r_EndFrame()
{
    r_RecomputeInvViewMatrix(&r_view);
    r_RecomputeProjectionMatrix(&r_view);
    r_DispatchPending();
    r_PresentFramebuffer(r_framebuffer);
}

void r_GetWindowSize(vec2_t *size)
{
    size->x = r_window_width;
    size->y = r_window_height;
}

void r_RecomputeInvViewMatrix(struct r_view_t *view)
{
    mat4_t_invvm(&view->inv_view_matrix, &view->view_transform);
}

void r_RecomputeProjectionMatrix(struct r_view_t *view)
{
    mat4_t_persp(&view->projection_matrix, view->fov_y, (float)view->viewport.width / (float)view->viewport.height, 0.1, 500.0);
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
    r_RecomputeProjectionMatrix(&r_view);
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

//void r_BeginDrawCmdSubmission(struct r_submission_state_t *submission_state, struct r_begin_submission_info_t *begin_info)
//{
//    struct r_draw_cmd_list_t *cmd_list;
//
//    if(submission_state->current_draw_cmd_list == 0xffffffff)
//    {
//        submission_state->current_draw_cmd_list = add_stack_list_element(&submission_state->draw_cmd_lists, NULL);
//        cmd_list = get_stack_list_element(&submission_state->draw_cmd_lists, submission_state->current_draw_cmd_list);
//        if(!cmd_list->draw_cmds.size)
//        {
//            cmd_list->draw_cmds = create_list(submission_state->draw_cmd_size, 1024);
//        }
//    }
//    
//    memcpy(&cmd_list->begin_info, begin_info, sizeof(struct r_begin_submission_info_t));
//    mat4_t_mul(&cmd_list->view_projection_matrix, &begin_info->inv_view_matrix, &begin_info->projection_matrix);
//    cmd_list->draw_cmds.cursor = 0;
//}
//
//uint32_t r_EndDrawCmdSubmission(struct r_submission_state_t *submission_state)
//{
//    uint32_t draw_cmd_list = submission_state->current_draw_cmd_list;
//    if(submission_state->current_draw_cmd_list != 0xffffffff)
//    {
//        draw_cmd_list = submission_state->current_draw_cmd_list;
//        add_list_element(&submission_state->pending_draw_cmd_lists, &draw_cmd_list);
//        submission_state->current_draw_cmd_list = 0xffffffff;
//    }
//    return draw_cmd_list;
//}

struct r_draw_cmd_list_t *r_GetDrawCmdListPointer(struct r_draw_cmd_list_h handle)
{
    struct r_draw_cmd_list_t *cmd_list = NULL;
    cmd_list = get_stack_list_element(&r_submission_state.draw_cmd_lists, handle.index);
    
    if(cmd_list && cmd_list->state == R_DRAW_CMD_LIST_STATE_FREE)
    {
        cmd_list = NULL;
    }
    
    return cmd_list;
}

struct r_draw_cmd_list_h r_BeginDrawCmdList(struct r_begin_submission_info_t *begin_info)
{
    struct r_draw_cmd_list_t *cmd_list;
    struct r_draw_cmd_list_h handle;
    
    handle.index = add_stack_list_element(&r_submission_state.draw_cmd_lists, NULL);
    cmd_list = get_stack_list_element(&r_submission_state.draw_cmd_lists, handle.index);
    
    if(!cmd_list->draw_cmds.size)
    {
        cmd_list->draw_cmds = create_list(sizeof(struct r_draw_cmd_t), 1024);
        cmd_list->immediate_draw_cmds = create_list(sizeof(struct r_i_draw_cmd_t), 1024);
        cmd_list->portal_handles = create_list(sizeof(struct r_portal_h), 64);
    }
    
    memcpy(&cmd_list->begin_info, begin_info, sizeof(struct r_begin_submission_info_t));
    mat4_t_mul(&cmd_list->view_projection_matrix, &begin_info->inv_view_matrix, &begin_info->projection_matrix);
    
    cmd_list->draw_cmds.cursor = 0;
    cmd_list->immediate_draw_cmds.cursor = 0;
    cmd_list->portal_handles.cursor = 0;
    cmd_list->state = R_DRAW_CMD_LIST_STATE_PENDING;
    
    return handle;
}

int32_t r_CmpDrawCmds(void *a, void *b)
{
    struct r_draw_cmd_t *cmd_a = (struct r_draw_cmd_t *)a;
    struct r_draw_cmd_t *cmd_b = (struct r_draw_cmd_t *)b;
    ptrdiff_t diff = cmd_a->material - cmd_b->material;
    return (int32_t)diff;
}

void r_EndDrawCmdList(struct r_draw_cmd_list_h handle)
{
    struct r_draw_cmd_list_t *cmd_list;
    cmd_list = r_GetDrawCmdListPointer(handle);
    if(cmd_list && cmd_list->state == R_DRAW_CMD_LIST_STATE_PENDING)
    {
        if(cmd_list->draw_cmds.cursor || cmd_list->immediate_draw_cmds.cursor || cmd_list->portal_handles.cursor)
        {
            qsort_list(&cmd_list->draw_cmds, r_CmpDrawCmds);
            while(!r_submission_state.ready_draw_cmd_lists.free_slots);
            add_ringbuffer_element(&r_submission_state.ready_draw_cmd_lists, &handle.index);
            cmd_list->state = R_DRAW_CMD_LIST_STATE_READY;
        }
        else
        {
            r_ResetDrawCmdList(handle);
        }
    }    
}

void r_ResetDrawCmdList(struct r_draw_cmd_list_h handle)
{
    struct r_draw_cmd_list_t *cmd_list;
    cmd_list = r_GetDrawCmdListPointer(handle);
    
    if(cmd_list)
    {
        cmd_list->state = R_DRAW_CMD_LIST_STATE_FREE;
        remove_stack_list_element(&r_submission_state.draw_cmd_lists, handle.index);
    }
}

void r_Draw(struct r_draw_cmd_list_h handle, uint32_t start, uint32_t count, struct r_material_t *material, mat4_t *transform)
{
    struct r_draw_cmd_list_t *cmd_list;
    struct r_draw_cmd_t *draw_cmd;
    
    if(count)
    {
        cmd_list = r_GetDrawCmdListPointer(handle);
        draw_cmd = get_list_element(&cmd_list->draw_cmds, add_list_element(&cmd_list->draw_cmds, NULL));
        
        draw_cmd->material = material;
        draw_cmd->start = start;
        draw_cmd->count = count;
        draw_cmd->vertex_offset = 0xffffffff;
        memcpy(&draw_cmd->transform, transform, sizeof(mat4_t));
    }
}

void r_DrawIndexed(struct r_draw_cmd_list_h handle, uint32_t start, uint32_t count, uint32_t vertex_offset, struct r_material_t *material, mat4_t *transform)
{
    struct r_draw_cmd_list_t *cmd_list;
    struct r_draw_cmd_t *draw_cmd;
    
    if(count)
    {
        cmd_list = r_GetDrawCmdListPointer(handle);
        draw_cmd = get_list_element(&cmd_list->draw_cmds, add_list_element(&cmd_list->draw_cmds, NULL));
        
        draw_cmd->material = material;
        draw_cmd->start = start;
        draw_cmd->count = count;
        draw_cmd->vertex_offset = vertex_offset;
        memcpy(&draw_cmd->transform, transform, sizeof(mat4_t));
    }
}

void r_DispatchPending()
{
//    struct r_render_pass_t *render_pass;
//    struct r_framebuffer_t *framebuffer;
//    struct r_framebuffer_t *default_framebuffer;
//    struct r_framebuffer_description_t *default_framebuffer_description;
//    struct r_buffer_heap_t *vertex_heap;
//    struct r_buffer_heap_t *index_heap;
    VkWriteDescriptorSet descriptor_set_write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
//    VkClearAttachment clear_attachments[2];
//    VkClearRect clear_rects[2];
    VkViewport viewport;

    struct r_submit_info_t submit_info = {};
    submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submit_info.command_buffer_count = r_submission_state.base.pending_draw_cmd_lists.cursor;
//    submit_info.command_buffers = alloca(sizeof(union r_command_buffer_h) * submit_info.command_buffer_count);
    submit_info.command_buffer_count = 0;
    
    struct r_buffer_heap_t *vertex_heap = r_GetHeapPointer(R_DEFAULT_VERTEX_HEAP);
    struct r_buffer_heap_t *index_heap = r_GetHeapPointer(R_DEFAULT_INDEX_HEAP);
    struct r_render_pass_t *render_pass = r_GetRenderPassPointer(r_render_pass);
    struct r_framebuffer_t *default_framebuffer = r_GetFramebufferPointer(r_framebuffer);
    struct r_framebuffer_description_t *default_framebuffer_description = r_GetFramebufferDescriptionPointer(r_framebuffer);
    struct r_pipeline_t *pipeline = r_GetPipelinePointer(render_pass->pipelines[0]);
    
    VkClearAttachment clear_attachments[] = {
        [0] = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .clearValue = (VkClearValue){
                .color.float32[0] = 0.01,
                .color.float32[1] = 0.01,
                .color.float32[2] = 0.03,
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
    
//    for(uint32_t pending_index = 0; pending_index < r_submission_state.ready_draw_cmd_lists.free_slots; pending_index++)
    uint32_t ready_cmd_list_index = 0;
    union r_command_buffer_h command_buffer = r_AllocateCommandBuffer();
    r_vkBeginCommandBuffer(command_buffer);
    r_vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_heap->buffer, &(VkDeviceSize){0});
    r_vkCmdBindIndexBuffer(command_buffer, index_heap->buffer, (VkDeviceSize){0}, VK_INDEX_TYPE_UINT32);
    r_vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    
    while(r_submission_state.ready_draw_cmd_lists.next_in != r_submission_state.ready_draw_cmd_lists.next_out)
    {
        struct r_draw_cmd_list_h handle = *(struct r_draw_cmd_list_h *)get_ringbuffer_element(&r_submission_state.ready_draw_cmd_lists);
        struct r_draw_cmd_list_t *cmd_list = r_GetDrawCmdListPointer(handle);
        
        r_vkCmdSetViewport(command_buffer, 0, 1, &cmd_list->begin_info.viewport);
        r_vkCmdSetScissor(command_buffer, 0, 1, &cmd_list->begin_info.scissor);
        
        struct r_framebuffer_t *framebuffer = r_GetFramebufferPointer(cmd_list->begin_info.framebuffer);
        
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
            
            if(draw_cmd->vertex_offset == 0xffffffff)
            {
                r_vkCmdDraw(command_buffer, draw_cmd->count, 1, draw_cmd->start, 0);
            }
            else
            {
                r_vkCmdDrawIndexed(command_buffer, draw_cmd->count, 1, draw_cmd->start, draw_cmd->vertex_offset, 0);
            }
        }
        
        
        
        struct r_i_draw_state_t current_draw_state;
        struct r_pipeline_t *current_pipeline = NULL;
        struct r_texture_h current_texture = R_INVALID_TEXTURE_HANDLE;
        for(uint32_t draw_cmd_index = 0; draw_cmd_index < cmd_list->immediate_draw_cmds.cursor; draw_cmd_index++)
        {
            struct r_i_draw_cmd_t *draw_cmd = get_list_element(&cmd_list->immediate_draw_cmds, draw_cmd_index);
            struct r_i_draw_cmd_data_t *draw_data;
            
            switch(draw_cmd->type)
            {
                case R_I_DRAW_CMD_DRAW:
                {
                    struct ds_chunk_t *chunk = ds_get_chunk_pointer(&r_submission_state.immediate_draw_data_heap, draw_cmd->data);
                    struct r_i_draw_cmd_data_t *draw_data = r_submission_state.immediate_draw_data_buffer + chunk->start;
                    mat4_t model_view_projection_matrix;
                    mat4_t_identity(&model_view_projection_matrix);
                    mat4_t_mul(&model_view_projection_matrix, &draw_data->model_matrix, &cmd_list->view_projection_matrix);
                    r_vkCmdPushConstants(command_buffer, current_pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), &model_view_projection_matrix);
                    
                    if(draw_data->indexed)
                    {
                        r_vkCmdDrawIndexed(command_buffer, draw_data->count, 1, draw_data->index_start, draw_data->vertex_start, 0);
                    }
                    else
                    {
                        r_vkCmdDraw(command_buffer, draw_data->count, 1, draw_data->vertex_start, 0);
                    }
                }
                break;
                
                case R_I_DRAW_CMD_UPLOAD_DATA:
                {
                    struct ds_chunk_t *chunk = ds_get_chunk_pointer(&r_submission_state.immediate_draw_data_heap, draw_cmd->data);
                    struct r_i_upload_buffer_data_t *data = r_submission_state.immediate_draw_data_buffer + chunk->start;
                    uint32_t offset = data->start * data->size;
                    uint32_t size = data->count * data->size;
                    
                    if(data->index_data)
                    {
                        r_FillBufferChunk(r_i_index_chunk, data->data, size, offset);
                    }
                    else
                    {
                        r_FillBufferChunk(r_i_vertex_chunk, data->data, size, offset);
                    }
                }
                break;

                case R_I_DRAW_CMD_SET_DRAW_STATE:
                {
                    struct ds_chunk_t *chunk = ds_get_chunk_pointer(&r_submission_state.immediate_draw_data_heap, draw_cmd->data);
                    struct r_i_set_draw_state_data_t *draw_state = r_submission_state.immediate_draw_data_buffer + chunk->start;
                    struct r_pipeline_t *next_pipeline;
                    uint32_t udpate_texture_state = 0;
                    
                    r_vkCmdSetScissor(command_buffer, 0, 1, &draw_state->draw_state.scissor);
                    next_pipeline = r_GetPipelinePointerByState(r_i_render_pass, 0, &draw_state->draw_state.pipeline_state);
                    
                    if(next_pipeline != current_pipeline)
                    {
                        current_pipeline = next_pipeline;
                        r_vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline->pipeline);
                        udpate_texture_state = 1;
                    }
                    r_vkCmdSetLineWidth(command_buffer, draw_state->draw_state.line_width); 
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
                        r_vkUpdateDescriptorSets(1, &descriptor_write);                        
                        r_vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline->layout, 0, 1, &descriptor_set, 0, NULL);
                    }
                }
                break;
            }
            if(ds_get_chunk_pointer(&r_submission_state.immediate_draw_data_heap, draw_cmd->data))
            {
                ds_free_chunk(&r_submission_state.immediate_draw_data_heap, draw_cmd->data);
            }
        }
            
        r_vkCmdEndRenderPass(command_buffer);
        r_ResetDrawCmdList(handle);
    }
    
    r_vkEndCommandBuffer(command_buffer);
    
    submit_info.command_buffers = &command_buffer;
    submit_info.command_buffer_count = 1;
    
    struct r_fence_h fence = r_AllocFence();
    
//    r_vkResetFences(1, &r_draw_fence);
    r_vkQueueSubmit(r_draw_queue, 1, &submit_info, fence);
    r_vkWaitForFences(1, &fence, 1, 0xffffffffffffffff);
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
        uniform_buffer->event = r_AllocEvent();
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

void r_i_SetDrawState(struct r_draw_cmd_list_h handle, struct r_i_draw_state_t *draw_state)
{
    struct r_i_set_draw_state_data_t *draw_state_data;
    struct ds_chunk_h draw_cmd_data;
    struct ds_chunk_t *chunk;
    draw_cmd_data = r_i_AllocateDrawCmdData(sizeof(struct r_i_set_draw_state_data_t));
    chunk = ds_get_chunk_pointer(&r_submission_state.immediate_draw_data_heap, draw_cmd_data);
    draw_state_data = r_submission_state.immediate_draw_data_buffer + chunk->start;
    draw_state_data->draw_state = *draw_state;
    r_i_SubmitCmd(handle, R_I_DRAW_CMD_SET_DRAW_STATE, draw_cmd_data);
}

struct ds_chunk_h r_i_AllocateDrawCmdData(uint32_t size)
{    
    struct ds_chunk_h chunk;
    uint32_t data_size = 1;
    
    if(size > R_I_DRAW_DATA_SLOT_SIZE)
    {
        data_size += size / R_I_DRAW_DATA_SLOT_SIZE;
    }
    
    chunk = ds_alloc_chunk(&r_submission_state.immediate_draw_data_heap, data_size * R_I_DRAW_DATA_SLOT_SIZE, R_I_DRAW_DATA_SLOT_SIZE);
    return chunk;
    
//    if(size > R_I_DRAW_DATA_SLOT_SIZE)
//    {
//        /* the data passed in is bigger than R_D_DRAW_DATA_SIZE, so we'll
//        need to accomodate it. */
//
//        size -= R_I_DRAW_DATA_SLOT_SIZE;
//        /* how many slots in the list this data takes */
//        data_slots += 1 + size / R_I_DRAW_DATA_SLOT_SIZE;
//        /* how many slots the current buffer of the list has available */
//        available_slots = (r_i_submission_state.cmd_data.buffer_size - r_i_submission_state.cmd_data.cursor % r_i_submission_state.cmd_data.buffer_size);
//        if(data_slots > available_slots)
//        {
//            while(available_slots)
//            {
//                /* the current buffer in the list doesn't have enough space to hold this data
//                contiguously, so we'll "pad" it until it gets to the start of the next buffer,
//                in which case it'll probably be enough. */
//                add_list_element(&r_i_submission_state.cmd_data, NULL);
//                available_slots--;
//            }
//        }
//
//        /* capture the index of the first slot. This is where the allocation for this draw
//        cmd begins */
//        data_index = add_list_element(&r_i_submission_state.cmd_data, NULL);
//        data_slots--;
//        while(data_slots)
//        {
//            /* data_size is bigger than the slot size, so we'll need to allocate some more
//            slots to accommodate it */
//            add_list_element(&r_i_submission_state.cmd_data, NULL);
//            data_slots--;
//        }
//        mem_CheckGuards();
//    }
//    else
//    {
//        data_index = add_list_element(&r_i_submission_state.cmd_data, NULL);
//    }

//    return get_list_element(&r_i_submission_state.cmd_data, data_index);
}

void r_i_FreeDrawCmdData(void *data)
{
    
}

void r_i_SubmitCmd(struct r_draw_cmd_list_h handle, uint32_t type, struct ds_chunk_h data)
{
    struct r_i_draw_cmd_t *draw_cmd;
    struct r_draw_cmd_list_t *cmd_list;

    cmd_list = r_GetDrawCmdListPointer(handle);
    draw_cmd = get_list_element(&cmd_list->immediate_draw_cmds, add_list_element(&cmd_list->immediate_draw_cmds, NULL));
    draw_cmd->type = type;
    draw_cmd->data = data;
}

uint32_t r_i_UploadVertices(struct r_draw_cmd_list_h handle, struct r_i_vertex_t *vertices, uint32_t count)
{
    return r_i_UploadBufferData(handle, vertices, sizeof(struct r_i_vertex_t), count, 0);
}

uint32_t r_i_UploadIndices(struct r_draw_cmd_list_h handle, uint32_t *indices, uint32_t count)
{
    return r_i_UploadBufferData(handle, indices, sizeof(uint32_t), count, 1);
}

uint32_t r_i_UploadBufferData(struct r_draw_cmd_list_h handle, void *data, uint32_t size, uint32_t count, uint32_t index_data)
{
    struct r_i_upload_buffer_data_t *upload_data;
    uint32_t data_size;
    struct r_chunk_t *chunk;
    struct ds_chunk_h draw_cmd_data;
    
    if(!count || !size || !data)
    {
        return;
    }
    
    draw_cmd_data = r_i_AllocateDrawCmdData(sizeof(struct r_i_upload_buffer_data_t) + size * count);
    chunk = ds_get_chunk_pointer(&r_submission_state.immediate_draw_data_heap, draw_cmd_data);
    
    upload_data = r_submission_state.immediate_draw_data_buffer + chunk->start;
    upload_data->size = size;
    upload_data->count = count;
    upload_data->index_data = index_data;
    
    upload_data->data = upload_data + 1;
    memcpy(upload_data->data, data, size * count);
    
    if(index_data)
    {
        chunk = r_GetChunkPointer(r_i_index_chunk);
        upload_data->start = chunk->start / sizeof(uint32_t) + r_submission_state.index_cursor;
        r_submission_state.index_cursor += count;
    }
    else
    {
        chunk = r_GetChunkPointer(r_i_vertex_chunk);
        upload_data->start = chunk->start / sizeof(struct r_i_vertex_t) + r_submission_state.vertex_cursor;
        r_submission_state.vertex_cursor += count;
    }
    
    r_i_SubmitCmd(handle, R_I_DRAW_CMD_UPLOAD_DATA, draw_cmd_data);
    return upload_data->start;
}

void r_i_Draw(struct r_draw_cmd_list_h handle, uint32_t vertex_start, uint32_t index_start, uint32_t count, uint32_t indexed, mat4_t *transform)
{
    struct r_i_draw_cmd_data_t *data;
    struct ds_chunk_h draw_cmd_data;
    struct ds_chunk_t *chunk;
    
    draw_cmd_data = r_i_AllocateDrawCmdData(sizeof(struct r_i_draw_cmd_data_t));
    chunk = ds_get_chunk_pointer(&r_submission_state.immediate_draw_data_heap, draw_cmd_data);
    data = r_submission_state.immediate_draw_data_buffer + chunk->start;
    data->vertex_start = vertex_start;
    data->index_start = index_start;
    data->count = count;
    data->indexed = indexed;
    
    if(transform)
    {
        memcpy(&data->model_matrix, transform, sizeof(mat4_t));
    }
    else
    {
        mat4_t_identity(&data->model_matrix);
    }
    
    r_i_SubmitCmd(handle, R_I_DRAW_CMD_DRAW, draw_cmd_data);
}

void r_i_DrawImmediate(struct r_draw_cmd_list_h handle, struct r_i_vertex_t *vertices, uint32_t vertex_count, uint32_t *indices, uint32_t index_count, mat4_t *transform)
{
    uint32_t vertex_start;
    uint32_t index_start;
    uint32_t count = vertex_count;
    
    vertex_start = r_i_UploadVertices(handle, vertices, vertex_count);
    if(indices)
    {
        index_start = r_i_UploadIndices(handle, indices, index_count);
        count = index_count;
    }
    
    r_i_Draw(handle, vertex_start, index_start, count, indices != NULL, transform);
}





