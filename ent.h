#ifndef ENT_H
#define ENT_H

#include <stdint.h>
#include "dstuff/math/vector.h"
#include "dstuff/math/matrix.h"

// enum G_ENTITY_TYPE
// {
//     G_ENTITY_TYPE_PLAYER,
//     G_ENTITY_TYPE_MONSTER,
//     G_ENTITY_TYPE_LAST,
// };

enum ENT_ENTITY_MOVE_FLAGS
{
    ENT_ENTITY_MOVE_FLAG_WALK_FORWARD = 1,
    ENT_ENTITY_MOVE_FLAG_WALK_BACKWARD = 1 << 1,
    ENT_ENTITY_MOVE_FLAG_STRAFE_LEFT = 1 << 2,
    ENT_ENTITY_MOVE_FLAG_STRAFE_RIGHT = 1 << 3,
    ENT_ENTITY_MOVE_FLAG_JUMP = 1 << 4,
    ENT_ENTITY_MOVE_FLAG_ON_AIR = 1 << 5
};

struct entity_prop_t
{
    struct entity_prop_t *next;
    char *name;
    uint32_t size;
    void *data;
};

struct entity_t
{
    char *name;
    uint32_t type;
    uint32_t move_flags;
    mat4_t transform;
    struct entity_prop_t *props;
};

union entity_handle_t
{
    uint32_t index;
    void *punned;
};

#define ENT_INVALID_ENTITY_INDEX 0xffffffff
#define ENT_ENTITY_HANDLE(index) (union entity_handle_t ){index}
#define ENT_INVALID_ENTITY_HANDLE ENT_ENTITY_HANDLE(ENT_INVALID_ENTITY_INDEX)

void ent_Init();

void ent_Shutdown();

union entity_handle_t ent_CreateEntity(char* name, vec3_t* position, mat3_t* orientation, uint32_t type);

void ent_DestroyEntity(union entity_handle_t handle);

struct entity_t* ent_GetEntityPointer(union entity_handle_t handle);

struct entity_prop_t* ent_AddProp(union entity_handle_t handle, char* name, uint32_t size);

void ent_RemoveProp(union entity_handle_t handle, char* name);

void ent_RemoveAllProps(union entity_handle_t handle);

struct entity_prop_t* ent_GetProp(union entity_handle_t handle, char* name);

void *ent_GetPropData(union entity_handle_t handle, char* name);

struct stack_list_t* ent_GetEntityList();


#endif 