#include "mdl.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/file/file.h"
#include "dstuff/file/path.h"
#include "dstuff/loaders/xchg.h"
#include "dstuff/loaders/obj.h"
#include "dstuff/loaders/bsp.h"
#include "dstuff/math/vector.h"
#include "r.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>

struct stack_list_t models;

void mdl_Init()
{
    models = create_stack_list(sizeof(struct model_t), 512);
}

void mdl_Shutdown()
{
    destroy_stack_list(&models);
}

struct model_handle_t mdl_AllocModel()
{
    struct model_t *model;
    struct model_handle_t handle;

    handle.model_index = add_stack_list_element(&models, NULL);
    model = (struct model_t*)get_stack_list_element(&models, handle.model_index);

    model->lod_count = 0;
    model->batch_count = 0;
    model->batches = NULL;
    model->name = NULL;

    return handle;
}

struct model_handle_t mdl_GetModelHandle(char* name)
{
    struct model_t* model = NULL;
    for(uint32_t i = 0; i < models.cursor; i++)
    {
        model = mdl_GetModelPointer(MDL_MODEL_HANDLE(i));
        if(model && !strcmp(model->name, name))
        {
            return MDL_MODEL_HANDLE(i);
        }
    }
    return MDL_INVALID_MODEL_HANDLE;
}
struct model_t *mdl_GetModelPointer(struct model_handle_t handle)
{
    struct model_t *model;

    model = (struct model_t*)get_stack_list_element(&models, handle.model_index);

    if(model && model->lod_count == MDL_INVALID_LOD_COUNT)
    {
        model = NULL;
    }

    return model;
}

struct model_batch_t *mdl_GetModelLod(struct model_t *model, uint32_t lod)
{
    if(lod >= model->lod_count)
    {
        lod = model->lod_count - 1;
    }

    return model->batches + model->batch_count * lod;
}

void mdl_FreeModel(struct model_handle_t handle)
{
    struct model_t *model;
    model = mdl_GetModelPointer(handle);

    if(model)
    {
        r_Free(model->alloc);
        free(model->batches);
        free(model->name);

        model->lod_count = MDL_INVALID_LOD_COUNT;
        remove_stack_list_element(&models, handle.model_index);
    }
}

