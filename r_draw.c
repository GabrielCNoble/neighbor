#include "r_draw.h"
#include "spr.h"
#include "lib/dstuff/file/file.h"
#include "lib/dstuff/file/path.h"
#include <stdlib.h>
#include <stdio.h>

struct r_draw_state_t r_draw_state;


struct list_t r_uniform_buffers;
struct list_t r_free_uniform_buffers;
struct list_t r_used_uniform_buffers;
VkPhysicalDeviceLimits *r_limits;
//    struct list_t r_i_draw_data;
//    struct stack_list_t r_i_draw_cmd_lists;
//    struct ringbuffer_t r_i_pending_draw_cmd_lists;
//    uint32_t r_i_current_draw_cmd_list = 0xffffffff;
//    struct r_render_pass_handle_t r_i_render_passes[R_I_DRAW_CMD_LAST];
//    struct r_buffer_h r_i_vertex_buffer;

//}r_immediate_draw_state;

struct r_render_pass_handle_t r_render_pass;
struct r_framebuffer_handle_t r_framebuffer;
struct r_buffer_h r_vertex_buffer;
VkQueue r_draw_queue;
VkFence r_draw_fence;

#define R_VERTEX_BUFFER_MEMORY 8388608
#define R_UNIFORM_BUFFER_MEMORY 8388608
#define R_DEBUG_VERTEX_BUFFER_MEMORY 8388608

struct r_view_t r_view;

