#include "r_draw.h"
#include "spr.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/containers/ringbuffer.h"
#include <stdlib.h>
#include <stdio.h>

struct r_buffer_handle_t r_uniform_buffer;
struct r_draw_cmd_buffer_t *r_draw_cmd_buffer;
struct stack_list_t r_draw_cmd_buffers;
struct ringbuffer_t r_pending_draw_cmd_buffers;
uint32_t r_current_cmd_buffer = 0xffffffff;

struct r_render_pass_handle_t r_render_pass;
struct r_framebuffer_handle_t r_framebuffer;
struct r_buffer_handle_t r_vertex_buffer;
VkQueue r_draw_queue;
VkFence r_draw_fence;


//VkCommandPool r_command_pool;
//VkCommandBuffer r_command_buffer;

void r_DrawInit()
{
    FILE *file;
    struct r_shader_description_t vertex_description = {};
    struct r_shader_description_t fragment_description = {};
    struct r_render_pass_description_t render_pass_description = {};
    struct r_buffer_t *buffer;

    file = fopen("shaders/shader.vert.spv", "rb");
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
    vertex_description.resources = (struct r_resource_binding_t []){
        [0] = {
            .descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .count = 1
        }
    };
    vertex_description.push_constant_count = 1;
    vertex_description.push_constants = (struct r_push_constant_t []){
        [0] = {
            .offset = 0,
            .size = sizeof(mat4_t)
        },
    };

    struct r_shader_t *vertex_shader = r_GetShaderPointer(r_CreateShader(&vertex_description));
    free(vertex_description.code);

    fopen("shaders/shader.frag.spv", "rb");
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
    struct r_shader_t *fragment_shader = r_GetShaderPointer(r_CreateShader(&fragment_description));
    free(fragment_description.code);

    render_pass_description = (struct r_render_pass_description_t){
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT_R32G32B32A32_SFLOAT, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR},
//            {.format = VK_FORMAT_R32G32B32A32_SFLOAT},
//            {.format = VK_FORMAT_R32G32B32A32_SFLOAT},
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
                .pipeline_description = &(struct r_pipeline_description_t){
                    .shader_count = 2,
                    .shaders = (struct r_shader_t *[]){
                        vertex_shader, fragment_shader
                    },
                    .vertex_input_state = &(VkPipelineVertexInputStateCreateInfo){
                        .vertexBindingDescriptionCount = 1,
                        .pVertexBindingDescriptions = (VkVertexInputBindingDescription []){
                            {.stride = sizeof(struct vertex_t)}
                        },
                        .vertexAttributeDescriptionCount = 3,
                        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription []){
                            {
                                .location = 0,
                                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                .offset = offsetof(struct vertex_t, position)
                            },
                            {
                                .location = 1,
                                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                .offset = offsetof(struct vertex_t, normal)
                            },
                            {
                                .location = 2,
                                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                .offset = offsetof(struct vertex_t, tex_coords)
                            }
                        }
                    }
                },
            }
        },
    };

    r_render_pass = r_CreateRenderPass(&render_pass_description);

    struct r_framebuffer_description_t framebuffer_description = (struct r_framebuffer_description_t){
        .attachment_count = render_pass_description.attachment_count,
        .attachments = render_pass_description.attachments,
        .frame_count = 2,
        .width = 800,
        .height = 600,
        .render_pass = r_GetRenderPassPointer(r_render_pass)
    };

    r_framebuffer = r_CreateFramebuffer(&framebuffer_description);
    r_draw_queue = r_GetDrawQueue();
    r_draw_fence = r_CreateFence();

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.size = R_DRAW_CMD_BUFFER_DRAW_CMDS * sizeof(struct r_draw_data_t);
    buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    r_uniform_buffer = r_CreateBuffer(&buffer_create_info);

    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_create_info.size = 8388608;
    r_vertex_buffer = r_CreateBuffer(&buffer_create_info);
