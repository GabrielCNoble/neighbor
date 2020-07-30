#ifndef R_DRAW_H
#define R_DRAW_H

#include "r_nvkl.h"
#include "lib/dstuff/ds_matrix.h"
#include "lib/dstuff/ds_list.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_ringbuffer.h"
#include "spr.h"
#include "mdl.h"

struct r_material_t
{
    char *name;
    struct r_texture_h diffuse_texture;
    struct r_texture_h normal_texture;
};

struct r_material_h
{
    uint32_t index;
};

#define R_DEFAULT_MATERIAL_INDEX 0x00000000
#define R_INVALID_MATERIAL_INDEX 0xffffffff
#define R_MATERIAL_HANDLE(index) (struct r_material_h){index}
#define R_INVALID_MATERIAL_HANDLE R_MATERIAL_HANDLE(R_INVALID_MATERIAL_INDEX)

struct r_submission_state_t
{
    struct stack_list_t draw_cmd_lists;
    struct list_t pending_draw_cmd_lists;
    uint32_t current_draw_cmd_list;
    uint32_t draw_cmd_size;
};

struct r_d_submission_state_t
{
    struct r_submission_state_t base;
    struct list_t draw_calls;
};

struct r_i_draw_state_t
{
    float point_size;
    float line_width;
    struct r_pipeline_state_t pipeline_state;
    struct r_texture_h texture;
    VkRect2D scissor;
};

struct r_i_submission_state_t
{
    struct r_submission_state_t base;
    struct list_t cmd_data;
    uint32_t current_pipeline;
    uint32_t vertex_cursor;
    uint32_t index_cursor;
    struct r_i_draw_state_t current_draw_state;
    struct r_i_draw_state_t next_draw_state;
};

struct r_view_t
{
    mat4_t view_transform;
    mat4_t projection_matrix;
    mat4_t inv_view_matrix;
    VkViewport viewport;
    VkRect2D scissor;
    float z_near;
    float z_far;
    float zoom;
};

struct r_i_vertex_t
{
    vec4_t position;
    vec4_t tex_coords;
    vec4_t color;
};

struct r_vertex_t
{
    vec4_t position;
    vec4_t normal;
    vec4_t tex_coords;
};

#define R_I_DRAW_DATA_SLOT_SIZE 64

struct r_i_draw_data_slot_t
{
    uint8_t data[R_I_DRAW_DATA_SLOT_SIZE];
};

struct r_i_upload_buffer_data_t
{
    uint32_t start;
    uint32_t count;
    uint32_t size;
    uint32_t index_data;
    void *data;
};

struct r_i_set_draw_state_data_t
{
    struct r_i_draw_state_t draw_state;
};

enum R_I_DRAW_CMDS
{
    R_I_DRAW_CMD_DRAW,
    R_I_DRAW_CMD_DRAW_LAST,
    R_I_DRAW_CMD_SET_DRAW_STATE,
    R_I_DRAW_CMD_UPLOAD_DATA,
    R_I_DRAW_CMD_LAST,
};

struct r_i_draw_cmd_t
{
    uint32_t type;
    void *data;
};

struct r_i_draw_cmd_data_t
{
    uint32_t index_start;
    uint32_t vertex_start;
    uint32_t count;
    uint32_t indexed;
};

#define R_MAX_SPRITE_LAYER 4

struct r_begin_submission_info_t
{
    mat4_t inv_view_matrix;
    mat4_t projection_matrix;
    struct r_framebuffer_h framebuffer;
    uint32_t clear_framebuffer;
    VkViewport viewport;
    VkRect2D scissor;
};

struct r_draw_cmd_t
{
    uint32_t start;
    uint32_t count;
    mat4_t transform;
    struct r_material_t *material;
};

struct r_draw_cmd_list_t
{
    struct r_begin_submission_info_t begin_info;
    mat4_t view_projection_matrix;
    struct list_t draw_cmds;
};

struct r_draw_uniform_data_t
{
    /* data that's written to the uniform buffer */
    mat4_t model_view_projection_matrix;
    vec4_t offset_size;
};

struct r_uniform_buffer_t
{
    struct r_buffer_h buffer;
    VkBuffer vk_buffer;
    void *memory;
    VkEvent event;
};

struct r_draw_call_data_t
{
    uint32_t first;
    uint32_t count;
    uint32_t first_instance;
    uint32_t instance_count;
    struct r_texture_h texture;
};


void r_DrawInit();

