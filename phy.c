#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <stdio.h>
#include "r_draw.h"
#include "phy.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_dbvh.h"

#define PHY_GRAVITY 0.0098


struct stack_list_t phy_shapes[PHY_SHAPE_TYPE_LAST];
struct stack_list_t phy_colliders[PHY_COLLIDER_TYPE_LAST];
struct list_t phy_active_colliders;
struct list_t phy_collision_pairs;
struct list_t phy_contact_points;
struct list_t phy_temp_contacts0;
struct list_t phy_temp_contacts1;
uint32_t phy_collider_sizes[] = {
    [PHY_COLLIDER_TYPE_STATIC] = sizeof(struct phy_static_collider_t),
    [PHY_COLLIDER_TYPE_DYNAMIC] = sizeof(struct phy_dynamic_collider_t),
    [PHY_COLLIDER_TYPE_KINEMATIC] = sizeof(struct phy_kinematic_collider_t)
};
uint32_t phy_shape_sizes[] = {
    [PHY_SHAPE_TYPE_BOX] = sizeof(struct phy_box_shape_t)
};

uint32_t (*phy_contact_functions[PHY_SHAPE_TYPE_LAST][PHY_SHAPE_TYPE_LAST])(struct phy_collision_data_t *data_a, struct phy_collision_data_t *data_b, struct phy_contact_point_t *contact) = {
    [PHY_SHAPE_TYPE_BOX][PHY_SHAPE_TYPE_BOX] = phy_ContactBoxBox,
};

void (*phy_collision_functions[PHY_COLLIDER_TYPE_LAST][PHY_COLLIDER_TYPE_LAST])(struct phy_collider_h collider_a, struct phy_collider_h collider_b) = {
    [PHY_COLLIDER_TYPE_STATIC][PHY_COLLIDER_TYPE_KINEMATIC] = phy_CollideStaticKinematic,
};

struct dbvh_tree_t phy_dbvh;
uint32_t phy_step = 0;
uint32_t phy_draw_colliders = 1;

void phy_Init()
{
    for(uint32_t collider_type = PHY_COLLIDER_TYPE_STATIC; collider_type < PHY_COLLIDER_TYPE_LAST; collider_type++)
    {
        phy_colliders[collider_type] = create_stack_list(phy_collider_sizes[collider_type], 512);
    }

    for(uint32_t shape_type = PHY_SHAPE_TYPE_BOX; shape_type < PHY_SHAPE_TYPE_LAST; shape_type++)
    {
        phy_shapes[shape_type] = create_stack_list(phy_shape_sizes[shape_type], 512);
    }

    phy_dbvh = create_dbvh_tree(sizeof(struct phy_collider_h));
    phy_collision_pairs = create_list(sizeof(struct phy_collision_pair_t), 4096);
    phy_contact_points = create_list(sizeof(struct phy_contact_point_t), 4096);
    phy_active_colliders = create_list(sizeof(struct phy_collider_h), 512);
}

void phy_Shutdown()
{

}

struct phy_collider_h phy_CreateCollider(uint32_t type, struct phy_shape_h shape_handle)
{
    struct phy_collider_h collider_handle = PHY_INVALID_COLLIDER_HANDLE;
    struct phy_collider_t *collider;

    if(type < PHY_COLLIDER_TYPE_LAST)
    {
        collider_handle.index = add_stack_list_element(&phy_colliders[type], NULL);
        collider_handle.type = type;
        collider = get_stack_list_element(&phy_colliders[type], collider_handle.index);
        collider->type = type;
        collider->shape = shape_handle;
        collider->dbvh_node = INVALID_DBVH_NODE_INDEX;
        collider->active_index = 0xffffffff;
        collider->position = vec2_t_c(0.0, 0.0);
        collider->rotation = 0.0;
    }

    return collider_handle;
}