void r_DrawInit()
{
    FILE *file;
//    struct r_shader_description_t vertex_description = {};
//    struct r_shader_description_t fragment_description = {};
    struct r_shader_description_t shader_description = {};
    struct r_render_pass_description_t render_pass_description = {};
    long code_size;

    file = fopen("shaders/shader.vert.spv", "rb");
    read_file(file, &shader_description.code, &code_size);
    shader_description.code_size = (uint32_t)code_size;
    fclose(file);
    shader_description.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_description.vertex_binding_count = 1;
    shader_description.vertex_bindings = (struct r_vertex_binding_t []){
        {
            .size = sizeof(struct r_vertex_t),
            .attrib_count = 3,
            .attribs = (struct r_vertex_attrib_t []){
                {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_vertex_t, position)},
                {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_vertex_t, normal)},
                {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_vertex_t, tex_coords)}
            },
        }
    };
    shader_description.resource_count = 1;
    shader_description.resources = (struct r_resource_binding_t []){
        {.descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1}
    };

    struct r_shader_t *vertex_shader = r_GetShaderPointer(r_CreateShader(&shader_description));
    free(shader_description.code);

    memset(&shader_description, 0, sizeof(struct r_shader_description_t));
    fopen("shaders/shader.frag.spv", "rb");
    read_file(file, &shader_description.code, &code_size);
    shader_description.code_size = (uint32_t)code_size;
    fclose(file);
    shader_description.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_description.resource_count = 1;
    shader_description.resources = (struct r_resource_binding_t []){{.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = 1}};
    struct r_shader_t *fragment_shader = r_GetShaderPointer(r_CreateShader(&shader_description));
    free(shader_description.code);

    render_pass_description = (struct r_render_pass_description_t){
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE},
            {.format = VK_FORMAT_D32_SFLOAT, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE}
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
                .pipeline_description = &(struct r_pipeline_description_t){
                    .shader_count = 2,
                    .shaders = (struct r_shader_t *[]){vertex_shader, fragment_shader}
                },
            }
        },
    };

    r_render_pass = r_CreateRenderPass(&render_pass_description);

    struct r_framebuffer_description_t framebuffer_description = (struct r_framebuffer_description_t){
        .attachment_count = render_pass_description.attachment_count,
        .attachments = render_pass_description.attachments,
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
    buffer_create_info.size = R_VERTEX_BUFFER_MEMORY;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    r_vertex_buffer = r_CreateBuffer(&buffer_create_info);
//    buffer_create_info.size = R_DEBUG_VERTEX_BUFFER_MEMORY;
//    r_debug_vertex_buffer = r_CreateBuffer(&buffer_create_info);

    struct r_vertex_t *data = (struct r_vertex_t []){
        {.position = vec4_t_c(-1.0, -1.0, -1.0, 1.0),.tex_coords = vec4_t_c(0.0, 1.0, 0.0, 0.0)},
        {.position = vec4_t_c(1.0, -1.0, -1.0, 1.0),.tex_coords = vec4_t_c(1.0, 1.0, 0.0, 0.0)},
        {.position = vec4_t_c(1.0, 1.0, -1.0, 1.0),.tex_coords = vec4_t_c(1.0, 0.0, 0.0, 0.0)},


        {.position = vec4_t_c(1.0, 1.0, -1.0, 1.0),.tex_coords = vec4_t_c(1.0, 0.0, 0.0, 0.0)},
        {.position = vec4_t_c(-1.0, 1.0, -1.0, 1.0),.tex_coords = vec4_t_c(0.0, 0.0, 0.0, 0.0)},
        {.position = vec4_t_c(-1.0, -1.0, -1.0, 1.0),.tex_coords = vec4_t_c(0.0, 1.0, 0.0, 0.0)},
    };

    r_FillBufferChunk(r_vertex_buffer, data, sizeof(struct r_vertex_t) * 6, 0);

    r_draw_state.draw_cmd_lists = create_stack_list(sizeof(struct r_draw_cmd_list_t), 32);
    r_draw_state.pending_draw_cmd_lists = create_ringbuffer(sizeof(uint32_t), 32);
    r_draw_state.draw_calls = create_list(sizeof(struct r_draw_call_data_t), 1024);
    r_draw_state.current_draw_cmd_list = 0xffffffff;
    r_draw_state.draw_cmd_size = sizeof(struct r_draw_cmd_t);

    r_uniform_buffers = create_list(sizeof(struct r_uniform_buffer_t), 64);
    r_free_uniform_buffers = create_list(sizeof(uint32_t), 64);
    r_used_uniform_buffers = create_list(sizeof(uint32_t), 64);




//    pipeline_description.shader_count = 2;
//    pipeline_description.input_assembly_state = &(VkPipelineInputAssemblyStateCreateInfo){};
//    pipeline_description.rasterization_state = &(VkPipelineRasterizationStateCreateInfo){};

    file = fopen("shaders/i_color.vert.spv", "rb");
    read_file(file, &shader_description.code, &code_size);
    fclose(file);
    shader_description.code_size = code_size;
    shader_description.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_description.vertex_binding_count = 1;
    shader_description.vertex_bindings = &(struct r_vertex_binding_t){
        .size = sizeof(struct r_i_vertex_t),
        .attrib_count = 3,
        .attribs = (struct r_vertex_attrib_t []){
            {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_i_vertex_t, position)},
            {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_i_vertex_t, tex_coords)},
            {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(struct r_i_vertex_t, color)},
        }
    };
    shader_description.push_constant_count = 1;
    shader_description.push_constants = (struct r_push_constant_t []){{.size = sizeof(mat4_t), .offset = 0}};
    vertex_shader = r_GetShaderPointer(r_CreateShader(&shader_description));


    file = fopen("shaders/i_color.frag.spv", "rb");
    read_file(file, &shader_description.code, &code_size);
    fclose(file);
    shader_description.code_size = code_size;
    shader_description.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_description.vertex_binding_count = 0;
    shader_description.resource_count = 0;
    fragment_shader = r_GetShaderPointer(r_CreateShader(&shader_description));

    render_pass_description.attachment_count = 2;
    render_pass_description.attachments = (VkAttachmentDescription []){
        {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {.format = VK_FORMAT_D32_SFLOAT, .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}
    };
    render_pass_description.subpass_count = 1;
    render_pass_description.subpasses = &(struct r_subpass_description_t){
        .color_attachment_count = 1,
        .color_attachments = (VkAttachmentReference[]){{.attachment = 0}},
        .depth_stencil_attachment = &(VkAttachmentReference){.attachment = 1},
        .pipeline_description = &(struct r_pipeline_description_t){
            .shader_count = 2,
            .shaders = (struct r_shader_t *[]){vertex_shader, fragment_shader},
            .input_assembly_state = &(VkPipelineInputAssemblyStateCreateInfo){},
            .rasterization_state = &(VkPipelineRasterizationStateCreateInfo){}
        }
    };

//    r_i_render_passes[R_I_DRAW_CMD_DRAW_LINE] = r_CreateRenderPass(&render_pass_description);
//    for(uint32_t render_pass_index = R_I_DRAW_CMD_DRAW_LINE; render_pass_index < R_I_DRAW_CMD_DRAW_TRI_FILL_TEXTURED; render_pass_index++)
//    {
//        switch(render_pass_index)
//        {
////            case R_I_DRAW_CMD_DRAW_POINT:
////                pipeline_description.input_assembly_state->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
////                pipeline_description.rasterization_state->polygonMode = VK_POLYGON_MODE_FILL;
////            break;
//
//            case R_I_DRAW_CMD_DRAW_LINE:
//                render_pass_description.subpasses->pipeline_description.input_assembly_state->topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
//                render_pass_description.subpasses->pipeline_description.rasterization_state->polygonMode = VK_POLYGON_MODE_FILL;
//            break;
//
////            case R_I_DRAW_CMD_DRAW_TRI_LINE:
////                pipeline_description.input_assembly_state->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
////                pipeline_description.rasterization_state->polygonMode = VK_POLYGON_MODE_LINE;
////            break;
//
////            case R_I_DRAW_CMD_DRAW_TRI_FILL:
////                pipeline_description.input_assembly_state->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
////                pipeline_description.rasterization_state->polygonMode = VK_POLYGON_MODE_FILL;
////            break;
//        }
//
//        r_i_render_passes[render_pass_index] = r_CreateRenderPass(&render_pass_description);
//    }

//    r_i_draw_data = create_list(sizeof(struct r_i_draw_data_t), 512);
//    r_i_draw_cmd_lists = create_stack_list(sizeof(struct r_draw_cmd_list_t), 32);
//    r_i_pending_draw_cmd_lists = create_ringbuffer(sizeof(uint32_t), 32);
//    r_i_draw_data = create_list(sizeof(struct r_i_draw_data_t), 1024);







    r_view.width = R_DEFAULT_WIDTH;
    r_view.height = R_DEFAULT_HEIGHT;
    r_view.z_near = 0.1;
    r_view.z_far = 20.0;
    r_view.zoom = 1.0;
    r_view.position = vec2_t_c(0.0, 0.0);

    r_RecomputeInvViewMatrix();
    r_RecomputeProjectionMatrix();

//    r_DrawInitImmediate();
}

