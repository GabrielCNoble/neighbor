#include "r_draw.h"
//#include "spr.h"
#include "lib/SDL/include/SDL.h"
#include "lib/dstuff/ds_file.h"
#include "lib/dstuff/ds_path.h"
#include "lib/dstuff/ds_mem.h"
#include "ent.h"
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
struct r_texture_state_t r_default_texture_state;
//struct r_i_submission_state_t r_i_submission_state = {};


struct list_t r_uniform_buffers;
struct list_t r_free_uniform_buffers;
struct list_t r_used_uniform_buffers;
VkPhysicalDeviceLimits *r_limits;

struct r_render_pass_handle_t r_render_pass;
struct r_shader_handle_t r_lit_vertex_shader;
struct r_shader_handle_t r_lit_fragment_shader;

struct r_shader_handle_t r_flat_vertex_shader;
struct r_shader_handle_t r_flat_fragment_shader;

//struct r_render_pass_handle_t r_i_render_pass;
struct r_framebuffer_h r_framebuffer;
//struct r_buffer_h r_i_vertex_buffer;
//struct r_buffer_h r_i_index_buffer;
struct r_queue_h r_draw_queue;
//VkFence r_draw_fence;

//struct r_heap_h r_vertex_heap;
//struct r_heap_h r_index_heap;
struct r_chunk_h r_temp_vertex_chunk;
uint32_t r_temp_vertex_chunk_start;
uint32_t r_temp_vertex_chunk_cursor;

struct r_chunk_h r_temp_index_chunk;
uint32_t r_temp_index_chunk_start;
uint32_t r_temp_index_chunk_cursor;


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
struct r_cluster_list_t r_cluster_list;
VkInstance r_instance;

extern struct stack_list_t ed_entities;



struct 
{
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    VkAccessFlags src_access;
    VkAccessFlags dst_access;
}r_sync_data[] = {
    [R_LAYOUT_TRANSITION_SYNC_RENDERPASS_RENDERPASS] = {
        .src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .src_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dst_access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
    },
    [R_LAYOUT_TRANSITION_SYNC_RENDERPASS_RENDERPASS + 1] = {
        .src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dst_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
    },
    [R_LAYOUT_TRANSITION_SYNC_RENDERPASS_SHADER] = {
        .src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .src_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dst_access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
    },
    [R_LAYOUT_TRANSITION_SYNC_RENDERPASS_SHADER + 1] = {
        .src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dst_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
    },
    [R_LAYOUT_TRANSITION_SYNC_SHADER_RENDERPASS] = {
        .src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .src_access = 0,
        .dst_access = 0,
    },
    [R_LAYOUT_TRANSITION_SYNC_SHADER_RENDERPASS + 1] = {
        .src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .src_access = 0,
        .dst_access = 0
    }
};

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
    r_lit_vertex_shader = r_CreateShader(&shader_description);
    struct r_shader_t *vertex_shader = r_GetShaderPointer(r_lit_vertex_shader);
    mem_Free(shader_description.code);
    
    shader_description = (struct r_shader_description_t){
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .resource_count = 1,
        .resources = (struct r_resource_binding_t []){{.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = 1}}
    };
    fopen("neighbor/shaders/d_lit.frag.spv", "rb");
    read_file(file, &shader_description.code, &shader_description.code_size);
    fclose(file);
    r_lit_fragment_shader = r_CreateShader(&shader_description);
    struct r_shader_t *fragment_shader = r_GetShaderPointer(r_lit_fragment_shader);
    mem_Free(shader_description.code);
    
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
    r_flat_vertex_shader = r_CreateShader(&shader_description);
//    vertex_shader = r_GetShaderPointer(r_flat_vertex_shader);



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
    r_flat_fragment_shader = r_CreateShader(&shader_description);
    
    struct r_attachment_d attachments[] = {
        {.format = VK_FORMAT_R8G8B8A8_UNORM, .initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, .final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.format = VK_FORMAT_D32_SFLOAT, .initial_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}
    };

    render_pass_description = (struct r_render_pass_description_t){
        .attachments = attachments,
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
                .pipeline_description_count = 2,
                .pipeline_descriptions = (struct r_pipeline_description_t[]){
                    {
                        .shader_count = 2,
                        .shaders = (struct r_shader_handle_t []){r_lit_vertex_shader, r_lit_fragment_shader}
                    },
                    {
                        .shader_count = 2,
                        .shaders = (struct r_shader_handle_t []){r_flat_vertex_shader, r_flat_fragment_shader}
                    }
                },
            }
        },
    };

    r_render_pass = r_CreateRenderPass(&render_pass_description);
    
