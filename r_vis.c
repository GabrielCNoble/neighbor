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

//void r_VisibleEntities(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list)
//{
//    visible_entities->cursor = 0;
//    
//    if(!entities)
//    {
//        entities = &ent_entities;
//    }
//    
//    for(uint32_t entity_index = 0; entity_index < entities->cursor; entity_index++)
//    {
//        struct ent_entity_t *entity = get_stack_list_element(entities, entity_index);
//        if(entity && entity->type != ENT_ENTITY_TYPE_NONE)
//        {
//            add_list_element(visible_entities, &ENT_ENTITY_HANDLE(entity_index));
//        }
//    }
//}
//
void r_VisibleLights(struct r_view_t *view, struct r_cluster_list_t *cluster_list)
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
//
//void r_VisiblePortals(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list)
//{
//    
//}
//
//void r_VisibleWorld(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list)
//{
//    
//}
//
//void r_DrawVisibleWorld(struct r_view_t *view, struct r_framebuffer_h framebuffer)
//{
//    
//}






