#include "r_vis.h"
#include "r_draw.h"
#include "ent.h"
#include "lib/dstuff/ds_stack_list.h"

#include <stdint.h>

void r_VisibleEntities()
{
    struct r_view_t *view;
    struct stack_list_t *entities;
    struct r_begin_submission_info_t begin_info;
    
    view = r_GetViewPointer();
    entities = ent_GetEntityList();
    
    begin_info.inv_view_matrix = view->inv_view_matrix;
    begin_info.projection_matrix = view->projection_matrix;
    begin_info.framebuffer = R_INVALID_FRAMEBUFFER_HANDLE;
    begin_info.viewport = view->viewport;
    begin_info.scissor = view->scissor;
    begin_info.clear_framebuffer = 0;
    
    r_BeginSubmission(&begin_info);
    for(uint32_t entity_index = 0; entity_index < entities->cursor; entity_index++)
    {
        struct ent_entity_t *entity = ent_GetEntityPointer(ENT_ENTITY_HANDLE(entity_index));
        struct mdl_model_t *model;
        if(entity)
        {
            model = mdl_GetModelPointer(entity->model);
            if(model)
            {
                struct mdl_batch_t *batches = mdl_GetModelLod(model, 0);
                for(uint32_t batch_index = 0; batch_index < model->batch_count; batch_index++)
                {
                    struct mdl_batch_t *batch = batches + batch_index;
                    r_Draw(batch->start, batch->count, batch->material, &entity->transform);
                }
            }
        }
    }
    r_EndSubmission();
}

void r_VisibleLights()
{
    
}

void r_VisibleWorld()
{
    
}