struct phy_collider_h phy_CreateDynamicCollider(struct phy_shape_h shape_handle)
{
    struct phy_dynamic_collider_t *collider;
    struct phy_collider_h collider_handle;
    collider_handle = phy_CreateCollider(PHY_COLLIDER_TYPE_DYNAMIC, shape_handle);
    collider = (struct phy_dynamic_collider_t *)phy_GetColliderPointer(collider_handle);
    collider->linear_velocity = vec2_t_c(0.0, 0.0);
    collider->angular_velocity = 0.0;
    return collider_handle;
}

struct phy_collider_h phy_CreateKinematicCollider(struct phy_shape_h shape_handle)
{
    struct phy_kinematic_collider_t *collider;
    struct phy_collider_h collider_handle;
    collider_handle = phy_CreateCollider(PHY_COLLIDER_TYPE_KINEMATIC, shape_handle);
    collider = (struct phy_kinematic_collider_t *)phy_GetColliderPointer(collider_handle);
    collider->velocity = vec2_t_c(0.0, 0.0);
    return collider_handle;
}

struct phy_collider_h phy_CreateStaticCollider(struct phy_shape_h shape_handle)
{
    struct phy_collider_h collider_handle;
    collider_handle = phy_CreateCollider(PHY_COLLIDER_TYPE_STATIC, shape_handle);
    return collider_handle;
}

struct phy_collider_t *phy_GetColliderPointer(struct phy_collider_h collider_handle)
{
    struct phy_collider_t *collider = NULL;
    if(collider_handle.type < PHY_COLLIDER_TYPE_LAST)
    {
        collider = get_stack_list_element(&phy_colliders[collider_handle.type], collider_handle.index);
        if(collider && collider->type == PHY_COLLIDER_TYPE_LAST)
        {
            collider = NULL;
        }
    }

    return collider;
}

void phy_GetColliderVertices(struct phy_collider_h collider, struct r_i_vertex_t **vertices, uint32_t *vertice_count)
{
    struct phy_collider_t *collider_ptr;
    struct phy_shape_t *shape;
    static struct r_i_vertex_t collider_vertices[64];
    uint32_t collider_vertice_count;
    
    collider_ptr = phy_GetColliderPointer(collider);
    shape = phy_GetShapePointer(collider_ptr->shape);
    
    switch(shape->type)
    {
        case PHY_SHAPE_TYPE_BOX:
        {
            struct phy_box_shape_t *box_shape = (struct phy_box_shape_t *)shape;            
            vec2_t top = vec2_t_c(box_shape->extents.y * box_shape->base.sin, box_shape->extents.y * box_shape->base.cos);
            vec2_t right = vec2_t_c(box_shape->extents.x * box_shape->base.cos, -box_shape->extents.x * box_shape->base.sin);
//            float x = box_shape->extents.x * box_shape->base.cos - box_shape->extents.x * box_shape->base.sin;
//            float y = box_shape->extents.y * box_shape->base.sin + box_shape->extents.y * box_shape->base.cos;
            
            collider_vertices[0].position = vec4_t_c(top.x - right.x, top.y - right.y, -0.2, 1.0);
            collider_vertices[0].color = vec4_t_c(0.0, 1.0, 0.0, 1.0);
            collider_vertices[1].position = vec4_t_c(-top.x - right.x, -top.y - right.y, -0.2, 1.0);
            collider_vertices[1].color = vec4_t_c(0.0, 1.0, 0.0, 1.0);
            collider_vertices[2].position = vec4_t_c(-top.x + right.x, -top.y + right.y, -0.2, 1.0);
            collider_vertices[2].color = vec4_t_c(0.0, 1.0, 0.0, 1.0);
            collider_vertices[3].position = vec4_t_c(top.x + right.x, top.y + right.y, -0.2, 1.0);
            collider_vertices[3].color = vec4_t_c(0.0, 1.0, 0.0, 1.0);
            collider_vertices[4] = collider_vertices[0];
            collider_vertice_count = 5;
        }
        break;
    }
    
    for(uint32_t i = 0; i < collider_vertice_count; i++)
    {
        collider_vertices[i].position.x += collider_ptr->position.x;
        collider_vertices[i].position.y += collider_ptr->position.y;
    }
    
    *vertices = collider_vertices;
    *vertice_count = collider_vertice_count;
}

