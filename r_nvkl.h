#ifndef R_H
#define R_H

#include "r_common.h"

VkInstance r_CreateInstance();

VkInstance r_GetInstance();

void r_DestroyInstance();

VkDevice r_CreateDevice(VkSurfaceKHR surface);

VkDevice r_GetDevice();

void r_DestroyDevice();

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

struct r_heap_h r_CreateHeap(uint32_t size, uint32_t type);

struct r_heap_h r_CreateImageHeap(VkFormat *formats, uint32_t format_count, uint32_t size);

struct r_heap_h r_CreateBufferHeap(uint32_t usage, uint32_t size);

void r_DestroyHeap(struct r_heap_h handle);

struct r_heap_t *r_GetHeapPointer(struct r_heap_h handle);

void r_DefragHeap(struct r_heap_h handle);

//void r_DefragImageHeap(struct r_heap_h handle);

//void r_DefragBufferHeap(struct r_heap_h handle);

struct r_chunk_h r_AllocChunk(struct r_heap_h handle, uint32_t size, uint32_t align);

void r_FreeChunk(struct r_chunk_h handle);

struct r_chunk_t *r_GetChunkPointer(struct r_chunk_h handle);

void *r_GetBufferChunkMappedMemory(struct r_chunk_h handle);

void r_FillImageChunk(struct r_image_handle_t handle, void *data, VkBufferImageCopy *copy);

void r_FillBufferChunk(struct r_chunk_h handle, void *data, uint32_t size, uint32_t offset);

struct r_staging_buffer_t *r_AllocateStagingBuffer(union r_command_buffer_h command_buffer);

void r_FreeStagingBuffer(struct r_staging_buffer_t *buffer);

/*
=================================================================
=================================================================
=================================================================
*/

union r_command_buffer_h r_AllocateCommandBuffer();

struct r_command_buffer_t *r_GetCommandBufferPointer(union r_command_buffer_h command_buffer);

void r_MarkCommandBufferAsPending(VkCommandBuffer command_buffer);



/*
=================================================================
=================================================================
=================================================================
*/

struct r_buffer_h r_CreateBuffer(VkBufferCreateInfo *description);

void r_DestroyBuffer(struct r_buffer_h handle);

struct r_buffer_t *r_GetBufferPointer(struct r_buffer_h handle);

void r_FillBuffer(struct r_buffer_h handle, void *data, uint32_t size, uint32_t offset);

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

void r_SetImageLayout(struct r_image_handle_t image, VkImageLayout new_layout);

void *r_MapImageMemory(struct r_image_handle_t handle);



void r_CreateDefaultTexture();

struct r_texture_h r_CreateTexture(struct r_texture_description_t *description);

void r_DestroyTexture(struct r_texture_h handle);

VkSampler r_TextureSampler(struct r_sampler_params_t *params);

struct r_texture_h r_LoadTexture(char *file_name, char *texture_name);

struct r_texture_t *r_GetTexturePointer(struct r_texture_h handle);

struct r_texture_t *r_GetDefaultTexturePointer();

struct r_texture_h r_GetDefaultTextureHandle();

struct r_texture_h r_GetTextureHandle(char *name);


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

struct r_render_pass_handle_t r_CreateRenderPass(struct r_render_pass_description_t *description);

void r_DestroyRenderPass(struct r_render_pass_handle_t handle);

struct r_render_pass_t *r_GetRenderPassPointer(struct r_render_pass_handle_t handle);

struct r_descriptor_pool_list_t *r_GetDescriptorPoolListPointer(struct r_pipeline_t *pipeline, VkShaderStageFlagBits stage);

VkDescriptorSet r_AllocateDescriptorSet(union r_command_buffer_h command_buffer, struct r_pipeline_t *pipeline, VkShaderStageFlagBits stage);

struct r_pipeline_t *r_GetPipelinePointerByState(struct r_render_pass_handle_t render_pass_handle, uint32_t subpass_index, struct r_pipeline_state_t *pipeline_state);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_pipeline_h r_CreatePipeline(struct r_pipeline_description_t *description);

struct r_pipeline_t *r_GetPipelinePointer(struct r_pipeline_h pipeline_handle);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_framebuffer_h r_CreateFramebuffer(struct r_framebuffer_description_t *description);

void r_InitializeFramebuffer(struct r_framebuffer_t *framebuffer, struct r_framebuffer_description_t *description);

void r_DestroyFramebuffer(struct r_framebuffer_h handle);

struct r_framebuffer_t *r_GetFramebufferPointer(struct r_framebuffer_h handle);

struct r_framebuffer_description_t *r_GetFramebufferDescriptionPointer(struct r_framebuffer_h handle);

void r_ResizeFramebuffer(struct r_framebuffer_h handle, uint32_t width, uint32_t height);

void r_PresentFramebuffer(struct r_framebuffer_h handle);

//void r_ClearFramebuffer(struct r_framebuffer_h handle, )

/*
=================================================================
=================================================================
=================================================================
*/

void r_SetSwapchainSurface(VkSurfaceKHR surface);

uint32_t r_NextSwapchainImage();

/*
=================================================================
=================================================================
=================================================================
*/

void r_LockQueue(struct r_queue_t *queue);

void r_UnlockQueue(struct r_queue_t *queue);

VkQueue r_GetDrawQueue();