void r_DrawShutdown()
{

}

void r_BeginFrame()
{

}

void r_EndFrame()
{
    r_DispatchPending(&r_draw_state);
//    r_i_DispatchImmediate();
    r_PresentFramebuffer(r_framebuffer);
}

void r_SetViewPosition(vec2_t *position)
{
    r_view.position = *position;
    r_RecomputeInvViewMatrix();
}

void r_TranslateView(vec2_t *translation)
{
    r_view.position.x += translation->x;
    r_view.position.y += translation->y;
    r_RecomputeInvViewMatrix();
}

void r_SetViewZoom(float zoom)
{
    r_view.zoom = zoom;
    r_RecomputeProjectionMatrix();
}

void r_RecomputeInvViewMatrix()
{
    mat4_t view_matrix;
    mat4_t_identity(&view_matrix);
    view_matrix.rows[3].x = r_view.position.x;
    view_matrix.rows[3].y = r_view.position.y;
    view_matrix.rows[3].z = 0.0;
    mat4_t_invvm(&r_view.inv_view_matrix, &view_matrix);
}

void r_RecomputeProjectionMatrix()
{
    mat4_t_ortho(&r_view.projection_matrix, r_view.width * r_view.zoom, r_view.height * r_view.zoom, r_view.z_near, r_view.z_far);
}

