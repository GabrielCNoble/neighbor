#ifndef R_H
#define R_H

#include "r_common.h"
//#include "r_vk.h"


void r_InitRenderer();

void r_InitVulkan();



/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_MemoryTypeFromProperties(uint32_t type_bits, uint32_t properties);

/*
=================================================================
=================================================================
=================================================================
*/

//struct r_pipeline_handle_t r_CreatePipeline(struct r_pipeline_description_t* description);
//
//void r_DestroyPipeline(struct r_pipeline_handle_t handle);
//
//void r_BindPipeline(struct r_pipeline_handle_t handle);
//
//struct r_pipeline_t *r_GetPipelinePointer(struct r_pipeline_handle_t handle);


/*
=================================================================
=================================================================
=================================================================
*/

void r_CreateDefaultTexture();

struct r_texture_handle_t r_AllocTexture();

void r_FreeTexture(struct r_texture_handle_t handle);

struct r_texture_handle_t r_CreateTexture(struct r_texture_description_t *description);

VkSampler r_TextureSampler(struct r_sampler_params_t *params);

void r_SetImageLayout(VkImage image, uint32_t aspect_mask, uint32_t old_layout, uint32_t new_layout);

void r_SetTextureLayout(struct r_texture_handle_t handle, uint32_t layout);

void r_BlitImageToTexture(struct r_texture_handle_t handle, VkImage image);

void r_LoadTextureData(struct r_texture_handle_t handle, void *data);

struct r_texture_handle_t r_LoadTexture(char *file_name, char *texture_name);

struct r_texture_t *r_GetTexturePointer(struct r_texture_handle_t handle);

struct r_texture_description_t *r_GetTextureDescriptionPointer(struct r_texture_handle_t handle);

struct r_texture_t* r_GetDefaultTexturePointer();

struct r_texture_handle_t r_GetTextureHandle(char *name);


/*
=================================================================
=================================================================
=================================================================
*/

struct r_shader_handle_t r_CreateShader(struct r_shader_description_t *description);

void r_DestroyShader(struct r_shader_handle_t handle);

