#ifndef ENT_H
#define ENT_H

#include <stdint.h>
#include "dstuff/math/vector.h"
#include "dstuff/math/matrix.h"
#include "mdl.h"
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
    // uint32_t node;
    mat4_t transform;
    struct model_handle_t model;
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

// void ent_SetEntityModel(union entity_handle_t handle, struct model_handle_t model);

struct entity_prop_t* ent_AddProp(union entity_handle_t handle, char* name, uint32_t size);

void ent_RemoveProp(union entity_handle_t handle, char* name);

void ent_RemoveAllProps(union entity_handle_t handle);

struct entity_prop_t* ent_GetProp(union entity_handle_t handle, char* name);

void *ent_GetPropData(union entity_handle_t handle, char* name);

struct stack_list_t* ent_GetEntityList();

void ent_DrawEntities();


#endif
