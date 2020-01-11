#include "ent.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/math/vector.h"
#include "dstuff/math/matrix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct stack_list_t entities;

void ent_Init()
{
    entities = create_stack_list(sizeof(struct entity_t), 128);
}

void ent_Shutdown()
{
    // destroy_stack_list(&entities);
}

union entity_handle_t ent_CreateEntity(char* name, vec3_t* position, mat3_t* orientation, uint32_t type)
{
    union entity_handle_t handle;
    struct entity_t* entity;
    handle.index = add_stack_list_element(&entities, NULL);
    entity = (struct entity_t*)get_stack_list_element(&entities, handle.index);
    entity->name = strdup(name);
    entity->transform = mat4_t(*orientation, *position);
    entity->props = NULL;
    entity->type = type;
    entity->move_flags = 0;
    return handle;
}

void ent_DestroyEntity(union entity_handle_t handle)
{
    struct entity_t* entity;
    entity = ent_GetEntityPointer(handle);
    if(entity)
    {
        free(entity->name);
        entity->name = NULL;
        entity->type = 0;
        ent_RemoveAllProps(handle);
        remove_stack_list_element(&entities, handle.index);
    }
}

struct entity_t* ent_GetEntityPointer(union entity_handle_t handle)
{
    struct entity_t* entity;
    entity = (struct entity_t*)get_stack_list_element(&entities, handle.index);
    if(entity && !entity->type)
    {
        entity = NULL;
    }
    return entity;
}

struct entity_prop_t* ent_AddProp(union entity_handle_t handle, char* name, uint32_t size)
{
    struct entity_prop_t* prop = NULL;
    struct entity_t* entity;
    entity = ent_GetEntityPointer(handle);
    if(entity && !ent_GetProp(handle, name))
    {
        prop = (struct entity_prop_t*)calloc(1, sizeof(struct entity_prop_t) + size);
        prop->name = strdup(name);
        prop->next = entity->props;
        prop->data = (char*)prop + sizeof(struct entity_prop_t);
        entity->props = prop;
    }
    return prop;
}

void ent_RemoveProp(union entity_handle_t handle, char* name)
{
    struct entity_prop_t* prop = NULL;
    struct entity_prop_t* prev = NULL;
    struct entity_t* entity;
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

void ent_RemoveAllProps(union entity_handle_t handle)
{
    struct entity_t* entity;
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

struct entity_prop_t* ent_GetProp(union entity_handle_t handle, char* name)
{
    struct entity_prop_t* prop = NULL;
    struct entity_t* entity;
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

void *ent_GetPropData(union entity_handle_t handle, char* name)
{
    struct entity_prop_t* prop;
    prop = ent_GetProp(handle, name);
    if(prop)
    {
        return prop->data;
    }
    return NULL;
}

struct stack_list_t* ent_GetEntityList()
{
    return &entities;
}