struct r_shader_t *r_GetShaderPointer(struct r_shader_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_IsDepthStencilFormat(VkFormat format);

/*
    The amount of stuff filled in the description may vary. To reduce the verbosity
    of Vulkan, it's possible to only pass the attachments used, and a sub pass description
    will be created automatically, which will use all attachments, and won't cause any
    image layout transitions. For example,

    struct r_render_pass_description_t description = (struct r_render_pass_description_t){
        .attachment_count = 3,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...}
        }
    };

    Is the same as

    struct r_render_pass_description description = (struct r_render_pass_description_t){
        .attachment_count = 3,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...}
        },
        .subpasses = (VkSubpassDescription []){
            .colorAttachmentCount = 3,
            .pColorAttachments = (VkAttachmentReference []){
                {.attachment = 0},
                {.attachment = 1},
                {.attachment = 2}
            }
        }
    }

    Also notice that the attachments are not being described fully here. Only their formats are
    being given. In this case, zero initialized fields will receive default values. The default
    values are as follows:

    samples = VK_SAMPLE_COUNT_1_BIT,
    loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
    storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    stencilLoadOp = VK_ATTACHMENT_LOAD_OP,
    stencolStoreOp = VK_ATTACHMENT_STORE_OP,
    initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL if color format.
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL if depth / stencil format.

    The attachment references also don't need to be fully described. Whenever layout
    is zero (VK_IMAGE_LAYOUT_UNDEFINED), it will be filled based on the format of the
    attachment it references. If the attachment is a color attachment, the layout will
    be VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. If it's a depth / stencil format,
    the value will be VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.

    In the case where sub passes are given, the pColorAttachment array will be filled with
    unused references, so it'll always have the same length as pColorAttachments. For example,

    struct r_render_pass_description_t description = (struct r_render_pass_description_t){
        .attachment_count = 4,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
        },
        .subpass_count = 1,
        .subpasses = (VkSubpassDescription []){
                {
                .colorAttachmentCount = 1,
                .pColorAttachments = (VkAttachmentReference []){
                    {.attachment = 2}
                }
            }
        }
    }

    will be the same as

    struct r_render_pass_description_t description = (struct r_render_pass_description_t){
        .attachment_count = 4,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...}
            {.format = VK_FORMAT...},
        },
        .subpass_count = 1,
        .subpasses = (VkSubpassDescription []){
                {
                .colorAttachmentCount = 4,
                .pColorAttachments = (VkAttachmentReference []){
                    {.attachment = VK_ATTACHMENT_UNUSED},
                    {.attachment = VK_ATTACHMENT_UNUSED},
                    {.attachment = 2},
                    {.attachment = VK_ATTACHMENT_UNUSED},
                }
            }
        }
    }

    This is to allow render passes created from the same set of attachments to be compatible
    with one another.

 */
struct r_render_pass_handle_t r_CreateRenderPass(struct r_render_pass_description_t *description);

void r_DestroyRenderPass(struct r_render_pass_handle_t handle);

struct r_render_pass_t *r_GetRenderPassPointer(struct r_render_pass_handle_t handle);


/*
    The purpose of a render pass set is to create a set of compatible render passes. Compatible
    render passes are required in order to use the same framebuffer to perform different passes.
    Although it's perfectly possible to create compatible render passes by just passing the same
    set of attachments to all of them, creating them through render pass sets allows for reduced
    verbosity. For instance, the creation of two compatible render passes would be something like,

    struct r_render_pass_description_t description0 = (struct r_render_pass_description_t){
        .attachment_count = 4,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT_D32_SFLOAT},
        }
    };

    struct r_render_pass_description_t description1 = (struct r_render_pass_description_t){
        .attachment_count = 4,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT_D32_SFLOAT},
        }
    }

    The above declarations are fairly straightforward, although there's a lot of repeated stuff.
    Also, this is the best case scenario, where both render passes use all the attachments, so
    the sub passes are created automatically. If one of the render passes were to not use all of the
    attachments, then describing a sub pass or sub passes would be required. For example,

    struct r_render_pass_description_t description0 = (struct r_render_pass_description_t){
        .attachment_count = 4,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT_D32_SFLOAT},
        }
    };

    struct r_render_pass_description_t description1 = (struct r_render_pass_description_t){
        .attachment_count = 4,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT_D32_SFLOAT},
        },
        .subpass_count = 1,
        .subpasses = (VkSubpassDescription []) {
                {
                .colorAttachmentCount = 1,
                .pColorAttachments = (VkAttachmentReference []){
                    {.attachment = 3}
                },
                .pDepthStencilAttachment = (VkAttachmentReference []){
                    {.attachment = 4}
                }
            }
        }
    }

    The second render pass only uses attachments 3 and 4, so a sub pass has to be
    added to describe this configuration, which makes the declaration become quite
    involved. And things can get uglier fast with this approach. However, creating
    those two render passes using a render pass set is quite cleaner.

    struct r_render_pass_set_description_t description = (struct r_render_pass_set_description_t){
        .attachment_count = 4,
        .attachments = (VkAttachmentDescription []){
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT...},
            {.format = VK_FORMAT_D32_SFLOAT},
        },
        .render_pass_count = 2,
        .render_passes = (struct r_render_pass_description_t []){
                {
                [1] = {
                    .subpasses = (VkSubpassDescription []){
                            {
                            .colorAttachmentCount = 1,
                            .pColorAttachments = (VkAttachmentReference []){
                                {.attachment = 3}
                            },
                            .pDepthStencilAttachment = (VkAttachmentReference []){
                                {.attachment = 4}
                            }
                        }
                    }
                }
            }
        }
    }

    Although the sub pass description still is necessary, there's no repeated initialization. Also,
    notice that the first render pass is not even being described. Its whole description is being
    zero initialized. In this case, the render pass will use all attachments, and will have a single
    sub pass. Also notice that subpass_count is zero initialized. This means that if this render pass
    has a something different than NULL in its subpasses field, this sub pass description will be
    describing the attachments used.
*/

struct r_render_pass_set_handle_t r_CreateRenderPassSet(struct r_render_pass_set_description_t *description);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_framebuffer_handle_t r_AllocFramebuffer();

struct r_framebuffer_handle_t r_CreateFramebuffer(struct r_framebuffer_description_t *description);

void r_DestroyFramebuffer(struct r_framebuffer_handle_t handle);

struct r_framebuffer_t *r_GetFramebufferPointer(struct r_framebuffer_handle_t handle);

struct r_framebuffer_description_t *r_GetFramebufferDescriptionPointer(struct r_framebuffer_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_alloc_handle_t r_Alloc(uint32_t size, uint32_t align, uint32_t index_alloc);

struct r_alloc_t *r_GetAllocPointer(struct r_alloc_handle_t handle);

void r_Free(struct r_alloc_handle_t handle);

void r_Memcpy(struct r_alloc_handle_t handle, void *data, uint32_t size);

/*
=================================================================
=================================================================
=================================================================
*/

void r_SetZPlanes(float z_near, float z_far);

void r_SetFovY(float fov_y);

void r_RecomputeViewProjectionMatrix();

void r_SetViewMatrix(mat4_t *view_matrix);

void r_GetWindowSize(uint32_t *width, uint32_t *height);


/*
=================================================================
=================================================================
=================================================================
*/

struct r_material_handle_t r_AllocMaterial();

void r_FreeMaterial(struct r_material_handle_t handle);

struct r_material_t *r_GetMaterialPointer(struct r_material_handle_t handle);

struct r_material_handle_t r_GetMaterialHandle(char *name);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_light_handle_t r_CreateLight(vec3_t *position, float radius, vec3_t *color);

struct r_light_t *r_GetLightPointer(struct r_light_handle_t handle);

void r_DestroyLight(struct r_light_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

void *r_AllocCmdData(uint32_t size);

void *r_AtomicAllocCmdData(uint32_t size);

void r_BeginSubmission(mat4_t *view_projection_matrix, mat4_t *view_matrix);

void r_SubmitDrawCmd(mat4_t *model_matrix, struct r_material_t *material, uint32_t start, uint32_t count);

void r_EndSubmission();

// void r_SortDrawCmds(struct r_udraw_cmd_buffer_t *udraw_cmd_buffer);

void r_QueueCmd(uint32_t type, void *data, uint32_t data_size);

struct r_cmd_t *r_NextCmd();

void r_AdvanceCmd();

int r_ExecuteCmds(void *data);

void r_WaitEmptyQueue();

void r_Draw(struct r_cmd_t *cmd);

/*
=================================================================================
=================================================================================
=================================================================================
*/

void r_DrawPoint(vec3_t* position, vec3_t* color);



#endif