void phy_AddColliderToWorld(struct phy_collider_h collider_handle)
{
    struct phy_collider_t *collider;
    struct dbvh_node_t *node;
    vec2_t extents;
    collider = phy_GetColliderPointer(collider_handle);
    if(collider && collider->active_index == 0xffffffff)
    {
        collider->active_index = add_list_element(&phy_active_colliders, &collider_handle);
        collider->step = phy_step + 1;
        if(collider->dbvh_node == INVALID_DBVH_NODE_INDEX)
        {
            collider->dbvh_node = alloc_dbvh_node(&phy_dbvh);
        }
        extents = phy_GetColliderExtends(collider_handle);
        node = get_dbvh_node_pointer(&phy_dbvh, collider->dbvh_node);
        node->max.x = collider->position.x + extents.x;
        node->max.y = collider->position.y + extents.y;
        node->max.z = 0.2;
        node->min.x = collider->position.x - extents.x;
        node->min.y = collider->position.y - extents.y;
        node->min.z = -0.2;
        insert_node_into_dbvh(&phy_dbvh, collider->dbvh_node);
        memcpy(node->user_data, &collider_handle, sizeof(struct phy_collider_h));
    }
}

void phy_SetColliderPosition(struct phy_collider_h collider_handle, vec2_t *position)
{
    struct phy_collider_t *collider;
    collider = phy_GetColliderPointer(collider_handle);
    if(collider)
    {
        collider->position.x = position->x;
        collider->position.y = position->y;
    }
}

void phy_SetColliderVelocity(struct phy_collider_h collider_handle, vec2_t *velocity)
{
    struct phy_dynamic_collider_t *collider;
    if(collider_handle.type == PHY_COLLIDER_TYPE_DYNAMIC)
    {
        collider = (struct phy_dynamic_collider_t *)phy_GetColliderPointer(collider_handle);
        collider->linear_velocity = *velocity;
    }
}

void phy_SetColliderRotation(struct phy_collider_h collider_handle, float rotation)
{
    struct phy_collider_t *collider;
    struct phy_shape_t *shape;
    collider = phy_GetColliderPointer(collider_handle);
    if(collider)
    {
        collider->rotation = rotation;
        shape = phy_GetShapePointer(collider->shape);
        shape->sin = sin(collider->rotation * 3.14159265);
        shape->cos = cos(collider->rotation * 3.14159265);
    }
}





struct phy_shape_h phy_CreateShape(uint32_t type)
{
    struct phy_shape_h shape_handle = PHY_INVALID_SHAPE_HANDLE;
    struct phy_shape_t *shape;

    if(type < PHY_SHAPE_TYPE_LAST)
    {
        shape_handle.index = add_stack_list_element(&phy_shapes[type], NULL);
        shape_handle.type = type;
        shape = get_stack_list_element(&phy_shapes[type], shape_handle.index);
        shape->type = type;
        shape->cos = 1.0;
        shape->sin = 0.0;
    }

    return shape_handle;
}

struct phy_shape_h phy_CreateBoxShape(vec2_t *extents)
{
    struct phy_box_shape_t *box_shape;
    struct phy_shape_h shape_handle;
    shape_handle = phy_CreateShape(PHY_SHAPE_TYPE_BOX);
    box_shape = (struct phy_box_shape_t *)phy_GetShapePointer(shape_handle);
    box_shape->extents = *extents;
    box_shape->extents.x *= 0.5;
    box_shape->extents.y *= 0.5;
    return shape_handle;
}

struct phy_shape_t *phy_GetShapePointer(struct phy_shape_h shape_handle)
{
    struct phy_shape_t *shape = NULL;
    if(shape_handle.type < PHY_SHAPE_TYPE_LAST)
    {
        shape = get_stack_list_element(&phy_shapes[shape_handle.type], shape_handle.index);
        if(shape && shape->type == PHY_SHAPE_TYPE_LAST)
        {
            shape = NULL;
        }
    }
    return shape;
}