/*
=================================================================
=================================================================
=================================================================
*/


void r_BeginDrawCmdSubmission(struct r_draw_state_t *draw_state)
{
    struct r_draw_cmd_list_t *cmd_list;

    if(draw_state->current_draw_cmd_list == 0xffffffff)
    {
        draw_state->current_draw_cmd_list = add_stack_list_element(&draw_state->draw_cmd_lists, NULL);
        cmd_list = get_stack_list_element(&draw_state->draw_cmd_lists, draw_state->current_draw_cmd_list);
        if(!cmd_list->draw_cmds.size)
        {
            cmd_list->draw_cmds = create_list(draw_state->draw_cmd_size, 1024);
        }
    }

    memcpy(&cmd_list->inv_view_matrix, &r_view.inv_view_matrix, sizeof(mat4_t));
    mat4_t_mul(&cmd_list->view_projection_matrix, &r_view.inv_view_matrix, &r_view.projection_matrix);
    cmd_list->draw_cmds.cursor = 0;
}

void r_EndDrawCmdSubmission(struct r_draw_state_t *draw_state)
{

}


void r_BeginSubmission()
{
    r_BeginDrawCmdSubmission(&r_draw_state);
}

int32_t r_CmpDrawCmds(void *a, void *b)
{
    struct r_draw_cmd_t *cmd_a = (struct r_draw_cmd_t *)a;
    struct r_draw_cmd_t *cmd_b = (struct r_draw_cmd_t *)b;
    return cmd_a->sprite.sprite_sheet.index - cmd_b->sprite.sprite_sheet.index;
}

void r_EndSubmission()
{
    struct r_draw_cmd_list_t *cmd_list;

    if(r_draw_state.current_draw_cmd_list != 0xffffffff)
    {
        cmd_list = get_stack_list_element(&r_draw_state.draw_cmd_lists, r_draw_state.current_draw_cmd_list);
        qsort_list(&cmd_list->draw_cmds, r_CmpDrawCmds);
        if(add_ringbuffer_element(&r_draw_state.pending_draw_cmd_lists, &r_draw_state.current_draw_cmd_list) == 0xffffffff)
        {
            r_DispatchPending(&r_draw_state);
            add_ringbuffer_element(&r_draw_state.pending_draw_cmd_lists, &r_draw_state.current_draw_cmd_list);
        }
        r_draw_state.current_draw_cmd_list = 0xffffffff;
    }
}

void r_DrawAnimationFrame(struct spr_animation_h animation, vec2_t *position, float scale, float rotation, uint32_t frame, uint32_t flipped, uint32_t layer)
{
    struct spr_animation_t *animation_ptr;
    struct spr_anim_frame_t *frame_ptr;
    vec2_t final_position;
    animation_ptr = spr_GetAnimationPointer(animation);
    frame_ptr = spr_GetAnimationFrame(animation, frame);

    final_position = *position;
    if(flipped)
    {
        final_position.x -= frame_ptr->offset.x;
    }
    else
    {
        final_position.x += frame_ptr->offset.x;
    }

    final_position.y += frame_ptr->offset.y;

    r_DrawSprite(animation_ptr->sprite, &final_position, scale, rotation, frame_ptr->sprite_frame, flipped, layer);
}

void r_DrawSprite(struct spr_sprite_h sprite, vec2_t *position, float scale, float rotation, uint32_t frame, uint32_t flipped, uint32_t layer)
{
    struct r_draw_cmd_list_t *cmd_list;
    struct r_draw_cmd_t *draw_cmd;

    if(sprite.index != SPR_INVALID_SPRITE_INDEX)
    {
        cmd_list = get_stack_list_element(&r_draw_state.draw_cmd_lists, r_draw_state.current_draw_cmd_list);
        draw_cmd = get_list_element(&cmd_list->draw_cmds, add_list_element(&cmd_list->draw_cmds, NULL));

        draw_cmd->position = *position;
        draw_cmd->scale = scale;
        draw_cmd->rotation = rotation;
        draw_cmd->sprite = sprite;
        draw_cmd->frame = frame;
        draw_cmd->flipped = flipped;
        draw_cmd->layer = layer;
    }
}

