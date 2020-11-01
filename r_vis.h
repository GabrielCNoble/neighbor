#ifndef R_VIS_H
#define R_VIS_H

#include "r_draw.h"
#include "lib/dstuff/ds_stack_list.h"

//void r_VisibleEntities(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list);
//
void r_VisibleLights(struct r_view_t *view, struct r_cluster_list_t *cluster_list);
//
//void r_VisiblePortals(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list);
//
//void r_VisibleWorld(struct r_view_t *view, struct r_draw_cmd_list_h draw_cmd_list);
//
//void r_DrawVisibleWorld(struct r_view_t *view, struct r_framebuffer_h framebuffer);


#endif // R_VIS_H