vec2_t phy_GetColliderExtends(struct phy_collider_h collider_handle)
{
    struct phy_collider_t *collider;
    struct phy_shape_t *shape;
    vec2_t extents = vec2_t_c(0.0, 0.0);
    collider = phy_GetColliderPointer(collider_handle);
    if(collider)
    {
        shape = phy_GetShapePointer(collider->shape);
        switch(shape->type)
        {
            case PHY_SHAPE_TYPE_BOX:
            {
                struct phy_box_shape_t *box_shape = (struct phy_box_shape_t *)shape;
                extents = box_shape->extents;
            }
            break;
        }
    }

    return extents;
}

#define PHY_STEP_COUNT 1
void phy_Step(float delta_time)
{
    struct phy_collider_t *collider;
    struct phy_collider_h *collider_handle;
    struct phy_collision_pair_t *pair;

    struct phy_collision_data_t data_a;
    struct phy_collision_data_t data_b;
    struct phy_contact_point_t contact;
    struct phy_box_shape_t *box_shape;
    
    struct r_i_vertex_t *collider_vertices;
    uint32_t collider_vertice_count;
    struct r_view_t *view = r_GetViewPointer();
    
    for(uint32_t collider_index = 0; collider_index < phy_colliders[PHY_COLLIDER_TYPE_KINEMATIC].cursor; collider_index++)
    {
//        struct phy_kinematic_collider_t *kinematic_collider = phy_GetColliderPointer(PHY_COLLIDER_HANDLE(collider_index, PHY_COLLIDER_TYPE_KINEMATIC));
//        kinematic_collider->velocity.y -= PHY_GRAVITY * delta_time;
    }
    
    for(uint32_t step_index = 0; step_index < PHY_STEP_COUNT; step_index++)
    {
        phy_collision_pairs.cursor = 0;
        phy_contact_points.cursor = 0;
        
        for(uint32_t collider_index = 0; collider_index < phy_active_colliders.cursor; collider_index++)
        {
//            collider_handle = get_list_element(&phy_active_colliders, collider_index);
//            collider = phy_GetColliderPointer(*collider_handle);
//            collider->first_pair = phy_collision_pairs.cursor;
//            phy_IntersectNodes(*collider_handle);
//            collider->last_pair = phy_collision_pairs.cursor;
        }
        
        for(uint32_t collider_index = 0; collider_index < phy_active_colliders.cursor; collider_index++)
        {
//            collider_handle = get_list_element(&phy_active_colliders, collider_index);
//            collider = phy_GetColliderPointer(*collider_handle);
//            
//            if(collider->first_pair != collider->last_pair)
//            {
//                for(uint32_t pair_index = collider->first_pair; pair_index < collider->last_pair; pair_index++)
//                {
//                    pair = get_list_element(&phy_collision_pairs, pair_index);
//                    phy_collision_functions[pair->collider_a.type][pair->collider_b.type](pair->collider_a, pair->collider_b);
//                }
//            }
//            else
//            {
//                phy_MoveCollider(*collider_handle);
//            }
        }

        phy_step++;
    }
    
//    if(phy_draw_colliders)
//    {
//        r_i_BeginSubmission(&view->inv_view_matrix, &view->projection_matrix);
//        for(uint32_t collider_index = 0; collider_index < phy_active_colliders.cursor; collider_index++)
//        {
//            collider_handle = get_list_element(&phy_active_colliders, collider_index);
//            collider = phy_GetColliderPointer(*collider_handle);
//            phy_GetColliderVertices(*collider_handle, &collider_vertices, &collider_vertice_count);
//            r_i_DrawLines(collider_vertices, collider_vertice_count, 1.0, 1);
//        }
//        r_i_EndSubmission();
//    }
}

void phy_MoveCollider(struct phy_collider_h collider_handle)
{
    struct phy_collider_t *collider;
    
    collider = phy_GetColliderPointer(collider_handle);
    switch(collider->type)
    {
        case PHY_COLLIDER_TYPE_KINEMATIC:
        {
            struct phy_kinematic_collider_t *kinematic_collider = (struct phy_kinematic_collider_t *)collider;
            kinematic_collider->base.position.x += kinematic_collider->velocity.x;
            kinematic_collider->base.position.y += kinematic_collider->velocity.y;
        }
        break;
    }
}

