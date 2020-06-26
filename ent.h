#ifndef ENT_H
#define ENT_H

#include <stdint.h>
#include "lib/dstuff/ds_vector.h"
#include "lib/dstuff/ds_matrix.h"
#include "mdl.h"
struct entity_prop_t
{
    struct entity_prop_t *next;
    char *name;
    uint32_t size;
    void *data;
};

struct ent_entity_t
{
    char *name;
    uint32_t type;
    // uint32_t node;
    mat4_t transform;
    struct mdl_model_h model;
    struct entity_prop_t *props;
};

union ent_entity_h
{
    uint32_t index;
    void *punned;
};

#define ENT_INVALID_ENTITY_INDEX 0xffffffff
#define ENT_ENTITY_HANDLE(index) (union ent_entity_h ){index}
#define ENT_INVALID_ENTITY_HANDLE ENT_ENTITY_HANDLE(ENT_INVALID_ENTITY_INDEX)

void ent_Init();

void ent_Shutdown();

union ent_entity_h ent_CreateEntity(char* name, vec3_t* position, mat3_t* orientation, uint32_t type);

void ent_DestroyEntity(union ent_entity_h handle);

struct ent_entity_t *ent_GetEntityPointer(union ent_entity_h handle);

 void ent_SetEntityModel(union ent_entity_h handle, struct mdl_model_h model);

struct entity_prop_t* ent_AddProp(union ent_entity_h handle, char* name, uint32_t size);

void ent_RemoveProp(union ent_entity_h handle, char* name);

void ent_RemoveAllProps(union ent_entity_h handle);

struct entity_prop_t *ent_GetProp(union ent_entity_h handle, char* name);

void *ent_GetPropData(union ent_entity_h handle, char* name);

struct stack_list_t *ent_GetEntityList();

void ent_DrawEntities();


#endif