void r_DrawInitImmediate();

void r_DrawShutdown();

void r_BeginFrame();

void r_EndFrame();

void r_RecomputeInvViewMatrix();

void r_RecomputeProjectionMatrix();

struct r_view_t *r_GetViewPointer();

void r_SetWindowSize(uint32_t width, uint32_t height);

void r_Fullscreen(uint32_t enable);

struct r_framebuffer_h r_CreateDrawableFramebuffer(uint32_t width, uint32_t height);

struct r_framebuffer_h r_GetBackbufferHandle();

/*
=================================================================
=================================================================
=================================================================
*/

void r_CreateDefaultMaterial();

struct r_material_h r_CreateMaterial();

void r_DestroyMaterial(struct r_material_h material);

struct r_material_t *r_GetMaterialPointer(struct r_material_h handle);

struct r_material_h r_GetMaterialHandle(char *name);

struct r_material_h r_GetDefaultMaterialHandle();

struct r_material_t *r_GetDefaultMaterialPointer();

/*
=================================================================
=================================================================
=================================================================
*/

struct r_chunk_h r_AllocVerts(uint32_t count);

struct r_chunk_h r_AllocIndexes(uint32_t count);

/*
=================================================================
=================================================================
=================================================================
*/

void r_BeginDrawCmdSubmission(struct r_submission_state_t *submission_state, struct r_begin_submission_info_t *begin_info);

uint32_t r_EndDrawCmdSubmission(struct r_submission_state_t *submission_state);

void r_BeginSubmission(struct r_begin_submission_info_t *begin_info);

void r_EndSubmission();

void r_Draw(uint32_t start, uint32_t count, struct r_material_t *material, mat4_t *transform);

void r_DispatchPending();

struct r_uniform_buffer_t *r_AllocateUniformBuffer(union r_command_buffer_h command_buffer);

/*
=================================================================
=================================================================
=================================================================
*/


void r_i_BeginSubmission(struct r_begin_submission_info_t *begin_info);

void r_i_EndSubmission();

void r_i_DispatchPending();

void r_i_SetDrawState(struct r_i_draw_state_t *draw_state);

void r_i_SetDepthWrite(uint32_t enable);

void r_i_SetDepthTest(uint32_t enable);

void r_i_SetDepthCompareOp(VkCompareOp op);

void r_i_SetPrimitiveTopology(VkPrimitiveTopology topology);

void r_i_SetPolygonMode(VkPolygonMode polygon_mode);

void r_i_SetCullMode(VkCullModeFlags cull_mode);

void r_i_SetTexture(struct r_texture_h texture);

void r_i_SetScissor(uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height);

void r_i_SetLineWidth(float width);

void r_i_SetPointSize(float size);


void *r_i_AllocateDrawCmdData(uint32_t size);

void r_i_SubmitCmd(uint32_t type, void *data);

uint32_t r_i_UploadVertices(struct r_i_vertex_t *vertices, uint32_t vert_count);

uint32_t r_i_UploadIndices(uint32_t *indices, uint32_t count);

uint32_t r_i_UploadBufferData(void *data, uint32_t size, uint32_t count, uint32_t index_data);

void r_i_Draw(uint32_t vertex_start, uint32_t index_start, uint32_t count, uint32_t indexed);

void r_i_DrawImmediate(struct r_i_vertex_t *vertices, uint32_t vertex_count, uint32_t *indices, uint32_t index_count);

void r_i_DrawLines(uint32_t first_vertex, uint32_t first_index, uint32_t count, uint32_t indexed, uint32_t line_strip);

void r_i_DrawLinesImmediate(struct r_i_vertex_t *verts, uint32_t vert_count, float size, uint32_t line_strip);

void r_i_DrawLineStrip(struct r_i_vertex_t *verts, uint32_t vert_count, float size);

void r_i_DrawTris(uint32_t first_vertex, uint32_t first_index, uint32_t count, uint32_t indexed, uint32_t fill);

void r_i_DrawTrisImmediate(struct r_i_vertex_t *verts, uint32_t vert_count, uint32_t *indices, uint32_t indice_count, uint32_t fill);

void r_i_DrawTriLine(struct r_i_vertex_t *verts);

void r_i_DrawTriFill(struct r_i_vertex_t *verts);

void r_i_DrawPoints(uint32_t first_vertex, uint32_t count, float size);

void r_i_DrawPoint(vec3_t *point, vec3_t *color, float size);




#endif // R_DRAW_H
