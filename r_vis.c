#include "r_vis.h"
#include "ent.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_mem.h"

#include <stdint.h>

extern struct stack_list_t r_lights;
extern struct stack_list_t ent_entities;
#define R_CLUSTER_WIDTH 64
#define R_CLUSTER_HEIGHT 32
#define R_CLUSTER_DEPTH 16

void r_VisibleEntities(struct r_view_t *view, struct stack_list_t *entities, struct list_t *visible_entities)
{
    visible_entities->cursor = 0;
    
    if(!entities)
    {
        entities = &ent_entities;
    }
    
    for(uint32_t entity_index = 0; entity_index < entities->cursor; entity_index++)
    {
        struct ent_entity_t *entity = get_stack_list_element(entities, entity_index);
        if(entity && entity->type != ENT_ENTITY_TYPE_NONE)
        {
            add_list_element(visible_entities, &ENT_ENTITY_HANDLE(entity_index));
        }
    }

//    struct r_view_t *view;
//    struct stack_list_t *entities;
//    struct r_begin_submission_info_t begin_info;
    
//    view = r_GetViewPointer();
//    entities = ent_GetEntityList();
//    
//    begin_info.inv_view_matrix = view->inv_view_matrix;
//    begin_info.projection_matrix = view->projection_matrix;
//    begin_info.framebuffer = R_INVALID_FRAMEBUFFER_HANDLE;
//    begin_info.viewport = view->viewport;
//    begin_info.scissor = view->scissor;
//    begin_info.clear_framebuffer = 0;
//    
//    r_BeginSubmission(&begin_info);
//    for(uint32_t entity_index = 0; entity_index < entities->cursor; entity_index++)
//    {
//        struct ent_entity_t *entity = ent_GetEntityPointer(ENT_ENTITY_HANDLE(entity_index));
//        struct mdl_model_t *model;
//        if(entity)
//        {
//            model = mdl_GetModelPointer(entity->model);
//            if(model)
//            {
//                struct mdl_batch_t *batches = mdl_GetModelLod(model, 0);
//                for(uint32_t batch_index = 0; batch_index < model->batch_count; batch_index++)
//                {
//                    struct mdl_batch_t *batch = batches + batch_index;
//                    r_Draw(batch->start, batch->count, batch->material, &entity->transform);
//                }
//            }
//        }
//    }
//    r_EndSubmission();
}

void r_VisibleLights(struct r_view_t *view, struct list_t *lights, struct r_cluster_list_t *cluster_list)
{    
    uint32_t cluster_columns = view->viewport.width / R_CLUSTER_WIDTH;
    uint32_t cluster_rows = view->viewport.height / R_CLUSTER_HEIGHT;
    
    /* clustered deferred and forward shading - Ola Olsson, Markus Billeter and Ulf Assarsson */
    float denom = log(1.0 + (2.0 * tanf(view->fov_y)) / R_CLUSTER_HEIGHT);
    uint32_t cluster_slices = (uint32_t)(log(view->z_far / view->z_near) / denom);
    
    if(!cluster_columns)
    {
        cluster_columns = 1;
    }
    
    if(!cluster_rows)
    {
        cluster_rows = 1;
    }
    
    if(!cluster_slices)
    {
        cluster_slices = 1;
    }
    
    uint32_t total_clusters = cluster_columns * cluster_rows * cluster_slices;
    
    if(total_clusters > cluster_list->cluster_count)
    {
        cluster_list->cluster_count = total_clusters;
        
        cluster_list->clusters = mem_Realloc(cluster_list->clusters, sizeof(struct r_cluster_t) * total_clusters);
        cluster_list->width = cluster_columns;
        cluster_list->height = cluster_rows;
        cluster_list->depth = cluster_slices;
    }
    
    for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
    {
        struct r_light_t *light = r_GetLightPointer(R_LIGHT_HANDLE(light_index));
        
        if(light)
        {
            
        }
    }
}

void r_VisiblePortals(struct r_view_t *view)
{
    
}

void r_VisibleWorld(struct r_view_t *view)
{
    
}

void r_DrawVisibleWorld(struct r_begin_submission_info_t *begin_info)
{
    
}






