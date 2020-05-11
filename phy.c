#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "phy.h"
#include "lib/dstuff/containers/stack_list.h"
#include "lib/dstuff/accel_structs/dbvh.h"



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

struct dbvh_tree_t phy_dbvh;
uint32_t phy_step = 0;

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

void phy_AddColliderToWorld(struct phy_collider_h collider_handle)
{
    struct phy_collider_t *collider;
    struct dbvh_node_t *node;
    vec2_t extents;
    collider = phy_GetColliderPointer(collider_handle);
    if(collider && collider->active_index == 0xffffffff)
    {
        collider->active_index = add_list_element(&phy_active_colliders, &collider_handle);
        if(collider->dbvh_node == INVALID_DBVH_NODE_INDEX)
        {
            collider->dbvh_node = alloc_dbvh_node(&phy_dbvh);
        }
        extents = phy_GetColliderExtends(collider_handle);
        extents.x *= 0.5;
        extents.y *= 0.5;
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
        collider->velocity = *velocity;
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

    for(uint32_t step_index = 0; step_index < PHY_STEP_COUNT; step_index++)
    {
        phy_collision_pairs.cursor = 0;
        phy_contact_points.cursor = 0;
        for(uint32_t collider_index = 0; collider_index < phy_active_colliders.cursor; collider_index++)
        {
            collider_handle = get_list_element(&phy_active_colliders, collider_index);
            phy_GenerateCollisionPairs(*collider_handle);
        }

        for(uint32_t pair_index = 0; pair_index < phy_collision_pairs.cursor; pair_index++)
        {
            struct phy_collision_pair_t *pair = get_list_element(&phy_collision_pairs, pair_index);
            struct phy_collider_t *collider_a = phy_GetColliderPointer(pair->collider_a);
            struct phy_collider_t *collider_b = phy_GetColliderPointer(pair->collider_b);
            struct phy_dynamic_collider_t *dynamic_collider;
            struct phy_static_collider_t *static_collider;

            if(collider_a->type == PHY_COLLIDER_TYPE_DYNAMIC)
            {
                dynamic_collider = collider_a;
                static_collider = collider_b;
            }
            else
            {
                dynamic_collider = collider_b;
                static_collider = collider_a;
            }

            struct phy_box_shape_t *shape_a = phy_GetShapePointer(dynamic_collider->base.shape);
            struct phy_box_shape_t *shape_b = phy_GetShapePointer(static_collider->base.shape);

            if(dynamic_collider->base.position.y - shape_a->extents.y * 0.5 < static_collider->base.position.y + shape_b->extents.y * 0.5)
            {
                dynamic_collider->base.position.y = static_collider->base.position.y + shape_b->extents.y * 0.5 + shape_a->extents.y * 0.5;
                dynamic_collider->velocity.y = 0.0;
            }
        }

        for(uint32_t collider_index = 0; collider_index < phy_active_colliders.cursor; collider_index++)
        {
            collider_handle = get_list_element(&phy_active_colliders, collider_index);
            collider = phy_GetColliderPointer(*collider_handle);
            switch(collider->type)
            {
                case PHY_COLLIDER_TYPE_DYNAMIC:
                    {
                        struct phy_dynamic_collider_t *dynamic_collider = (struct phy_dynamic_collider_t *)collider;
                        dynamic_collider->base.position.x += dynamic_collider->velocity.x;
                        dynamic_collider->base.position.y += dynamic_collider->velocity.y;
                        dynamic_collider->velocity.y -= 0.0098 * delta_time;
                    }
                break;
            }
        }

        phy_step++;
    }
}

void phy_GenerateCollisionPairsRec(struct dbvh_node_t *node, uint32_t node_index, uint32_t cur_node_index, uint32_t *smallest_index, float *smallest_area)
{
    struct dbvh_node_t *cur_node;
    struct phy_collision_pair_t *pair;
    struct phy_collider_t *collider;
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
                    memcpy(&pair->collider_a, node->user_data, sizeof(struct phy_collider_h));
                    memcpy(&pair->collider_b, cur_node->user_data, sizeof(struct phy_collider_h));
                }
            }
            else
            {
                phy_GenerateCollisionPairsRec(node, node_index, cur_node->children[0], smallest_index, smallest_area);
                phy_GenerateCollisionPairsRec(node, node_index, cur_node->children[1], smallest_index, smallest_area);
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

void phy_GenerateCollisionPairs(struct phy_collider_h collider_handle)
{
    struct phy_collider_t *collider;
    struct dbvh_node_t *node;
    float smallest_area = FLT_MAX;
    uint32_t smallest_index = INVALID_DBVH_NODE_INDEX;
    vec2_t extents;

    collider = phy_GetColliderPointer(collider_handle);
    node = get_dbvh_node_pointer(&phy_dbvh, collider->dbvh_node);
    extents = phy_GetColliderExtends(collider_handle);
    extents.x *= 0.5;
    extents.y *= 0.5;

    node->max.x = collider->position.x + extents.x;
    node->max.y = collider->position.y + extents.y;
    node->max.z = 0.2;

    node->min.x = collider->position.x - extents.x;
    node->min.y = collider->position.y - extents.y;
    node->min.z = -0.2;

    phy_GenerateCollisionPairsRec(node, collider->dbvh_node, phy_dbvh.root, &smallest_index, &smallest_area);
    if(smallest_index != INVALID_DBVH_NODE_INDEX)
    {
        pair_dbvh_nodes(&phy_dbvh, collider->dbvh_node, smallest_index);
    }
    collider->step = phy_step;
}