void r_DispatchPending(struct r_draw_state_t *draw_state)
{
    struct r_buffer_t *vertex_buffer;
    struct r_render_pass_t *render_pass;
    struct r_framebuffer_t *framebuffer;
    VkViewport viewport;
    VkRect2D scissor;


    struct r_submit_info_t submit_info = {};
    submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.command_buffer_count = r_draw_state.pending_draw_cmd_lists.buffer_size;
    submit_info.command_buffer_count -= r_draw_state.pending_draw_cmd_lists.free_slots;
    submit_info.command_buffers = alloca(sizeof(union r_command_buffer_h) * submit_info.command_buffer_count);
    submit_info.command_buffer_count = 0;

    vertex_buffer = r_GetBufferPointer(r_vertex_buffer);
    render_pass = r_GetRenderPassPointer(r_render_pass);
    framebuffer = r_GetFramebufferPointer(r_framebuffer);

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.1;
    clear_values[0].color.float32[1] = 0.1;
    clear_values[0].color.float32[2] = 0.1;
    clear_values[0].color.float32[3] = 1.0;
    clear_values[1].depthStencil.depth = 1.0;

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass->render_pass;
    render_pass_begin_info.framebuffer = framebuffer->buffers[0];
    render_pass_begin_info.renderArea.extent.width = R_DEFAULT_WIDTH;
    render_pass_begin_info.renderArea.extent.height = R_DEFAULT_HEIGHT;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    viewport.x = 0;
    viewport.y = 0;
    viewport.width = R_DEFAULT_WIDTH;
    viewport.height = R_DEFAULT_HEIGHT;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = R_DEFAULT_WIDTH;
    scissor.extent.height = R_DEFAULT_HEIGHT;

    uint32_t *cmd_list_index;

    while((cmd_list_index = get_ringbuffer_element(&draw_state->pending_draw_cmd_lists)))
    {
        struct r_draw_cmd_list_t *cmd_list = get_stack_list_element(&r_draw_state.draw_cmd_lists, *cmd_list_index);
        union r_command_buffer_h command_buffer = r_AllocateCommandBuffer();
        r_vkBeginCommandBuffer(command_buffer);
        r_vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer->buffer, &(VkDeviceSize){0});
        r_vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        r_vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->pipeline.pipeline);
        r_vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        r_vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        struct spr_sprite_sheet_h current_sprite_sheet = SPR_INVALID_SPRITE_SHEET_HANDLE;
        uint32_t draw_cmd_index = 0;
        struct r_draw_call_data_t *current_call_data;
        draw_state->draw_calls.cursor = 0;
        while(draw_cmd_index < cmd_list->draw_cmds.cursor)
        {
            VkDescriptorSet descriptor_set = r_AllocateDescriptorSet(command_buffer, &render_pass->pipeline, VK_SHADER_STAGE_VERTEX_BIT);
            struct r_uniform_buffer_t *uniform_buffer = r_AllocateUniformBuffer(command_buffer);
            r_UpdateUniformBufferDescriptorSet(descriptor_set, 0, uniform_buffer->vk_buffer, 0, r_limits->maxUniformBufferRange);
            r_vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->pipeline.layout, 0, 1, &descriptor_set, 0, NULL);

            uint32_t draw_cmds_left = cmd_list->draw_cmds.cursor - draw_cmd_index;
            uint32_t uniform_data_index = 0;
            uint32_t uniform_data_count = r_limits->maxUniformBufferRange / sizeof(struct r_draw_uniform_data_t);

            if(uniform_data_count > draw_cmds_left)
            {
                uniform_data_count = draw_cmds_left;
            }

            while(uniform_data_index < uniform_data_count)
            {
                struct r_draw_cmd_t *draw_cmd = get_list_element(&cmd_list->draw_cmds, draw_cmd_index + uniform_data_index);
                if(draw_cmd->sprite.sprite_sheet.index != current_sprite_sheet.index)
                {
                    current_sprite_sheet = draw_cmd->sprite.sprite_sheet;
                    struct spr_sprite_sheet_t *sprite_sheet = spr_GetSpriteSheetPointer(draw_cmd->sprite.sprite_sheet);

                    current_call_data = get_list_element(&draw_state->draw_calls, add_list_element(&draw_state->draw_calls, NULL));
                    current_call_data->count = 6;
                    current_call_data->first = 0;
                    current_call_data->first_instance = draw_cmd_index + uniform_data_index;
                    current_call_data->instance_count = 0;
                    current_call_data->texture = sprite_sheet->texture;
                }

                struct spr_sprite_entry_t *entry = spr_GetSpriteEntry(draw_cmd->sprite, draw_cmd->frame);
                uint32_t offset = sizeof(struct r_draw_uniform_data_t) * (current_call_data->first_instance + current_call_data->instance_count);
                struct r_draw_uniform_data_t *draw_data = (struct r_draw_uniform_data_t *)((char *)uniform_buffer->memory + offset);

                mat4_t transform;
                mat4_t_identity(&transform);
                transform.rows[3].x = draw_cmd->position.x;
                transform.rows[3].y = draw_cmd->position.y;
                transform.rows[3].z = -(float)draw_cmd->layer;

                transform.rows[0].x *= entry->width * 0.5 * draw_cmd->scale;
                transform.rows[1].y *= entry->height * 0.5 * draw_cmd->scale;

                mat4_t_mul(&draw_data->model_view_projection_matrix, &transform, &cmd_list->view_projection_matrix);
                draw_data->offset_size.x = entry->normalized_x;
                draw_data->offset_size.y = entry->normalized_y;
                draw_data->offset_size.z = draw_cmd->flipped ? -entry->normalized_width : entry->normalized_width;
                draw_data->offset_size.w = entry->normalized_height;

                current_call_data->instance_count++;
                uniform_data_index++;
            }

            draw_cmd_index += uniform_data_count;
        }

        for(uint32_t draw_call_index = 0; draw_call_index < draw_state->draw_calls.cursor; draw_call_index++)
        {
            current_call_data = get_list_element(&draw_state->draw_calls, draw_call_index);
            VkDescriptorSet descriptor_set = r_AllocateDescriptorSet(command_buffer, &render_pass->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT);
            r_UpdateCombinedImageSamplerDescriptorSet(descriptor_set, 0, current_call_data->texture);
            r_vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->pipeline.layout, 1, 1, &descriptor_set, 0, NULL);
            r_vkCmdDraw(command_buffer, 6, current_call_data->instance_count, 0, current_call_data->first_instance);
        }

        r_vkCmdEndRenderPass(command_buffer);
        r_vkEndCommandBuffer(command_buffer);

        submit_info.command_buffers[submit_info.command_buffer_count] = command_buffer;
        submit_info.command_buffer_count++;
        remove_stack_list_element(&draw_state->draw_cmd_lists, *cmd_list_index);
    }
    r_vkResetFences(1, &r_draw_fence);
    r_vkQueueSubmit(r_draw_queue, 1, &submit_info, r_draw_fence);
    r_vkWaitForFences(1, &r_draw_fence, 1, 0xffffffffffffffff);
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