VkPhysicalDeviceLimits *r_GetDeviceLimits();

/*
=================================================================
=================================================================
=================================================================
*/

VkFence r_CreateFence();

VkEvent r_CreateEvent();

/*
=================================================================
=================================================================
=================================================================
*/

struct r_light_handle_t r_CreateLight(vec3_t *position, float radius, vec3_t *color);

struct r_light_t *r_GetLightPointer(struct r_light_handle_t handle);

void r_DestroyLight(struct r_light_handle_t handle);

/*
=================================================================================
=================================================================================
=================================================================================
*/

void r_vkBeginCommandBuffer(union r_command_buffer_h command_buffer);

void r_AppendEvent(union r_command_buffer_h command_buffer, VkEvent event);

void r_vkCmdBindPipeline(union r_command_buffer_h command_buffer, VkPipelineBindPoint bind_point, VkPipeline pipeline);

void r_vkCmdBindVertexBuffers(union r_command_buffer_h command_buffer, uint32_t first_binding, uint32_t binding_count, VkBuffer *buffers, VkDeviceSize *offsets);

void r_vkCmdBindIndexBuffer(union r_command_buffer_h command_buffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType index_type);

void r_vkCmdBeginRenderPass(union r_command_buffer_h command_buffer, struct r_render_pass_begin_info_t *begin_info, VkSubpassContents subpass_contents);

void r_vkCmdEndRenderPass(union r_command_buffer_h command_buffer);

void r_vkCmdSetViewport(union r_command_buffer_h command_buffer, uint32_t first_viewport, uint32_t viewport_count, VkViewport *viewports);

void r_vkCmdSetScissor(union r_command_buffer_h command_buffer, uint32_t first_scissor, uint32_t scissor_count, VkRect2D *scissors);

void r_vkCmdSetLineWidth(union r_command_buffer_h command_buffer, float width);

void r_vkCmdSetPointSize(union r_command_buffer_h command_buffer, float size);

void r_vkCmdPushConstants(union r_command_buffer_h command_buffer, VkPipelineLayout layout, VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size, void *data);

void r_vkCmdDraw(union r_command_buffer_h command_buffer, uint32_t count, uint32_t instance_count, uint32_t first, uint32_t first_instance);

void r_vkCmdDrawIndexed(union r_command_buffer_h command_buffer, uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);

void r_vkCmdCopyBufferToImage(union r_command_buffer_h command_buffer, VkBuffer src_buffer, VkImage dst_image, VkImageLayout dst_layout, uint32_t region_count, VkBufferImageCopy *regions);

void r_vkCmdBindDescriptorSets(union r_command_buffer_h command_buffer, VkPipelineBindPoint bind_point, VkPipelineLayout layout, uint32_t first_set, uint32_t set_count, VkDescriptorSet *descriptor_sets, uint32_t dynamic_offset_count, uint32_t *dynamic_offsets);

void r_vkCmdBlitImage(union r_command_buffer_h command_buffer, struct r_image_handle_t src_handle, struct r_image_handle_t dst_handle, VkImageBlit *blit);

void r_vkCmdPipelineBarrier(union r_command_buffer_h command_buffer, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask, VkDependencyFlags dependency_flags,
                            uint32_t memory_barrier_count, VkMemoryBarrier *memory_barriers,
                            uint32_t buffer_barrier_count, VkBufferMemoryBarrier *buffer_memory_barriers,
                            uint32_t image_barrier_count, VkImageMemoryBarrier *image_memory_barriers);
                            
void r_vkCmdPipelineImageBarrier(union r_command_buffer_h command_buffer, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask, VkDependencyFlags dependency_flags, uint32_t barrier_count, VkImageMemoryBarrier *barriers);

void r_vkCmdSetImageLayout(union r_command_buffer_h command_buffer, struct r_image_handle_t handle, uint32_t new_layout);

void r_vkCmdClearAttachments(union r_command_buffer_h command_buffer, uint32_t attachment_count, VkClearAttachment *attachments, uint32_t rect_count, VkClearRect *rects);

void r_vkCmdUpdateBuffer(union r_command_buffer_h command_buffer, struct r_buffer_h buffer, uint32_t offset, uint32_t size, void *data);

void r_vkUpdateDescriptorSets(uint32_t descriptor_write_count, VkWriteDescriptorSet *descriptor_writes);

void r_UpdateUniformBufferDescriptorSet(VkDescriptorSet descriptor_set, uint32_t dst_binding, VkBuffer uniform_buffer, uint32_t offset, uint32_t range);

void r_UpdateCombinedImageSamplerDescriptorSet(VkDescriptorSet descriptor_set, uint32_t dst_binding, struct r_texture_h texture);

void r_vkEndCommandBuffer(union r_command_buffer_h command_buffer);

VkResult r_vkQueueSubmit(VkQueue queue, uint32_t submit_count, struct r_submit_info_t *submit_info, VkFence fence);

void r_vkResetFences(uint32_t fence_count, VkFence *fences);

void r_vkWaitForFences(uint32_t fence_count, VkFence *fences, VkBool32 wait_all, uint64_t time_out);

VkResult r_vkGetEventStatus(VkEvent event);

void r_vkSetEvent(VkEvent event);

void r_vkResetEvent(VkEvent event);

void r_vkMapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **data);


#endif
