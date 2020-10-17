#include "r.h"
#include "r_nvkl.h"
#include "lib/stb/stb_image.h"
#include "lib/dstuff/ds_mem.h"
#include "lib/dstuff/ds_path.h"

struct stack_list_t r_materials;
struct stack_list_t r_lights;

void r_Init()
{
    r_materials = create_stack_list(sizeof(struct r_material_t), 128);
    r_lights = create_stack_list(sizeof(struct r_light_t), 128);
    
//    r_CreateDefaultMaterial();
//    r_CreateDefaultTexture();
}

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

void r_CreateDefaultTexture()
{
    struct r_texture_h default_texture;
    struct r_texture_description_t description = {};;
    struct r_texture_t *texture;
    uint32_t *default_texture_pixels;
    uint32_t pixel_pitch;
//    uint32_t row_pitch;

    description.extent.width = 8;
    description.extent.height = 8;
    description.extent.depth = 1;
    description.image_type = VK_IMAGE_TYPE_2D;
    description.name = "default_texture";
    description.format = VK_FORMAT_R8G8B8A8_UNORM;
    description.sampler_params.min_filter = VK_FILTER_NEAREST;
    description.sampler_params.mag_filter = VK_FILTER_NEAREST;
    description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    default_texture = r_CreateTexture(&description);
    pixel_pitch = r_GetFormatPixelPitch(description.format);
//    row_pitch = pixel_pitch * description.extent.width;
    default_texture_pixels = mem_Calloc(pixel_pitch, description.extent.width * description.extent.height);
    texture = r_GetTexturePointer(default_texture);
    for(uint32_t y = 0; y < description.extent.height; y++)
    {
        for(uint32_t x = 0; x < description.extent.width; x++)
        {
            default_texture_pixels[y * description.extent.width + x] = ((x + y) % 2) ? 0xff111111 : 0xff222222;
        }
    }

    r_FillImageChunk(texture->image, default_texture_pixels, NULL);
    r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    mem_Free(default_texture_pixels);
}

struct r_texture_h r_LoadTexture(char *file_name, char *texture_name)
{
    unsigned char *pixels;
    int width;
    int height;
    int channels;
    struct r_texture_h handle = R_INVALID_TEXTURE_HANDLE;
    struct r_texture_t *texture;
    struct r_texture_description_t description = {};

    file_name = ds_path_FormatPath(file_name);

    pixels = stbi_load(file_name, &width, &height, &channels, STBI_rgb_alpha);

    if(pixels)
    {
        description.extent.width = width;
        description.extent.height = height;
        description.extent.depth = 1;
        description.image_type = VK_IMAGE_TYPE_2D;
        description.format = VK_FORMAT_R8G8B8A8_UNORM;
        description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        description.sampler_params.mag_filter = VK_FILTER_LINEAR;
        description.sampler_params.min_filter = VK_FILTER_LINEAR;
        description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        handle = r_CreateTexture(&description);
        texture = r_GetTexturePointer(handle);

        if(texture_name)
        {
            texture->name = strdup(texture_name);
        }
        else
        {
            texture->name = strdup(file_name);
        }

        r_FillImageChunk(texture->image, pixels, NULL);
        printf("texture %s loaded!\n", texture->name);
    }

    return handle;
}

struct r_texture_t *r_GetDefaultTexturePointer()
{
    return r_GetTexturePointer(R_DEFAULT_TEXTURE_HANDLE);
}

struct r_texture_h r_GetDefaultTextureHandle()
{
    return R_DEFAULT_TEXTURE_HANDLE;
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_light_h r_CreateLight(vec3_t *position, uint32_t type)
{
    struct r_light_h handle;
    struct r_light_t *light;
    
    handle.index = add_stack_list_element(&r_lights, NULL);
    light = get_stack_list_element(&r_lights, handle.index);
    light->position = *position;
    light->type = type;
    
    return handle;
}

struct r_light_h r_CreatePointLight(vec3_t *position, vec3_t *color, float radius)
{
    struct r_light_h handle = r_CreateLight(position, R_LIGHT_TYPE_POINT);
    struct r_light_t *light = r_GetLightPointer(handle);
    
    light->color = *color;
    light->size = vec2_t_c(radius, 0.0);
    
    return handle;
}

struct r_light_t *r_GetLightPointer(struct r_light_h handle)
{
    struct r_light_t *light = get_stack_list_element(&r_lights, handle.index);
    if(light && light->type == R_LIGHT_TYPE_NONE)
    {
        light = NULL;
    }
    
    return light;
}

void r_DestroyLight(struct r_light_h handle)
{
    struct r_light_t *light = r_GetLightPointer(handle);
    
    if(light)
    {
        light->type = R_LIGHT_TYPE_NONE;
        remove_stack_list_element(&r_lights, handle.index);
    }
}

/*
=================================================================
=================================================================
=================================================================
*/

struct r_chunk_h r_AllocVerts(uint32_t count)
{
    return r_AllocVertexChunk(sizeof(struct r_vertex_t), count, sizeof(struct r_vertex_t));
}

void r_FillVertsChunk(struct r_chunk_h chunk, struct r_vertex_t *vertices, uint32_t count)
{
    r_FillBufferChunk(chunk, vertices, sizeof(struct r_vertex_t) * count, 0);
}

struct r_chunk_h r_AllocIndexes(uint32_t count)
{
    return r_AllocIndexChunk(sizeof(uint32_t), count, sizeof(uint32_t));
}

void r_FillIndexChunk(struct r_chunk_h chunk, uint32_t *indices, uint32_t count)
{
    r_FillBufferChunk(chunk, indices, sizeof(uint32_t) * count, 0);
}
