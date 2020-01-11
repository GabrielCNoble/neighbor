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
    G_ENTITY_TYPE_NONE,
};



void g_Init();

void g_Shutdown();

// union entity_handle_t g_CreateEntity(char *name, vec3_t *position, mat3_t *orientation, uint32_t type);

union entity_handle_t g_CreatePlayerEntity(char *name, vec3_t *position, mat3_t *orientation);

void g_DestroyEntity(union entity_handle_t handle);

// struct entity_t *g_GetEntityPointer(union entity_handle_t handle);

void g_SetPlayer(union entity_handle_t handle);

void g_PlayerInput();

void g_MonsterInput();

void g_UpdatePlayer(union entity_handle_t handle);

void g_UpdateEntities();

void g_DrawFrame();

void g_DrawWorld(mat4_t *view_projection_matrix, mat4_t *view_matrix);

#endif