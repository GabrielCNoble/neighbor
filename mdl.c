#include "mdl.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/file/file.h"
#include "dstuff/file/path.h"
#include "dstuff/loaders/xchg.h"
#include "dstuff/loaders/obj.h"
#include "dstuff/math/vector.h"
#include "r_main.h"
#include <stdlib.h>
#include <string.h>

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
    model = get_stack_list_element(&models, handle.model_index);

    model->lod_count = 0;
    model->batch_count = 0;
    model->batches = NULL;

    return handle;
}

struct model_t *mdl_GetModelPointer(struct model_handle_t handle)
{
    struct model_t *model;

    model = get_stack_list_element(&models, handle.model_index);

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

        model->lod_count = MDL_INVALID_LOD_COUNT;
        remove_stack_list_element(&models, handle.model_index);
    }
}

struct model_handle_t mdl_LoadModel(char *file_name)
{
    struct geometry_data_t data;
    struct batch_data_t *batch;
    struct vertex_t *vertices;
    struct r_material_handle_t material_handle;
    struct r_material_t *material;
    struct model_handle_t handle = MDL_INVALID_MODEL_HANDLE;
    struct model_t *model;
    struct r_alloc_t *alloc;
    char *texture_name;
    vec3_t vec3;
    vec2_t vec2;
    uint32_t model_start;

    load_wavefront(file_name, &data);

    if(data.vertices.cursor)
    {
        handle = mdl_AllocModel();
        model = mdl_GetModelPointer(handle);
        model->batches = calloc(sizeof(struct model_batch_t), data.batches.cursor);
        model->lod_count = 1;

        for(uint32_t i = 0; i < data.batches.cursor; i++)
        {
            batch = get_list_element(&data.batches, i);
            material_handle = r_GetMaterialHandle(batch->material);
            material = r_GetMaterialPointer(material_handle);

            printf("%s\n", batch->material);

            if(!material)
            {
                material_handle = r_AllocMaterial();
                material = r_GetMaterialPointer(material_handle);

                material->name = strdup(batch->material);
                material->base_color = batch->base_color;

                if(batch->diffuse_texture[0] != '\0')
                {
                    material->diffuse_texture = r_GetTextureHandle(batch->diffuse_texture);

                    if(material->diffuse_texture.index != R_INVALID_TEXTURE_INDEX)
                    {
                        texture_name = get_file_name_no_ext(get_file_from_path(batch->diffuse_texture));
                        printf("diffuse texture: %s\n", texture_name);
                        material->diffuse_texture = r_LoadTexture(batch->diffuse_texture, texture_name);
                    }
                }

                if(batch->normal_texture[0] != '\0')
                {
                    material->normal_texture = r_GetTextureHandle(batch->normal_texture);

                    if(material->normal_texture.index != R_INVALID_TEXTURE_INDEX)
                    {
                        texture_name = get_file_name_no_ext(get_file_from_path(batch->diffuse_texture));
                        printf("normal texture: %s\n", texture_name);
                        material->normal_texture = r_LoadTexture(batch->normal_texture, texture_name);
                    }
                }
            }

            model->batches[i].material = material_handle;
            model->batches[i].range.start = batch->start;
            model->batches[i].range.count = batch->count;
        }

        vertices = calloc(data.vertices.cursor, sizeof(struct vertex_t));

        for(uint32_t i = 0; i < data.vertices.cursor; i++)
        {
            vec3 = *(vec3_t *)get_list_element(&data.vertices, i);

            vertices[i].position.comps[0] = vec3.comps[0];
            vertices[i].position.comps[1] = vec3.comps[1];
            vertices[i].position.comps[2] = vec3.comps[2];
            vertices[i].position.comps[3] = 1.0;
        }

        /* some models may not have texture coordinates */
        for(uint32_t i = 0; i < data.tex_coords.cursor; i++)
        {
            vec2 = *(vec2_t *)get_list_element(&data.tex_coords, i);
            vertices[i].tex_coords.comps[0] = vec2.comps[0];
            vertices[i].tex_coords.comps[1] = vec2.comps[1];
        }


        model->alloc = r_Alloc(sizeof(struct vertex_t) * data.vertices.cursor, sizeof(struct vertex_t), 0);
        r_Memcpy(model->alloc, vertices, sizeof(struct vertex_t) * data.vertices.cursor);
        alloc = r_GetAllocPointer(model->alloc);
        model_start = alloc->start + alloc->align;

        for(uint32_t i = 0; i < model->batch_count; i++)
        {
            model->batches[i].range.start += model_start;
        }
    }

    return handle;
}