void phy_IntersectNodesRec(struct dbvh_node_t *node, uint32_t node_index, uint32_t cur_node_index, uint32_t *smallest_index, float *smallest_area)
{
    struct dbvh_node_t *cur_node;
    struct phy_collision_pair_t *pair;
    struct phy_collider_t *collider;
    struct phy_collider_h collider_a;
    struct phy_collider_h collider_b;
    vec3_t volume_min;
    vec3_t volume_max;
    vec3_t extents;
    float area;

    if(node_index == cur_node_index)
    {
        return;
    }

    cur_node = get_dbvh_node_pointer(&phy_dbvh, cur_node_index);

    if(node->max.x > cur_node->min.x && node->min.x < cur_node->max.x)
    {
        if(node->max.y > cur_node->min.y && node->min.y < cur_node->max.y)
        {
            if(cur_node->children[0] == cur_node->children[1])
            {
                collider = phy_GetColliderPointer(*(struct phy_collider_h *)cur_node->user_data);
                if(collider->step != phy_step)
                {
                    pair = get_list_element(&phy_collision_pairs, add_list_element(&phy_collision_pairs, NULL));
                    memcpy(&collider_a, node->user_data, sizeof(struct phy_collider_h));
                    memcpy(&collider_b, cur_node->user_data, sizeof(struct phy_collider_h));
                    
                    if(collider_a.type > collider_b.type)
                    {
                        pair->collider_a = collider_b;
                        pair->collider_b = collider_a;
                    }
                    else
                    {
                        pair->collider_a = collider_a;
                        pair->collider_b = collider_b;
                    }
                }
            }
            else
            {
                phy_IntersectNodesRec(node, node_index, cur_node->children[0], smallest_index, smallest_area);
                phy_IntersectNodesRec(node, node_index, cur_node->children[1], smallest_index, smallest_area);
            }
        }
    }

    dbvh_nodes_volume(&phy_dbvh, node_index, cur_node_index, &volume_max, &volume_min);
    vec3_t_sub(&extents, &volume_max, &volume_min);
    area = ((extents.x * extents.y) + (extents.x * extents.z) + (extents.y * extents.z)) * 2.0;

    if(area < *smallest_area)
    {
        *smallest_area = area;
        *smallest_index = cur_node_index;
    }
}

void phy_IntersectNodes(struct phy_collider_h collider_handle)
{
    struct phy_collider_t *collider;
    struct dbvh_node_t *node;
    float smallest_area = FLT_MAX;
    uint32_t smallest_index = INVALID_DBVH_NODE_INDEX;
    vec2_t extents;

    collider = phy_GetColliderPointer(collider_handle);
    node = get_dbvh_node_pointer(&phy_dbvh, collider->dbvh_node);
    extents = phy_GetColliderExtends(collider_handle);

    node->max.x = collider->position.x + extents.x;
    node->max.y = collider->position.y + extents.y;
    node->max.z = 0.2;

    node->min.x = collider->position.x - extents.x;
    node->min.y = collider->position.y - extents.y;
    node->min.z = -0.2;
     
    switch(collider->type)
    {
        case PHY_COLLIDER_TYPE_KINEMATIC:
        {
            struct phy_kinematic_collider_t *kinematic_collider = (struct phy_kinematic_collider_t *)collider;
            for(uint32_t i = 0; i < 2; i++)
            {
                if(kinematic_collider->velocity.comps[i] < 0.0)
                {
                    node->min.comps[i] += kinematic_collider->velocity.comps[i];
                }
                else
                {
                    node->max.comps[i] += kinematic_collider->velocity.comps[i];
                }
            }
        }
        break;
    }

    phy_IntersectNodesRec(node, collider->dbvh_node, phy_dbvh.root, &smallest_index, &smallest_area);
    if(smallest_index != INVALID_DBVH_NODE_INDEX)
    {
        pair_dbvh_nodes(&phy_dbvh, collider->dbvh_node, smallest_index);
    }
    collider->step = phy_step;
}

