#include "ent.h"
#include "lib/dstuff/ds_stack_list.h"
#include "lib/dstuff/ds_dbvh.h"
#include "lib/dstuff/ds_vector.h"
#include "lib/dstuff/ds_matrix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct stack_list_t ent_entities;

void ent_Init()
{
    ent_entities = create_stack_list(sizeof(struct ent_entity_t), 128);
}

void ent_Shutdown()
{
}

union ent_entity_h ent_CreateEntity(char* name, vec3_t* position, mat3_t* orientation, uint32_t type)
{
    union ent_entity_h handle;
    struct ent_entity_t *entity;
    handle.index = add_stack_list_element(&ent_entities, NULL);
    entity = (struct entity_t*)get_stack_list_element(&ent_entities, handle.index);
    entity->name = strdup(name);
    mat4_t_comp(&entity->transform, orientation, position);
    entity->props = NULL;
    entity->type = type;
    entity->model = MDL_INVALID_MODEL_HANDLE;
    // entity->node = INVALID_DBVH_NODE_INDEX;
    return handle;
}

void ent_DestroyEntity(union ent_entity_h handle)
{
    struct ent_entity_t *entity;
    entity = ent_GetEntityPointer(handle);
    if(entity)
    {
        free(entity->name);
        entity->name = NULL;
        entity->type = 0;
        ent_RemoveAllProps(handle);
        remove_stack_list_element(&ent_entities, handle.index);
    }
}

struct ent_entity_t *ent_GetEntityPointer(union ent_entity_h handle)
{
    struct ent_entity_t *entity;
    entity = (struct entity_t*)get_stack_list_element(&ent_entities, handle.index);
//    if(entity && !entity->type)
//    {
//        entity = NULL;
//    }
    return entity;
}

 void ent_SetEntityModel(union ent_entity_h handle, struct mdl_model_h model)
 {
     struct ent_entity_t *entity;
     entity = ent_GetEntityPointer(handle);
     if(entity)
     {
         entity->model = model;
//         if(entity->node == INVALID_DBVH_NODE_INDEX)
//         {
//             entity->node = alloc_dbvh_node(&dbvh);
//
//         }
     }
 }

struct entity_prop_t *ent_AddProp(union ent_entity_h handle, char* name, uint32_t size)
{
    struct entity_prop_t* prop = NULL;
    struct ent_entity_t *entity;
    entity = ent_GetEntityPointer(handle);
    if(entity && !ent_GetProp(handle, name))
    {
        prop = (struct entity_prop_t*)calloc(1, sizeof(struct entity_prop_t) + size);
        prop->name = strdup(name);
        prop->next = entity->props;
        prop->data = (char*)prop + sizeof(struct entity_prop_t);
        prop->size = size;
        entity->props = prop;
    }
    return prop;
}

void ent_RemoveProp(union ent_entity_h handle, char* name)
{
    struct entity_prop_t* prop = NULL;
    struct entity_prop_t* prev = NULL;
    struct ent_entity_t *entity;
    entity = ent_GetEntityPointer(handle);
    if(entity)
    {
        prop = entity->props;
        while(prop)
        {
            if(!strcmp(prop->name, name))
            {
                if(!prev)
                {
                    entity->props = prop->next;
                }
                else
                {
                    prev->next = prop->next;
                }
                break;
            }
            prev = prop;
            prop = prop->next;
        }
    }
}

void ent_RemoveAllProps(union ent_entity_h handle)
{
    struct ent_entity_t *entity;
    struct entity_prop_t* prop;
    struct entity_prop_t* next;
    entity = ent_GetEntityPointer(handle);
    if(entity)
    {
        prop = entity->props;
        while(prop)
        {
            next = prop->next;
            free(prop->name);
            free(prop);
            prop = next;
        }
    }
}

struct entity_prop_t* ent_GetProp(union ent_entity_h handle, char* name)
{
    struct entity_prop_t* prop = NULL;
    struct ent_entity_t *entity;
    entity = ent_GetEntityPointer(handle);
    if(entity)
    {
        prop = entity->props;
        while(prop)
        {
            if(!strcmp(prop->name, name))
            {
                break;
            }

            prop = prop->next;
        }
    }

    return prop;
}

void *ent_GetPropData(union ent_entity_h handle, char* name)
{
    struct entity_prop_t* prop;
    prop = ent_GetProp(handle, name);
    if(prop)
    {
        return prop->data;
    }
    return NULL;
}

struct stack_list_t *ent_GetEntityList()
{
    return &ent_entities;
}
