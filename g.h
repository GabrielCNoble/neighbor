#ifndef G_H
#define G_H

#include "dstuff/math/vector.h"
#include "dstuff/math/matrix.h"
#include "dstuff/physics/physics.h"
#include "mdl.h"
#include "ent.h"

enum G_ENTITY_TYPE
{
    G_ENTITY_TYPE_PLAYER = 1,
    G_ENTITY_TYPE_PROP,
    G_ENTITY_TYPE_PLATFORM,
};
struct player_entity_props_t
{
    float pitch;
    float yaw;
    float gun_x;
    float gun_y;
    float gun_z;
    float gun_dx;
    float gun_dy; 
    struct collider_handle_t collider;
    union entity_handle_t gun;
};

void g_Init();

void g_Shutdown();

union entity_handle_t g_CreatePlayer(char *name, vec3_t *position, mat3_t *orientation);

union entity_handle_t g_CreateProp(char* name, vec3_t* position, mat3_t* orientation);

// union entity_handle_t g_CreatePlatform(char* name, vec3_t* position, mat3_t* orientation);

void g_DestroyEntity(union entity_handle_t handle);

void g_SetPlayer(union entity_handle_t handle);

void g_UpdatePlayer(union entity_handle_t handle);

void g_UpdateEntities();

void g_DrawFrame();

void g_DrawWorld(mat4_t *view_projection_matrix, mat4_t *view_matrix);

#endif