void phy_CollideStaticKinematic(struct phy_collider_h collider_a, struct phy_collider_h collider_b)
{
    struct phy_static_collider_t *static_collider;
    struct phy_kinematic_collider_t *kinematic_collider;
    struct phy_collision_data_t data_a;
    struct phy_collision_data_t data_b;
    struct phy_contact_point_t contact_point;
    vec2_t parallel_velocity;
    
    static_collider = (struct phy_static_collider_t *)phy_GetColliderPointer(collider_a);
    kinematic_collider = (struct phy_kinematic_collider_t *)phy_GetColliderPointer(collider_b);
    
    data_a.shape = kinematic_collider->base.shape;
    data_a.position = kinematic_collider->base.position;
    data_a.rotation = kinematic_collider->base.rotation;
    data_a.velocity = kinematic_collider->velocity;
    
    data_b.shape = static_collider->base.shape;
    data_b.position = static_collider->base.position;
    data_b.rotation = static_collider->base.rotation;
    data_b.velocity = vec2_t_c(0.0, 0.0);
    
    if(phy_contact_functions[static_collider->base.shape.type][kinematic_collider->base.shape.type](&data_a, &data_b, &contact_point))
    {
        vec2_t_mul(&parallel_velocity, &contact_point.normal, vec2_t_dot(&contact_point.normal, &kinematic_collider->velocity));
        vec2_t_sub(&kinematic_collider->velocity, &kinematic_collider->velocity, &parallel_velocity);
        kinematic_collider->base.position.x += contact_point.step_back.x;
        kinematic_collider->base.position.y += contact_point.step_back.y;
    }
    
//    phy_MoveCollider(collider_b);
}

uint32_t phy_ContactBoxBox(struct phy_collision_data_t *data_a, struct phy_collision_data_t *data_b, struct phy_contact_point_t *contact)
{
//    struct phy_box_shape_t *box_shape_a;
//    struct phy_box_shape_t *box_shape_b;
//    
//    vec2_t axes[2];
//    float a_extents[2];
//    float b_extents[2];
////    vec2_t a_extents[2];
//    vec2_t b_extents;
//    vec2_t diag;
//    float b_dist;
//    
//    vec2_t ab_vec;
//    vec2_t_sub(&ab_vec, &data_b->position, &data_a->position);
//    
//    box_shape_a = (struct phy_box_shape_t*)phy_GetShapePointer(data_a->shape);
//    box_shape_b = (struct phy_box_shape_t*)phy_GetShapePointer(data_b->shape);
//    
//    axes[0].x = box_shape_a->base.cos * box_shape_a->extents.x - box_shape_a->base.sin * box_shape_a->base.sin;
//    axes[0].y = box_shape_a->base.sin * box_shape_a->extents.x + box_shape_a->base.cos * box_shape_a->extents.y;
//    axes[1].x = axes[0].y;
//    axes[1].y = -axes[0].x;
//    
//    a_extents[0] = box_shape_a->extents.x;
//    a_extents[1] = box_shape_a->extents.y;
//    
//    diag.x = box_shape_b->base.cos * box_shape_b->extents.x + box_shape_b->base.sin * box_shape_b->extents.x;
//    diag.y = box_shape_b->base.sin * box_shape_b->extents.y + box_shape_b->base.cos * box_shape_b->extents.y;
//    
//    for(uint32_t index = 0; index < 2; index++)
//    {
//        b_dist = vec2_t_dot(&axes[index], &ab_vec);
//                
//        b_extents[0] = vec2_t_dot(&axes[index], &diag) + b_dist;
//        diag.x = -diag.x;
//        diag.y = -diag.y;
//        b_extents[1] = vec2_t_dot(&axes[index], &diag) + b_dist;
//        
//        if(!(a_extents[0] > b_extents[1] && a_extents[1]))
//    }
}

