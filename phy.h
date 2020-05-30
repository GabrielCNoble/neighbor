#ifndef PHY_H
#define PHY_H

#include "lib/dstuff/ds_vector.h"
#include <stdint.h>

enum PHY_SHAPE_TYPES
{
    PHY_SHAPE_TYPE_BOX = 0,
    PHY_SHAPE_TYPE_LAST
};

struct phy_shape_t
{
    uint32_t type;
    float sin;
    float cos;
};

struct phy_box_shape_t
{
    struct phy_shape_t base;
    vec2_t extents;
};

struct phy_shape_h
{
    uint16_t index;
    uint16_t type;
};

#define PHY_INVALID_SHAPE_INDEX 0xffff
#define PHY_SHAPE_HANDLE(index, type)(struct phy_shape_h){index, type}
#define PHY_INVALID_SHAPE_HANDLE PHY_SHAPE_HANDLE(PHY_INVALID_SHAPE_INDEX, PHY_SHAPE_TYPE_LAST)

enum PHY_COLLIDER_TYPES
{
    PHY_COLLIDER_TYPE_STATIC = 0,
    PHY_COLLIDER_TYPE_DYNAMIC,
    PHY_COLLIDER_TYPE_KINEMATIC,
    PHY_COLLIDER_TYPE_LAST,
};

struct phy_collider_t
{
    vec2_t position;
    float rotation;
    uint32_t type;
    struct phy_shape_h shape;
    uint32_t dbvh_node;
    uint32_t step;
    uint32_t active_index;
    uint16_t first_contact;
    uint16_t contact_count;
};

struct phy_static_collider_t
{
    struct phy_collider_t base;
};

struct phy_dynamic_collider_t
{
    struct phy_collider_t base;
    vec2_t linear_velocity;
    float angular_velocity;
};

struct phy_kinematic_collider_t
{
    struct phy_collider_t base;
    vec2_t velocity;
};

struct phy_collider_h
{
    uint16_t index;
    uint16_t type;
};

#define PHY_INVALID_COLLIDER_INDEX 0xffff
#define PHY_COLLIDER_HANDLE(index, type) (struct phy_collider_h){index, type}
#define PHY_INVALID_COLLIDER_HANDLE PHY_COLLIDER_HANDLE(PHY_INVALID_COLLIDER_INDEX, PHY_COLLIDER_TYPE_LAST)



struct phy_collision_pair_t
{
    struct phy_collider_h collider_a;
    struct phy_collider_h collider_b;
};

struct phy_contact_point_t
{
    struct phy_collider_h collider;
    vec2_t relative_pos;
    vec2_t normal;
    float penetration;
};

struct phy_collision_data_t
{
    struct phy_shape_h shape;
    vec2_t position;
    vec2_t velocity;
    float rotation;
};


void phy_Init();

void phy_Shutdown();



struct phy_collider_h phy_CreateCollider(uint32_t type, struct phy_shape_h shape);

struct phy_collider_h phy_CreateDynamicCollider(struct phy_shape_h shape_handle);

struct phy_collider_h phy_CreateStaticCollider(struct phy_shape_h shape_handle);

struct phy_collider_t *phy_GetColliderPointer(struct phy_collider_h collider_handle);

void phy_AddColliderToWorld(struct phy_collider_h collider_handle);

void phy_SetColliderPosition(struct phy_collider_h collider_handle, vec2_t *position);

void phy_SetColliderVelocity(struct phy_collider_h collider_handle, vec2_t *velocity);



struct phy_shape_h phy_CreateShape(uint32_t type);

struct phy_shape_h phy_CreateBoxShape(vec2_t *extents);

struct phy_shape_t *phy_GetShapePointer(struct phy_shape_h shape_handle);

vec2_t phy_GetColliderExtends(struct phy_collider_h collider_handle);


void phy_Step(float delta_time);

void phy_IntersectNodes(struct phy_collider_h collider_handle);

void phy_CollideStaticDynamic(struct phy_collider_h collider_a, struct phy_collider_h collider_b);

uint32_t phy_ContactBoxBox(struct phy_collision_data_t *data_a, struct phy_collision_data_t *data_b, struct phy_contact_point_t *contact);

void phy_SupportVertexBox(struct phy_box_shape_t *box, vec2_t *direction, vec2_t *point, vec2_t *normal);

#endif








