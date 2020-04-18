#ifndef R_H
#define R_H

#include "r_common.h"
//#include "r_vk.h"


void r_Init();

void r_InitDevice();

void r_Shutdown();



/*
=================================================================
=================================================================
=================================================================
*/

uint32_t r_MemoryIndexWithProperties(uint32_t type_bits, uint32_t properties);

uint32_t r_IsDepthStencilFormat(VkFormat format);

uint32_t r_ImageUsageFromFormat(VkFormat format);

VkImageAspectFlagBits r_GetFormatAspectFlags(VkFormat format);

uint32_t r_GetFormatPixelPitch(VkFormat format);

/*
=================================================================
=================================================================
=================================================================
*/


struct stack_list_t *r_GetHeapListFromType(uint32_t type);

struct r_heap_handle_t r_CreateHeap(uint32_t size, uint32_t type);

struct r_heap_handle_t r_CreateImageHeap(VkFormat *formats, uint32_t format_count, uint32_t size);

struct r_heap_handle_t r_CreateBufferHeap(uint32_t usage, uint32_t size);

void r_DestroyHeap(struct r_heap_handle_t handle);

struct r_heap_t *r_GetHeapPointer(struct r_heap_handle_t handle);

void r_DefragHeap(struct r_heap_handle_t handle, uint32_t move_allocs);

struct r_chunk_handle_t r_AllocChunk(struct r_heap_handle_t handle, uint32_t size, uint32_t align);

void r_FreeChunk(struct r_chunk_handle_t handle);

struct r_chunk_t *r_GetChunkPointer(struct r_chunk_handle_t handle);

//struct r_image_handle_t r_GetCompatibleStagingImage(VkImageCreateInfo *description);

//struct r_buffer_handle_t r_GetCompatibleStagingBuffer()

void r_FillImageChunk(struct r_image_handle_t handle, void *data, VkBufferImageCopy *copy);

void r_FillBufferChunk(struct r_buffer_handle_t handle, void *data, uint32_t size, uint32_t offset);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_buffer_handle_t r_CreateBuffer(VkBufferCreateInfo *description);

void r_DestroyBuffer(struct r_buffer_handle_t handle);

struct r_buffer_t *r_GetBufferPointer(struct r_buffer_handle_t handle);

//void r_FillBuffer(struct r_buffer_handle_t handle, void *data, uint32_t size, uint32_t offset);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_image_handle_t r_CreateImage(VkImageCreateInfo *description);

struct r_image_handle_t r_AllocImage(VkImageCreateInfo *description);

void r_AllocImageMemory(struct r_image_handle_t handle);

struct r_image_handle_t r_CreateImageFrom(struct r_image_t *image);

void r_DestroyImage(struct r_image_handle_t handle);

struct r_image_t *r_GetImagePointer(struct r_image_handle_t handle);

VkImageCreateInfo *r_GetImageDescriptionPointer(struct r_image_handle_t handle);

void r_BlitImage(struct r_image_handle_t src_handle, struct r_image_handle_t dst_handle, VkImageBlit *blit);

void r_CmdBlitImage(VkCommandBuffer command_buffer, struct r_image_handle_t src_handle, struct r_image_handle_t dst_handle, VkImageBlit *blit);

void r_CmdSetImageLayout(VkCommandBuffer command_buffer, struct r_image_handle_t handle, uint32_t new_layout);

//void r_FillImage(struct r_image_handle_t handle, void *data);




void r_CreateDefaultTexture();

struct r_texture_handle_t r_CreateTexture(struct r_texture_description_t *description);

void r_DestroyTexture(struct r_texture_handle_t handle);

VkSampler r_TextureSampler(struct r_sampler_params_t *params);

struct r_texture_handle_t r_LoadTexture(char *file_name, char *texture_name);

struct r_texture_t *r_GetTexturePointer(struct r_texture_handle_t handle);

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
    stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    stencolStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
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
                .color_attachment_count = 1,
                .color_attachments = (VkAttachmentReference []){
                    {.attachment = 2}
                },
                .pipeline_description = &(struct r_pipeline_description_t){
                    ...
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
        .subpasses = (struct r_subpass_description_t[]){
                {
                .color_attachment_count = 4,
                .color_attachments = (VkAttachmentReference []){
                    {.attachment = VK_ATTACHMENT_UNUSED},
                    {.attachment = VK_ATTACHMENT_UNUSED},
                    {.attachment = 2},
                    {.attachment = VK_ATTACHMENT_UNUSED},
                },
                .pipeline_description = &(struct r_pipeline_description_t ){
                    ...
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

struct r_descriptor_pool_list_t *r_GetDescriptorPoolListPointer(struct r_pipeline_t *pipeline, VkShaderStageFlagBits stage);

VkDescriptorSet r_AllocateDescriptorSet(struct r_pipeline_t *pipeline, VkShaderStageFlagBits stage, VkFence submission_fence);

void r_ResetPipelineDescriptorPools(struct r_pipeline_t *pipeline);

//r_CmdBindPipeline(VkCommandBuffer command_buffer, struct r_pipeline_t *pipeline);

//void r_CmdBeginSubpass(VkCommandBuffer command_buffer )

/*
=================================================================
=================================================================
=================================================================
*/

struct r_framebuffer_handle_t r_CreateFramebuffer(struct r_framebuffer_description_t *description);

void r_DestroyFramebuffer(struct r_framebuffer_handle_t handle);

struct r_framebuffer_t *r_GetFramebufferPointer(struct r_framebuffer_handle_t handle);

void r_PresentFramebuffer(struct r_framebuffer_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_swapchain_handle_t r_CreateSwapchain(VkSwapchainCreateInfoKHR *description);

void r_DestroySwapchain(struct r_swapchain_handle_t handle);

struct r_swapchain_t *r_GetSwapchainPointer(struct r_swapchain_handle_t handle);

void r_NextImage(struct r_swapchain_handle_t handle);

/*
=================================================================
=================================================================
=================================================================
*/


//struct r_alloc_handle_t r_Alloc(uint32_t size, uint32_t align, uint32_t index_alloc);
//
//struct r_alloc_t *r_GetAllocPointer(struct r_alloc_handle_t handle);
//
//void r_Free(struct r_alloc_handle_t handle);
//
//void r_Memcpy(struct r_alloc_handle_t handle, void *data, uint32_t size);

/*
=================================================================
=================================================================
=================================================================
*/

void r_LockQueue(struct r_queue_t *queue);

void r_UnlockQueue(struct r_queue_t *queue);

void r_QueueSubmit(struct r_queue_t *queue, uint32_t submit_count, VkSubmitInfo *submit_info, VkFence fence);

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