//    render_pass_description = (struct r_render_pass_description_t){
//        .attachment_count = 2,
//        .attachments = attachments,
//        .subpass_count = 1,
//        .subpasses = &(struct r_subpass_description_t){
//            .color_attachment_count = 1,
//            .color_attachments = (VkAttachmentReference[]){{.attachment = 0}},
//            .depth_stencil_attachment = &(VkAttachmentReference){.attachment = 1},
//            .pipeline_description_count = 1,
//            .pipeline_descriptions = (struct r_pipeline_description_t []){
//                {
//                    .shader_count = 2,
//                    .shaders = (struct r_shader_t *[]){vertex_shader, fragment_shader},
//                },
//            }
//        }
//    };
// 
//    r_i_render_pass = r_CreateRenderPass(&render_pass_description);
    
    
    
    
    

    struct r_framebuffer_d framebuffer_description = (struct r_framebuffer_d){
        .frame_count = 2,
        .width = R_DEFAULT_WIDTH,
        .height = R_DEFAULT_HEIGHT,
        .render_pass = r_GetRenderPassPointer(r_render_pass)
    };
    
    r_framebuffer = r_CreateFramebuffer(&framebuffer_description);
    r_limits = r_GetDeviceLimits();

    r_temp_vertex_chunk = r_AllocVertexChunk(sizeof(struct r_i_vertex_t), R_I_MAX_VERTEX_COUNT, sizeof(struct r_i_vertex_t));
    r_temp_index_chunk = r_AllocIndexChunk(sizeof(uint32_t), R_I_MAX_INDEX_COUNT, sizeof(uint32_t));
    
    struct r_chunk_t *chunk = r_GetChunkPointer(r_temp_vertex_chunk);
    r_temp_vertex_chunk_start = chunk->start;
    chunk = r_GetChunkPointer(r_temp_index_chunk);
    r_temp_index_chunk_start = chunk->start;
    
    
//    r_submission_state.draw_cmd_lists = create_stack_list(sizeof(struct r_draw_cmd_list_t), 512);
//    r_submission_state.immediate_draw_data_heap = ds_create_heap(33554432);
//    r_submission_state.immediate_draw_data_buffer = mem_Calloc(1, 33554432);
//    r_submission_state.ready_draw_cmd_lists = create_ringbuffer(sizeof(uint32_t), 512);
//    r_submission_state.index_cursor = 0;
//    r_submission_state.vertex_cursor = 0;

    r_uniform_buffers = create_list(sizeof(struct r_uniform_buffer_t), 64);
    r_free_uniform_buffers = create_list(sizeof(uint32_t), 64);
    r_used_uniform_buffers = create_list(sizeof(uint32_t), 64);
}

void r_DrawShutdown()
{

}

void r_BeginFrame()
{
    union r_command_buffer_h command_buffer = r_AllocateCommandBuffer();
    r_BeginCommandBuffer(command_buffer, &r_view);
    r_BeginRenderPass(command_buffer, &r_view.scissor, r_framebuffer);
    r_ClearAttachments(command_buffer, 1, 1);
    r_EndRenderPass(command_buffer);
    r_EndCommandBuffer(command_buffer);
    r_SubmitCommandBuffers(1, &command_buffer, R_INVALID_FENCE_HANDLE);
}