struct model_handle_t mdl_LoadModel(char *file_name)
{
//    struct geometry_data_t data;
//    struct batch_data_t *batch;
//    struct vertex_t *vertices;
//    struct r_material_handle_t material_handle;
//    struct r_texture_handle_t texture_handle;
//    struct r_material_t *material;
//    struct model_handle_t handle = MDL_INVALID_MODEL_HANDLE;
//    struct model_t *model;
//    struct r_alloc_t *alloc;
//    char *texture_name;
//    vec3_t vert;
//    vec2_t vec2;
//    uint32_t model_start;
//
//    if(strstr(file_name, ".bsp"))
//    {
////        load_bsp(file_name, &data);
//    }
//    else
//    {
////        load_wavefront(file_name, &data);
//    }
//
//    // printf("%s\n", file_name);
//
//    if(data.vertices.cursor)
//    {
//        handle = mdl_AllocModel();
//        model = mdl_GetModelPointer(handle);
//        model->batches = (struct model_batch_t*)calloc(sizeof(struct model_batch_t), data.batches.cursor);
//        model->lod_count = 1;
//        model->batch_count = data.batches.cursor;
//        model->name = strdup(get_file_name_no_ext(get_file_from_path(file_name)));
//        model->vertices = (vec3_t*)calloc(data.vertices.cursor, sizeof(vec3_t));
//        for(uint32_t i = 0; i < data.vertices.cursor; i++)
//        {
//            model->vertices[i] = *(vec3_t*)get_list_element(&data.vertices, i);
//        }
//
//        for(uint32_t i = 0; i < data.batches.cursor; i++)
//        {
//            batch = (struct batch_data_t*)get_list_element(&data.batches, i);
//            material_handle = r_GetMaterialHandle(batch->material);
//            material = r_GetMaterialPointer(material_handle);
//            if(!material)
//            {
//                material_handle = r_AllocMaterial();
//                material = r_GetMaterialPointer(material_handle);
//
//                material->name = strdup(batch->material);
//                material->base_color = batch->base_color;
//
//                if(batch->diffuse_texture[0] != '\0')
//                {
//                    texture_handle = r_GetTextureHandle(batch->diffuse_texture);
//
//                    if(texture_handle.index == R_INVALID_TEXTURE_INDEX)
//                    {
//                        texture_name = get_file_name_no_ext(get_file_from_path(batch->diffuse_texture));
//                        texture_handle = r_LoadTexture(batch->diffuse_texture, texture_name);
//                    }
//
//                    material->diffuse_texture = r_GetTexturePointer(texture_handle);
//                }
//
//                if(!material->diffuse_texture)
//                {
//                    material->diffuse_texture = r_GetDefaultTexturePointer();
//                }
//
//                if(batch->normal_texture[0] != '\0')
//                {
//                    texture_handle = r_GetTextureHandle(batch->normal_texture);
//
//                    if(texture_handle.index == R_INVALID_TEXTURE_INDEX)
//                    {
//                        texture_name = get_file_name_no_ext(get_file_from_path(batch->normal_texture));
//                        texture_handle = r_LoadTexture(batch->normal_texture, texture_name);
//                    }
//
//                    material->normal_texture = r_GetTexturePointer(texture_handle);
//                }
//            }
//
//            model->batches[i].material = material;
//            model->batches[i].range.start = batch->start;
//            model->batches[i].range.count = batch->count;
//        }
//
//        vertices = (struct vertex_t*)calloc(data.vertices.cursor, sizeof(struct vertex_t));
//        model->min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);
//        model->max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);
//        for(uint32_t i = 0; i < data.vertices.cursor; i++)
//        {
//            vert = *(vec3_t *)get_list_element(&data.vertices, i);
//            vertices[i].position.comps[0] = vert.comps[0];
//            vertices[i].position.comps[1] = vert.comps[1];
//            vertices[i].position.comps[2] = vert.comps[2];
//            vertices[i].position.comps[3] = 1.0;
//
//            if(vert.x > model->max.x) model->max.x = vert.x;
//            if(vert.y > model->max.y) model->max.y = vert.y;
//            if(vert.z > model->max.z) model->max.z = vert.z;
//
//            if(vert.x < model->min.x) model->min.x = vert.x;
//            if(vert.y < model->min.y) model->min.y = vert.y;
//            if(vert.z < model->min.z) model->min.z = vert.z;
//
//            vert = *(vec3_t *)get_list_element(&data.normals, i);
//            vertices[i].normal.comps[0] = vert.comps[0];
//            vertices[i].normal.comps[1] = vert.comps[1];
//            vertices[i].normal.comps[2] = vert.comps[2];
//            vertices[i].normal.comps[3] = 0.0;
//        }
//
//        /* some models may not have texture coordinates */
//        for(uint32_t i = 0; i < data.tex_coords.cursor; i++)
//        {
//            vec2 = *(vec2_t *)get_list_element(&data.tex_coords, i);
//            vertices[i].tex_coords.comps[0] = vec2.comps[0];
//            vertices[i].tex_coords.comps[1] = vec2.comps[1];
//        }
//
//
//        model->alloc = r_Alloc(sizeof(struct vertex_t) * data.vertices.cursor, sizeof(struct vertex_t), 0);
//        r_Memcpy(model->alloc, vertices, sizeof(struct vertex_t) * data.vertices.cursor);
//        alloc = r_GetAllocPointer(model->alloc);
//        model_start = (alloc->start + alloc->align) / sizeof(struct vertex_t);
//
//        for(uint32_t i = 0; i < model->batch_count; i++)
//        {
//            model->batches[i].range.start += model_start;
//        }
//    }
//
//    return handle;
}