void phy_SupportEdge(struct phy_box_shape_t *box, vec2_t *direction, vec2_t *points, vec2_t *normal)
{
//    float area = 1.0 + box->extents.x + box->extents.y * 2.0;
//    vec2_t p = vec2_t_c(direction->x * area, direction->y * area);
    vec2_t p;
    vec2_t n;
    vec2_t_normalize(&p, direction);
    vec2_t_mul(&p, &p, sqrt(box->extents.x * box->extents.x + box->extents.y * box->extents.y));
    p = vec2_t_c(p.x * box->base.cos + p.y * box->base.sin, -p.x * box->base.sin + p.y * box->base.cos);
    
    
    if(p.x > box->extents.x)
    {
        p.y *= box->extents.x / p.x;
        p.x = box->extents.x;
    }
    else if(p.x < -box->extents.x)
    {
        p.y *= -box->extents.x / p.x;
        p.x = -box->extents.x;
    }
    else if(p.y > box->extents.y)
    {
        p.x *= box->extents.y / p.y;
        p.y = box->extents.y;
    }
    else if(p.y < -box->extents.y)
    {
        p.x *= -box->extents.y / p.y;
        p.y = -box->extents.y;
    }
    
//    if(fabs(p.x) > fabs(p.y))
    if(fabs(p.y) < box->extents.y)
    {
        if(p.x > 0.0)
        {
            points[0] = vec2_t_c(box->extents.x, -box->extents.y);
            points[1] = vec2_t_c(box->extents.x, box->extents.y);
            n = vec2_t_c(1.0, 0.0);
        }
        else
        {
            points[0] = vec2_t_c(-box->extents.x, box->extents.y);
            points[1] = vec2_t_c(-box->extents.x, -box->extents.y);
            n = vec2_t_c(-1.0, 0.0);
        }
    }
    else
    {
        if(p.y > 0.0)
        {
            points[0] = vec2_t_c(box->extents.x, box->extents.y);
            points[1] = vec2_t_c(-box->extents.x, box->extents.y);
            n = vec2_t_c(0.0, 1.0);
        }
        else
        { 
            points[0] = vec2_t_c(-box->extents.x, -box->extents.y);
            points[1] = vec2_t_c(box->extents.x, -box->extents.y);
            n = vec2_t_c(0.0, -1.0);
        }
    }
    
    points[0] = vec2_t_c(points[0].x * box->base.cos + points[0].y * box->base.sin, -points[0].x * box->base.sin + points[0].y * box->base.cos);
    points[1] = vec2_t_c(points[1].x * box->base.cos + points[1].y * box->base.sin, -points[1].x * box->base.sin + points[1].y * box->base.cos);
    *normal = vec2_t_c(n.x * box->base.cos + n.y * box->base.sin, -n.x * box->base.sin + n.y * box->base.cos);
}

void phy_SupportVertexBox(struct phy_box_shape_t *box, vec2_t *direction, vec2_t *point, vec2_t *normal)
{
//    vec2_t p;
//    vec2_t n;
//    float area = 1.0 + box->extents.x + box->extents.y * 2.0;
//    float extents[4] = {-box->extents.x, box->extents.x, -box->extents.y, box->extents.y};
//    p.x = direction->x * area;
//    p.y = direction->y * area;
//    p = vec2_t_c(p.x * box->base.cos + p.y * box->base.sin, -p.x * box->base.sin + p.y * box->base.cos);
//
//    if(fabs(p.x) > fabs(p.y))
//    {
//        if(p.x > 0.0) n = vec2_t_c(1.0, 0.0);
//        else n = vec2_t_c(-1.0, 0.0);
//    }
//    else
//    {
//        if(p.y > 0.0) n = vec2_t_c(0.0, 1.0);
//        else n = vec2_t_c(0.0, -1.0);
//    }
//
//    p.x = extents[p.x >= 0];
//    p.y = extents[(p.y >= 0) + 2];
//
//    *normal = vec2_t_c(n.x * box->base.cos + n.y * box->base.sin, -n.x * box->base.sin + n.y * box->base.cos);
//    *point = vec2_t_c(p.x * box->base.cos + p.y * box->base.sin, -p.x * box->base.sin + p.y * box->base.cos);
}












