#ifndef R_H
#define R_H

#include "r_defs.h"

struct r_i_vertex_t
{
    vec4_t position;
    vec4_t color;
    vec4_t tex_coords;
};

struct r_vertex_t
{
    vec4_t position;
    vec4_t normal;
    vec4_t tex_coords;
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
    float fov_y;
    float zoom;
};

struct r_cluster_t
{
    uint32_t start;
    uint32_t count;
};

struct r_cluster_light_t
{
    uint16_t light_index;
};

struct r_cluster_list_t
{
    uint32_t cluster_count;
    struct r_cluster_t *clusters;
    uint8_t width;
    uint8_t height;
    uint8_t depth;
    uint8_t align;
};

enum R_LIGHT_TYPE
{
    R_LIGHT_TYPE_POINT = 0,
    R_LIGHT_TYPE_SPOT,
    R_LIGHT_TYPE_AREA,
    R_LIGHT_TYPE_NONE,
};

struct r_light_t
{
    vec3_t position;
    vec3_t color;
    vec2_t size;
    uint32_t type;
};

struct r_light_h
{
    uint32_t index;
};

#define R_LIGHT_HANDLE(index) (struct r_light_h){index}
#define R_INVALID_LIGHT_INDEX 0xffffffff
#define R_INVALID_LIGHT_HANDLE(R_INVALID_LIGHT_INDEX)


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

struct r_portal_t
{
    
    vec3_t position;
};

struct r_portal_h
{
    uint32_t index;
};


#define R_DEFAULT_TEXTURE_INDEX 0
#define R_DEFAULT_TEXTURE_HANDLE R_TEXTURE_HANDLE(R_DEFAULT_TEXTURE_INDEX)


void r_Init();

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

void r_CreateDefaultTexture();

struct r_texture_h r_LoadTexture(char *file_name, char *texture_name);

struct r_texture_t *r_GetDefaultTexturePointer();

struct r_texture_h r_GetDefaultTextureHandle();

/*
=================================================================
=================================================================
=================================================================
*/

struct r_light_h r_CreateLight(vec3_t *position, uint32_t type);

struct r_light_h r_CreatePointLight(vec3_t *position, vec3_t *color, float radius);

struct r_light_t *r_GetLightPointer(struct r_light_h handle);

void r_DestroyLight(struct r_light_h handle);

/*
=================================================================
=================================================================
=================================================================
*/

struct r_chunk_h r_AllocVerts(uint32_t count);

void r_FillVertsChunk(struct r_chunk_h chunk, struct r_vertex_t *vertices, uint32_t count);

struct r_chunk_h r_AllocIndexes(uint32_t count);

void r_FillIndexChunk(struct r_chunk_h chunk, uint32_t *indices, uint32_t count);

#endif