//void r_d_CmpDrawCmds(const void *a, const void *b)
//{
//    struct r_d_draw_cmd_t *cmd_a = (struct r_d_draw_cmd_t *)a;
//    struct r_d_draw_cmd_t *cmd_b = (struct r_d_draw_cmd_t *)b;
//    int32_t type_a = cmd_a->type;
//    int32_t type_b = cmd_b->type;
//    return type_a - type_b;
//}

void r_i_BeginSubmission()
{

}

void r_i_EndSubmission()
{

}

void r_i_DispatchImmediate()
{
//    struct r_d_draw_cmd_t *draw_cmd;
//    union r_command_buffer_h command_buffer;
//    struct r_framebuffer_t *framebuffer;
//    struct r_render_pass_t *render_pass;
//    struct r_buffer_t *vertex_buffer;
//    struct r_d_draw_line_data_t *line_data;
//    uint32_t offset = 0;
//    mat4_t *model_view_projection_matrix = NULL;
////    struct r_debug_vertex_t verts[2];
//    VkViewport viewport;
//    VkRect2D scissor;
//    VkRenderPassBeginInfo render_pass_begin_info = {};
//    struct r_submit_info_t submit_info = {};
//
//    framebuffer = r_GetFramebufferPointer(r_framebuffer);
//    vertex_buffer = r_GetBufferPointer(r_debug_vertex_buffer);
//
//    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//    render_pass_begin_info.framebuffer = framebuffer->buffers[0];
//    render_pass_begin_info.renderArea.extent.width = R_DEFAULT_WIDTH;
//    render_pass_begin_info.renderArea.extent.height = R_DEFAULT_HEIGHT;
//
//    viewport.minDepth = 0.0;
//    viewport.maxDepth = 1.0;
//    viewport.width = R_DEFAULT_WIDTH;
//    viewport.height = R_DEFAULT_HEIGHT;
//    viewport.x = 0;
//    viewport.y = 0;
//
//    scissor.extent.width = R_DEFAULT_WIDTH;
//    scissor.extent.height = R_DEFAULT_HEIGHT;
//    scissor.offset.x = 0;
//    scissor.offset.y = 0;
//
//    command_buffer = r_AllocateCommandBuffer();
//    r_vkBeginCommandBuffer(command_buffer);
//    r_vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer->buffer, (VkDeviceSize[]){0});
//    r_vkCmdSetViewport(command_buffer, 0, 1, &viewport);
//    r_vkCmdSetScissor(command_buffer, 0, 1, &scissor);
//
//    for(uint32_t cmd_type = R_I_DRAW_CMD_DRAW_LINE; cmd_type < R_I_DRAW_CMD_DRAW_TRI_FILL_TEXTURED; cmd_type++)
//    {
//        render_pass = r_GetRenderPassPointer(r_i_render_passes[cmd_type]);
//        render_pass_begin_info.renderPass = render_pass->render_pass;
//        r_vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
//        r_vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->pipeline.pipeline);
//
//        for(uint32_t cmd_index = 0; cmd_index < r_i_draw_cmds[cmd_type].cursor; cmd_index++)
//        {
//            draw_cmd = get_list_element(&r_i_draw_cmds[cmd_type], cmd_index);
//
//        }
//
//        r_vkCmdEndRenderPass(command_buffer);
//        r_i_draw_cmds[cmd_type].cursor = 0;
//    }
//
//
////    r_vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->pipeline.pipeline);
////
////    for(uint32_t draw_cmd_index = 0; draw_cmd_index < r_d_draw_cmds.cursor; draw_cmd_index++)
////    {
////        draw_cmd = get_list_element(&r_d_draw_cmds, draw_cmd_index);
////
////        line_data = (struct r_d_draw_line_data_t *)draw_cmd->data;
////        verts[0].position.x = line_data->from.x;
////        verts[0].position.y = line_data->from.y;
////        verts[0].position.z = line_data->from.z;
////        verts[0].position.w = 1.0;
////
////        verts[0].color.x = line_data->color.x;
////        verts[0].color.y = line_data->color.y;
////        verts[0].color.z = line_data->color.z;
////        verts[0].color.w = 1.0;
////
////        verts[1].position.x = line_data->to.x;
////        verts[1].position.y = line_data->to.y;
////        verts[1].position.z = line_data->to.z;
////        verts[1].position.w = 1.0;
////
////        verts[1].color.x = line_data->color.x;
////        verts[1].color.y = line_data->color.y;
////        verts[1].color.z = line_data->color.z;
////        verts[1].color.w = 1.0;
////
////        r_vkCmdUpdateBuffer(command_buffer, r_debug_vertex_buffer, offset, sizeof(struct r_debug_vertex_t) * 2, verts);
////        offset += sizeof(struct r_debug_vertex_t) * 2;
////
////        if(!model_view_projection_matrix)
////        {
////            model_view_projection_matrix = &draw_cmd->model_view_projection_matrix;
////        }
////    }
//
////    r_vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
//
////    if(r_d_draw_cmds.cursor)
////    {
////        r_vkCmdSetViewport(command_buffer, 0, 1, &viewport);
////        r_vkCmdSetScissor(command_buffer, 0, 1, &scissor);
////        r_vkCmdSetLineWidth(command_buffer, 1.0);
////        r_vkCmdPushConstants(command_buffer, render_pass->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), model_view_projection_matrix);
////        r_vkCmdDraw(command_buffer, r_d_draw_cmds.cursor * 2, 1, 0, 0);
////    }
////    r_vkCmdEndRenderPass(command_buffer);
//    r_vkEndCommandBuffer(command_buffer);
//
//    submit_info.s_type = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submit_info.command_buffer_count = 1;
//    submit_info.command_buffers = &command_buffer;
//    r_vkResetFences(1, &r_draw_fence);
//    r_vkQueueSubmit(r_draw_queue, 1, &submit_info, r_draw_fence);
//    r_vkWaitForFences(1, &r_draw_fence, VK_TRUE, 0xffffffffffffffff);
//
//    r_i_draw_data.cursor = 0;
}

