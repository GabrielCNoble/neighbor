#include "mdl.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_file.h"
#include "lib/dstuff/ds_path.h"
#include "lib/dstuff/ds_xchg.h"
#include "lib/dstuff/ds_obj.h"
//#include "lib/dstuff/loaders/bsp.h"
#include "lib/dstuff/ds_vector.h"
#include "lib/dstuff/ds_mem.h"
#include "r_nvkl.h"
#include "r_draw.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>

struct stack_list_t mdl_models;

void mdl_Init()
{
    mdl_models = create_stack_list(sizeof(struct mdl_model_t), 512);
}

void mdl_Shutdown()
{
    destroy_stack_list(&mdl_models);
}

struct mdl_model_h mdl_CreateModel()
{
    struct mdl_model_t *model;
    struct mdl_model_h handle;

    handle.model_index = add_stack_list_element(&mdl_models, NULL);
    model = (struct mdl_model_t *)get_stack_list_element(&mdl_models, handle.model_index);

    model->lod_count = 0;
    model->batch_count = 0;
    model->batches = NULL;
    model->name = NULL;

    return handle;
}

void mdl_DestroyModel(struct mdl_model_h handle)
{
    
}

struct mdl_model_h mdl_GetModelHandle(char* name)
{
    struct mdl_model_t* model = NULL;
    for(uint32_t i = 0; i < mdl_models.cursor; i++)
    {
        model = mdl_GetModelPointer(MDL_MODEL_HANDLE(i));
        if(model && !strcmp(model->name, name))
        {
            return MDL_MODEL_HANDLE(i);
        }
    }
    return MDL_INVALID_MODEL_HANDLE;
}

struct mdl_model_t *mdl_GetModelPointer(struct mdl_model_h handle)
{
    struct mdl_model_t *model;

    model = (struct mdl_model_t *)get_stack_list_element(&mdl_models, handle.model_index);

    if(model && model->lod_count == MDL_INVALID_LOD_COUNT)
    {
        model = NULL;
    }

    return model;
}

struct mdl_batch_t *mdl_GetModelLod(struct mdl_model_t *model, uint32_t lod)
{
    if(lod >= model->lod_count)
    {
        lod = model->lod_count - 1;
    }

    return model->batches + model->batch_count * lod;
}

struct mdl_model_h mdl_LoadModel(char *file_name, char *model_name)
{
    struct geometry_data_t data;
    struct batch_data_t *batch;
    struct r_vertex_t *vertices;
    struct r_material_h material_handle;
//    struct r_texture_h texture_handle;
    struct r_texture_t *texture;
    struct r_material_t *material;
    struct mdl_model_h handle = MDL_INVALID_MODEL_HANDLE;
    struct mdl_model_t *model;
    struct r_chunk_t *chunk;
//    struct r_alloc_t *alloc;
    char *texture_name;
    vec3_t vert;
    vec2_t vec2;
    uint32_t model_start;

//    if(strstr(file_name, ".bsp"))
//    {
////        load_bsp(file_name, &data);
//    }
//    else
    {
        load_wavefront(file_name, &data);
    }


    if(data.vertices.cursor)
    {
        handle = mdl_CreateModel();
        model = mdl_GetModelPointer(handle);
        model->batches = mem_Calloc(sizeof(struct mdl_batch_t), data.batches.cursor);
        model->lod_count = 1;
        model->batch_count = data.batches.cursor;
        model->name = strdup(model_name);
        model->vertices = mem_Calloc(data.vertices.cursor, sizeof(vec3_t));
        
        for(uint32_t i = 0; i < data.vertices.cursor; i++)
        {
            model->vertices[i] = *(vec3_t *)get_list_element(&data.vertices, i);
        }

        for(uint32_t i = 0; i < data.batches.cursor; i++)
        {
            batch = (struct batch_data_t*)get_list_element(&data.batches, i);
            material_handle = r_GetMaterialHandle(batch->material);
            material = r_GetMaterialPointer(material_handle);
            
            if(!material)
            {
                material_handle = r_CreateMaterial();
                material = r_GetMaterialPointer(material_handle);

                material->name = strdup(batch->material);
//                material->base_color = batch->base_color;

                if(batch->diffuse_texture[0] != '\0')
                {
                    texture = r_GetTexture(batch->diffuse_texture);

                    if(!texture)
                    {
                        texture_name = ds_path_GetFileName(batch->diffuse_texture);
                        texture = r_LoadTexture(batch->diffuse_texture, texture_name);
                    }

                    material->diffuse_texture = texture;
                }

                if(!material->diffuse_texture)
                {
                    material->diffuse_texture = r_GetDefaultTexture();
                }

                if(batch->normal_texture[0] != '\0')
                {
                    texture = r_GetTexture(batch->normal_texture);

                    if(!texture)
                    {
                        texture_name = ds_path_GetFileName(batch->normal_texture);
                        texture = r_LoadTexture(batch->normal_texture, texture_name);
                    }

                    material->normal_texture = texture;
                }
            }

            model->batches[i].material = material;
            model->batches[i].start = batch->start;
            model->batches[i].count = batch->count;
        }

        vertices = mem_Calloc(data.vertices.cursor, sizeof(struct r_vertex_t));
        model->min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);
        model->max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        
        for(uint32_t i = 0; i < data.vertices.cursor; i++)
        {
            vert = *(vec3_t *)get_list_element(&data.vertices, i);
            vertices[i].position.comps[0] = vert.comps[0];
            vertices[i].position.comps[1] = vert.comps[1];
            vertices[i].position.comps[2] = vert.comps[2];
            vertices[i].position.comps[3] = 1.0;

            if(vert.x > model->max.x) model->max.x = vert.x;
            if(vert.y > model->max.y) model->max.y = vert.y;
            if(vert.z > model->max.z) model->max.z = vert.z;

            if(vert.x < model->min.x) model->min.x = vert.x;
            if(vert.y < model->min.y) model->min.y = vert.y;
            if(vert.z < model->min.z) model->min.z = vert.z;

            vert = *(vec3_t *)get_list_element(&data.normals, i);
            vertices[i].normal.comps[0] = vert.comps[0];
            vertices[i].normal.comps[1] = vert.comps[1];
            vertices[i].normal.comps[2] = vert.comps[2];
            vertices[i].normal.comps[3] = 0.0;
        }

        /* some models may not have texture coordinates */
        for(uint32_t i = 0; i < data.tex_coords.cursor; i++)
        {
            vec2 = *(vec2_t *)get_list_element(&data.tex_coords, i);
            vertices[i].tex_coords.comps[0] = vec2.comps[0];
            vertices[i].tex_coords.comps[1] = vec2.comps[1];
        }


        model->chunk = r_AllocVerts(sizeof(struct r_vertex_t) * data.vertices.cursor);
        r_FillBufferChunk(model->chunk, vertices, sizeof(struct r_vertex_t) * data.vertices.cursor, 0);
        chunk = r_GetChunkPointer(model->chunk);
        model_start = chunk->start / sizeof(struct r_vertex_t);

        for(uint32_t i = 0; i < model->batch_count; i++)
        {
            model->batches[i].start += model_start;
        }
    }

    return handle;
}