//    buffer = r_GetBufferPointer(r_vertex_buffer);

    struct vertex_t *data = (struct vertex_t []){
        {.position = (vec4_t){-50.0, -50.0, -1.2, 1.0},.tex_coords = (vec4_t){0.0, 1.0, 0.0, 0.0}},
        {.position = (vec4_t){50.0, -50.0, -1.2, 1.0},.tex_coords = (vec4_t){1.0, 1.0, 0.0, 0.0}},
        {.position = (vec4_t){50.0, 50.0, -1.2, 1.0},.tex_coords = (vec4_t){1.0, 0.0, 0.0, 0.0}},


        {.position = (vec4_t){50.0, 50.0, -1.2, 1.0},.tex_coords = (vec4_t){1.0, 0.0, 0.0, 0.0}},
        {.position = (vec4_t){-50.0, 50.0, -1.2, 1.0},.tex_coords = (vec4_t){0.0, 0.0, 0.0, 0.0}},
        {.position = (vec4_t){-50.0, -50.0, -1.2, 1.0},.tex_coords = (vec4_t){0.0, 1.0, 0.0, 0.0}},
    };

    r_FillBufferChunk(r_vertex_buffer, data, sizeof(struct vertex_t) * 6, 0);

    r_draw_cmd_buffers = create_stack_list(sizeof(struct r_draw_cmd_buffer_t), 32);
    r_pending_draw_cmd_buffers = create_ringbuffer(sizeof(uint32_t), 32);
}

void r_DrawShutdown()
{

}

void r_BeginFrame()
{

}

void r_EndFrame()
{
    r_DispatchPending();
    r_PresentFramebuffer(r_framebuffer);
}

/*
=================================================================
=================================================================
=================================================================
*/

void r_BeginSubmission(mat4_t *view_matrix, mat4_t *projection_matrix)
{
    struct r_draw_cmd_buffer_t *cmd_buffer;

    if(r_current_cmd_buffer == 0xffffffff)
    {
        r_current_cmd_buffer = add_stack_list_element(&r_draw_cmd_buffers, NULL);
        cmd_buffer = get_stack_list_element(&r_draw_cmd_buffers, r_current_cmd_buffer);
    }

    if(view_matrix && projection_matrix)
    {
        memcpy(&cmd_buffer->view_matrix, view_matrix, sizeof(mat4_t));
        mat4_t_mul(&cmd_buffer->view_projection_matrix, view_matrix, projection_matrix);
    }

    cmd_buffer->draw_cmd_count = 0;
}

void r_DrawSprite(struct spr_sprite_h sprite, vec2_t *position, float scale, float rotation, uint32_t frame)
{
    struct r_draw_cmd_buffer_t *cmd_buffer;
    struct r_draw_cmd_t *draw_cmd;

    if(sprite.index != SPR_INVALID_SPRITE_INDEX)
    {
        cmd_buffer = get_stack_list_element(&r_draw_cmd_buffers, r_current_cmd_buffer);
        draw_cmd = cmd_buffer->draw_cmds + cmd_buffer->draw_cmd_count;
        cmd_buffer->draw_cmd_count++;

        draw_cmd->position = *position;
        draw_cmd->scale = scale;
        draw_cmd->rotation = rotation;
        draw_cmd->sprite = sprite;
        draw_cmd->frame = frame;
    }
}

void r_CmpDrawCmds(struct r_draw_cmd_t *a, struct r_draw_cmd_t *b)
{
    return a->sprite.sprite_sheet.index - b->sprite.sprite_sheet.index;
}

void r_EndSubmission()
{
    struct r_draw_cmd_buffer_t *cmd_buffer;

    if(r_current_cmd_buffer != 0xffffffff)
    {
        cmd_buffer = get_stack_list_element(&r_draw_cmd_buffers, r_current_cmd_buffer);
        qsort(cmd_buffer->draw_cmds, cmd_buffer->draw_cmd_count, sizeof(struct r_draw_cmd_t), r_CmpDrawCmds);
        if(add_ringbuffer_element(&r_pending_draw_cmd_buffers, &r_current_cmd_buffer) == 0xffffffff)
        {
            r_DispatchPending();
            add_ringbuffer_element(&r_pending_draw_cmd_buffers, &r_current_cmd_buffer);
        }
        r_current_cmd_buffer = 0xffffffff;
    }
}