void r_i_DrawCmd(mat4_t *model_view_projection_matrix, uint32_t type, void *data, uint32_t data_size)
{
//    struct r_i_draw_data_t *draw_data;
//    struct r_i_draw_cmd_t *draw_cmd;
//    uint32_t data_slots;
//    uint32_t available_slots;
//    if(data_size > R_I_DRAW_DATA_SIZE)
//    {
//        /* the data passed in is bigger than R_D_DRAW_DATA_SIZE, so we'll
//        need to accomodate it. */
//
//
//        /* how many slots in the list this data takes */
//        data_slots = data_size / r_i_draw_data.buffer_size;
//        /* how many slots the current buffer of the list have available */
//        available_slots = r_i_draw_data.buffer_size - r_i_draw_data.cursor % r_i_draw_data.buffer_size;
//        if(data_slots > available_slots)
//        {
//            while(available_slots)
//            {
//                /* the current buffer in the list doesn't have enough space to hold this data
//                contiguously, so we'll "pad" it until it gets to the start of the next buffer,
//                in which case it'll probably be enough. */
//                add_list_element(&r_i_draw_data, NULL);
//                available_slots--;
//            }
//        }
//    }
//
//    draw_cmd = get_list_element(&r_i_draw_cmds[type], add_list_element(&r_i_draw_cmds[type], NULL));
//    draw_data = get_list_element(&r_i_draw_data, add_list_element(&r_i_draw_data, NULL));
//
//    draw_cmd->type = type;
//    draw_cmd->data = draw_data;
//    memcpy(&draw_cmd->model_view_projection_matrix, model_view_projection_matrix, sizeof(mat4_t));
//    memcpy(draw_data, data, data_size);
}

void r_i_DrawLine(mat4_t *model_view_projection_matrix, vec3_t *from, vec3_t *to, vec3_t *color, float size)
{
//    struct r_i_draw_line_data_t line_data;
//
//    line_data.from = *from;
//    line_data.to = *to;
//    line_data.color = *color;
//    line_data.size = size;
//
//    r_i_DrawCmd(model_view_projection_matrix, R_I_DRAW_CMD_DRAW_LINE, &line_data, sizeof(struct r_i_draw_line_data_t));
}