void r_EndFrame()
{
    r_PresentFramebuffer(r_framebuffer);
    r_temp_vertex_chunk_cursor = 0;
    r_temp_index_chunk_cursor = 0;
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
    struct r_framebuffer_d *backbuffer_description;
    struct r_framebuffer_d description;
    
    backbuffer_description = r_GetFramebufferDescPointer(r_framebuffer);
    
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

//struct r_draw_cmd_list_t *r_GetDrawCmdListPointer(struct r_draw_cmd_list_h handle)
//{
//    struct r_draw_cmd_list_t *cmd_list = NULL;
//    cmd_list = get_stack_list_element(&r_submission_state.draw_cmd_lists, handle.index);
//    
//    if(cmd_list && cmd_list->state == R_DRAW_CMD_LIST_STATE_FREE)
//    {
//        cmd_list = NULL;
//    }
//    
//    return cmd_list;
//}
//
//struct r_draw_cmd_list_h r_BeginDrawCmdList(struct r_begin_submission_info_t *begin_info)
//{
//    struct r_draw_cmd_list_t *cmd_list;
//    struct r_draw_cmd_list_h handle;
//    
//    handle.index = add_stack_list_element(&r_submission_state.draw_cmd_lists, NULL);
//    cmd_list = get_stack_list_element(&r_submission_state.draw_cmd_lists, handle.index);
//    
//    if(!cmd_list->draw_cmds.size)
//    {
//        cmd_list->draw_cmds = create_list(sizeof(struct r_draw_cmd_t), 1024);
//        cmd_list->immediate_draw_cmds = create_list(sizeof(struct r_i_draw_cmd_t), 1024);
//        cmd_list->portal_handles = create_list(sizeof(struct r_portal_h), 64);
//    }
//    
//    memcpy(&cmd_list->begin_info, begin_info, sizeof(struct r_begin_submission_info_t));
//    mat4_t_mul(&cmd_list->view_projection_matrix, &begin_info->inv_view_matrix, &begin_info->projection_matrix);
//    
//    cmd_list->draw_cmds.cursor = 0;
//    cmd_list->immediate_draw_cmds.cursor = 0;
//    cmd_list->portal_handles.cursor = 0;
//    cmd_list->state = R_DRAW_CMD_LIST_STATE_PENDING;
//    
//    return handle;
//}
//
//int32_t r_CmpDrawCmds(void *a, void *b)
//{
//    struct r_draw_cmd_t *cmd_a = (struct r_draw_cmd_t *)a;
//    struct r_draw_cmd_t *cmd_b = (struct r_draw_cmd_t *)b;
//    ptrdiff_t diff = cmd_a->material - cmd_b->material;
//    return (int32_t)diff;
//}
//
//void r_EndDrawCmdList(struct r_draw_cmd_list_h handle)
//{
//    struct r_draw_cmd_list_t *cmd_list;
//    cmd_list = r_GetDrawCmdListPointer(handle);
//    if(cmd_list && cmd_list->state == R_DRAW_CMD_LIST_STATE_PENDING)
//    {
//        qsort_list(&cmd_list->draw_cmds, r_CmpDrawCmds);
//        while(!r_submission_state.ready_draw_cmd_lists.free_slots);
//        add_ringbuffer_element(&r_submission_state.ready_draw_cmd_lists, &handle.index);
//        cmd_list->state = R_DRAW_CMD_LIST_STATE_READY;
//    }    
//}
//
//void r_ResetDrawCmdList(struct r_draw_cmd_list_h handle)
//{
//    struct r_draw_cmd_list_t *cmd_list;
//    cmd_list = r_GetDrawCmdListPointer(handle);
//    
//    if(cmd_list)
//    {
//        cmd_list->state = R_DRAW_CMD_LIST_STATE_FREE;
//        remove_stack_list_element(&r_submission_state.draw_cmd_lists, handle.index);
//    }
//}

void r_BeginCommandBuffer(union r_command_buffer_h command_buffer, struct r_view_t *view)
{
    struct r_buffer_heap_t *vertex_heap;
    struct r_buffer_heap_t *index_heap;
    struct r_render_pass_t *render_pass;
    struct r_framebuffer_t *framebuffer_ptr;
    
    vertex_heap = r_GetHeapPointer(R_DEFAULT_VERTEX_HEAP);
    index_heap = r_GetHeapPointer(R_DEFAULT_INDEX_HEAP);
    
    r_vkBeginCommandBuffer(command_buffer);
    r_vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_heap->buffer, &(VkDeviceSize){0});
    r_vkCmdBindIndexBuffer(command_buffer, index_heap->buffer, (VkDeviceSize){0}, VK_INDEX_TYPE_UINT32);
    r_vkCmdSetViewport(command_buffer, 0, 1, &view->viewport);
    
//    struct r_render_pass_begin_info_t begin_info = {};
//    begin_info.framebuffer = framebuffer;
//    begin_info.render_area = view->scissor;
//    begin_info.render_pass = r_render_pass;
//    
//    r_vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void r_EndCommandBuffer(union r_command_buffer_h command_buffer)
{
    r_vkEndCommandBuffer(command_buffer);
}

void r_BeginRenderPass(union r_command_buffer_h command_buffer, VkRect2D *render_area, struct r_framebuffer_h framebuffer)
{
    struct r_render_pass_begin_info_t begin_info = {};
    begin_info.framebuffer = framebuffer;
    begin_info.render_area = *render_area;
    begin_info.render_pass = r_render_pass;
    
    r_vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void r_EndRenderPass(union r_command_buffer_h command_buffer)
{
    r_vkCmdEndRenderPass(command_buffer);
}

void r_SubmitCommandBuffers(uint32_t command_buffer_count, union r_command_buffer_h *command_buffers, struct r_fence_h fence)
{
    struct r_submit_info_t submit_info = {};
    submit_info.command_buffer_count = command_buffer_count;
    submit_info.command_buffers = command_buffers;
    r_vkQueueSubmit(r_draw_queue, 1, &submit_info, fence);
}

void r_ClearAttachments(union r_command_buffer_h command_buffer, uint32_t clear_color, uint32_t clear_depth)
{
    VkClearAttachment clear_values[2] = {};
    VkClearRect clear_rects[2] = {};
    struct r_command_buffer_t *command_buffer_ptr;
    struct r_framebuffer_d *framebuffer_description;
    
    clear_values[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clear_values[0].clearValue.color.float32[0] = 0.0;
    clear_values[0].clearValue.color.float32[1] = 0.0;
    clear_values[0].clearValue.color.float32[2] = 0.0;
    clear_values[0].clearValue.color.float32[3] = 1.0;
    
    clear_values[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    clear_values[1].clearValue.depthStencil.depth = 1.0;
    
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    framebuffer_description = r_GetFramebufferDescPointer(command_buffer_ptr->framebuffer);
    
    clear_rects[0].rect.offset.x = 0;
    clear_rects[0].rect.offset.y = 0;
    clear_rects[0].rect.extent.width = framebuffer_description->width;
    clear_rects[0].rect.extent.height = framebuffer_description->height;
    clear_rects[0].baseArrayLayer = 0;
    clear_rects[0].layerCount = 1;
    
    clear_rects[1].rect.offset.x = 0;
    clear_rects[1].rect.offset.y = 0;
    clear_rects[1].rect.extent.width = framebuffer_description->width;
    clear_rects[1].rect.extent.height = framebuffer_description->height;
    clear_rects[1].baseArrayLayer = 0;
    clear_rects[1].layerCount = 1;
    
    r_vkCmdClearAttachments(command_buffer, 2, clear_values, 2, clear_rects);
}

void r_Draw(union r_command_buffer_h command_buffer, uint32_t start, uint32_t count)
{
    r_vkCmdDraw(command_buffer, count, 1, start, 0);
}

void r_DrawIndexed(union r_command_buffer_h command_buffer, uint32_t start, uint32_t count, uint32_t vertex_offset)
{
    r_vkCmdDrawIndexed(command_buffer, count, 1, start, vertex_offset, 0);
}

//void r_BindTexture(union r_command_buffer_h command_buffer, uint32_t binding_index, struct r_texture_h texture)
//{
//    VkDescriptorSet descriptor_set = r_AllocateDescriptorSet(command_buffer, NULL, VK_SHADER_STAGE_FRAGMENT_BIT);
//    struct r_texture_t *texture_ptr = r_GetTexturePointer(texture);
//    VkWriteDescriptorSet update_data = {};
//    VkDescriptorImageInfo image_info = {};
//    struct r_pipeline_t *pipeline;
//    struct r_command_buffer_t *command_buffer_ptr;
//    
//    if(!texture_ptr)
//    {
//        texture_ptr = r_GetDefaultTexturePointer();
//    }
//    
//    struct r_image_t *image = texture_ptr->image;
//    
//    image_info.imageView = texture_ptr->image_view;
//    image_info.sampler = texture_ptr->sampler;
//    image_info.imageLayout = image->current_layout;
//    
//    update_data.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    update_data.descriptorCount = 1;
//    update_data.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    update_data.dstArrayElement = 0;
//    update_data.dstSet = descriptor_set;
//    update_data.dstBinding = binding_index;
//    update_data.pImageInfo = &image_info;
//    
//    r_vkUpdateDescriptorSets(1, &update_data);
//    
//    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
//    pipeline = r_GetPipelinePointer(command_buffer_ptr->current_pipeline);
//    
////    if(image->current_layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
////    {
////        r_vkCmdSetImageLayout(command_buffer, texture_ptr->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
////    }
//    
//    r_vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &descriptor_set, 0, NULL);
//}

void r_PushConstants(union r_command_buffer_h command_buffer, VkShaderStageFlagBits stage, uint32_t size, uint32_t offset, void *data)
{
    struct r_command_buffer_t *command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    struct r_pipeline_t *pipeline = r_GetPipelinePointer(command_buffer_ptr->current_pipeline);
    r_vkCmdPushConstants(command_buffer, pipeline->layout, stage, offset, size, data);
}

void r_TransitionLayout(union r_command_buffer_h command_buffer, uint32_t layout_transition_sync, uint32_t new_layout, struct r_texture_t *texture)
{
    struct r_image_memory_barrier_t memory_barrier = {};
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    
    if(texture->image->current_layout == new_layout)
    {
        return;
    }
    
    memory_barrier.src_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.dst_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.old_layout = texture->image->current_layout;
    memory_barrier.new_layout = new_layout;
    memory_barrier.subresource_range.aspectMask = texture->image->aspect_mask;
    memory_barrier.subresource_range.baseArrayLayer = 0;
    memory_barrier.subresource_range.baseMipLevel = 0;
    memory_barrier.subresource_range.layerCount = 1;
    memory_barrier.subresource_range.levelCount = 1;
    memory_barrier.image = texture->image;
    
    uint32_t is_depth_stencil_format = r_IsDepthStencilFormat(texture->image->description->format);
    
    memory_barrier.src_access_mask = r_sync_data[layout_transition_sync + is_depth_stencil_format].src_access;
    memory_barrier.dst_access_mask = r_sync_data[layout_transition_sync + is_depth_stencil_format].dst_access;
    src_stage = r_sync_data[layout_transition_sync + is_depth_stencil_format].src_stage;
    dst_stage = r_sync_data[layout_transition_sync + is_depth_stencil_format].dst_stage;
    
//    switch(layout_transition_sync)
//    {
//        case R_LAYOUT_TRANSITION_SYNC_RENDERPASS_RENDERPASS:
//            if(is_depth_stencil_format)
//            {
//                memory_barrier.src_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//                memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
//                src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
//                dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//            }
//            else
//            {
//                memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//                memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
//                src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//                dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//            }
//        break;
//        
//        case R_LAYOUT_TRANSITION_SYNC_RENDERPASS_SHADER:
//            if(is_depth_stencil_format)
//            {
//                memory_barrier.src_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//                memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
//                src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
//                dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//            }
//            else
//            {
//                memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//                memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
//                src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//                dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//            }
//        break;
//        
//        case R_LAYOUT_TRANSITION_SYNC_SHADER_RENDERPASS:
//            
//            /* write after read, only execution barrier is necessary */
//            memory_barrier.src_access_mask = 0;
//            memory_barrier.dst_access_mask = 0;
//            src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//            
//            if(is_depth_stencil_format)
//            {
//                dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//            }
//            else
//            {
//                dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//            }
//        break;
//    }
    
    r_vkCmdPipelineImageBarrier(command_buffer, src_stage, dst_stage, 1, &memory_barrier);
}

uint32_t r_FillTempVertices(void *data, uint32_t size, uint32_t count)
{
    r_FillBufferChunk(r_temp_vertex_chunk, data, size * count, r_temp_vertex_chunk_cursor);
    uint32_t start = (r_temp_vertex_chunk_start + r_temp_vertex_chunk_cursor) / size;
    r_temp_vertex_chunk_cursor += size * count;
    return start;
}

uint32_t r_FillTempIndices(void *data, uint32_t size, uint32_t count)
{
    r_FillBufferChunk(r_temp_index_chunk, data, size * count, r_temp_index_chunk_cursor);
    uint32_t start = (r_temp_index_chunk_start + r_temp_index_chunk_cursor) / size;
    r_temp_index_chunk_cursor += size * count;
    return start;
}

void r_SetDrawState(union r_command_buffer_h command_buffer, struct r_draw_state_t *draw_state)
{
    struct r_pipeline_h next_pipeline;
    struct r_pipeline_h current_pipeline;
    struct r_pipeline_t *pipeline;
    struct r_command_buffer_t *command_buffer_ptr;
    uint32_t udpate_texture_state = 0;
    
    command_buffer_ptr = r_GetCommandBufferPointer(command_buffer);
    r_vkCmdSetScissor(command_buffer, 0, 1, &draw_state->scissor);
    current_pipeline = command_buffer_ptr->current_pipeline;
    
    if(draw_state->pipeline_state)
    {
        next_pipeline = r_GetPipelineHandleByState(r_render_pass, 0, draw_state->pipeline_state);
        
        if(next_pipeline.index != current_pipeline.index)
        {
            current_pipeline = next_pipeline;
            r_vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, next_pipeline);
        }
    }
    pipeline = r_GetPipelinePointer(current_pipeline);
    r_vkCmdSetLineWidth(command_buffer, draw_state->line_width); 
    
    if(draw_state->texture_state && draw_state->texture_state->texture_count)
    {
        VkDescriptorSet descriptor_set = r_AllocateDescriptorSet(command_buffer, pipeline, VK_SHADER_STAGE_FRAGMENT_BIT);
        
        VkWriteDescriptorSet *update_state = alloca(sizeof(VkWriteDescriptorSet) * draw_state->texture_state->texture_count);
        VkDescriptorImageInfo *image_info = alloca(sizeof(VkDescriptorImageInfo) * draw_state->texture_state->texture_count);
            
        for(uint32_t texture_index = 0; texture_index < draw_state->texture_state->texture_count; texture_index++)
        {
            struct r_texture_state_binding_t *binding = draw_state->texture_state->textures + texture_index;
            struct r_texture_t *texture = binding->texture;
            struct r_image_t *image = texture->image;
            
            image_info[texture_index].sampler = texture->sampler;
            image_info[texture_index].imageView = texture->image_view;
            image_info[texture_index].imageLayout = image->current_layout;
            
            VkWriteDescriptorSet *state = update_state + texture_index;
            state->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            state->descriptorCount = 1;
            state->dstArrayElement = 0;
            state->dstBinding = binding->binding_index;
            state->dstSet = descriptor_set;
            state->pImageInfo = image_info + texture_index;
            state->pNext = NULL;
            state->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        }
        
        r_vkUpdateDescriptorSets(draw_state->texture_state->texture_count, update_state);
        r_vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &descriptor_set, 0, NULL);
    }
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_DrawVisibleEntities(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list)
{
//    struct stack_list_t *entities = ent_GetEntityList();
//    for(uint32_t entity_index = 0; entity_index < entities->cursor; entity_index++)
//    {
//        struct ent_entity_t *entity = ent_GetEntityPointer(ENT_ENTITY_HANDLE(entity_index));
//        if(entity)
//        {
//            struct mdl_model_t *model = mdl_GetModelPointer(entity->model);
//            struct mdl_batch_t *batches = mdl_GetModelLod(model, 0);
//            
//            for(uint32_t batch_index = 0; batch_index < model->batch_count; batch_index++)
//            {
//                struct mdl_batch_t *batch = batches + batch_index;
//                r_Draw(draw_cmd_list, batch->start, batch->count, r_GetDefaultMaterialPointer(), &entity->transform);
//            }
//        }
//    }
}

void r_DrawVisibleLights(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list)
{
    
}

void r_DrawVisiblePortals(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list)
{
    
}

void r_DrawVisibleWorld(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list)
{
    
}

void r_DrawVisible(struct r_view_t *view, struct r_framebuffer_h framebuffer)
{
//    struct r_begin_submission_info_t begin_info = {};
//    
//    begin_info.clear_framebuffer = 0;
//    begin_info.framebuffer = framebuffer;
//    begin_info.inv_view_matrix = view->inv_view_matrix;
//    begin_info.projection_matrix = view->projection_matrix;
//    begin_info.viewport = view->viewport;
//    begin_info.scissor = view->scissor;
//    begin_info.clear_framebuffer = 1;
//    
//    struct r_draw_cmd_list_h draw_cmd_list = r_BeginDrawCmdList(&begin_info);
//    r_DrawVisibleEntities(view, draw_cmd_list);
//    r_EndDrawCmdList(draw_cmd_list);
}




