#ifndef R_VIS_H
#define R_VIS_H

#include "r_draw.h"
#include "lib/dstuff/ds_stack_list.h"

void r_VisibleEntities();

void r_VisibleLights(struct r_view_t *view, struct list_t *lights, struct r_cluster_list_t *cluster_list);

void r_VisiblePortals(struct r_view_t *view);

void r_VisibleWorld(struct r_view_t *view);

void r_DrawVisibleWorld(struct r_begin_submission_info_t *begin_info);


#endif // R_VIS_H