void r_DispatchPending()
{
    struct r_draw_cmd_buffer_t *cmd_buffer;
    struct r_draw_cmd_t *draw_cmd;
    uint32_t *cmd_buffer_index;
    struct r_buffer_t *vertex_buffer;
    struct r_render_pass_t *render_pass;
    struct r_framebuffer_t *framebuffer;
    struct r_texture_t *texture;
    struct spr_sprite_sheet_t *sprite_sheet;
    struct spr_sprite_sheet_h current_sprite_sheet;
    void *staging_memory;
    VkBuffer staging_buffer;
    VkViewport viewport;
    VkRect2D scissor;
    VkDescriptorSet descriptor_set;

    mat4_t model_view_projection_matrix;
    mat4_t transform;

    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkCommandBuffer command_buffer;
    VkCommandBufferBeginInfo command_buffer_begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VkRenderPassBeginInfo render_pass_begin_info = {};
    VkWriteDescriptorSet write_descriptor_set = {};
    VkDescriptorImageInfo image_info = {};

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = r_pending_draw_cmd_buffers.buffer_size;
    submit_info.commandBufferCount -= r_pending_draw_cmd_buffers.free_slots;
    submit_info.pCommandBuffers = alloca(sizeof(VkCommandBuffer) * submit_info.commandBufferCount);
    submit_info.commandBufferCount = 0;

    vertex_buffer = r_GetBufferPointer(r_vertex_buffer);
    render_pass = r_GetRenderPassPointer(r_render_pass);
    framebuffer = r_GetFramebufferPointer(r_framebuffer);

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.1;
    clear_values[0].color.float32[1] = 0.1;
    clear_values[0].color.float32[2] = 0.1;
    clear_values[0].color.float32[3] = 1.0;
    clear_values[1].depthStencil.depth = 1.0;

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

    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    write_descriptor_set.pImageInfo = &image_info;

    staging_buffer = r_GetStagingBuffer();
    staging_memory = r_LockStagingMemory();
    r_UnlockStagingMemory();

    current_sprite_sheet = SPR_INVALID_SPRITE_SHEET_HANDLE;

    while((cmd_buffer_index = get_ringbuffer_element(&r_pending_draw_cmd_buffers)))
    {
        cmd_buffer = get_stack_list_element(&r_draw_cmd_buffers, *cmd_buffer_index);
        command_buffer = r_AllocateCommandBuffer();
        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->pipeline.pipeline);
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer->buffer, &(VkDeviceSize){0});
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        for(uint32_t draw_cmd_index = 0; draw_cmd_index < cmd_buffer->draw_cmd_count; draw_cmd_index++)
        {
            draw_cmd = cmd_buffer->draw_cmds + draw_cmd_index;

            if(draw_cmd->sprite.sprite_sheet.index != current_sprite_sheet.index)
            {
                current_sprite_sheet = draw_cmd->sprite.sprite_sheet;
                sprite_sheet = spr_GetSpriteSheetPointer(draw_cmd->sprite.sprite_sheet);
                texture = r_GetTexturePointer(sprite_sheet->texture);

                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.sampler = texture->sampler;
                image_info.imageView = texture->image_view;

                descriptor_set = r_AllocateDescriptorSet(&render_pass->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, r_draw_fence);
                write_descriptor_set.dstSet = descriptor_set;
                r_UpdateDescriptorSets(1, &write_descriptor_set);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pass->pipeline.layout, 0, 1, &descriptor_set, 0, NULL);
            }

            mat4_t_identity(&transform);
            transform.rows[3].x = draw_cmd->position.x;
            transform.rows[3].y = draw_cmd->position.y;

            mat4_t_mul(&model_view_projection_matrix, &transform, &cmd_buffer->view_projection_matrix);
            vkCmdPushConstants(command_buffer, render_pass->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4_t), &model_view_projection_matrix);
            vkCmdDraw(command_buffer, 6, 1, 0, 0);
        }
        vkCmdEndRenderPass(command_buffer);
        vkEndCommandBuffer(command_buffer);
        ((VkCommandBuffer *)submit_info.pCommandBuffers)[submit_info.commandBufferCount] = command_buffer;
        submit_info.commandBufferCount++;
        remove_stack_list_element(&r_draw_cmd_buffers, *cmd_buffer_index);
    }
    r_ResetFences(1, &r_draw_fence);
    r_QueueSubmit(r_draw_queue, 1, &submit_info, r_draw_fence);
    r_WaitForFences(1, &r_draw_fence, 1, 0xffffffffffffffff);
}


