#include "g.h"
#include "in.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/physics/physics.h"
#include "r.h"
#include <stdio.h>

union entity_handle_t player_handle;

void g_Init()
{
    player_handle = ENT_INVALID_ENTITY_HANDLE;

    in_RegisterKey(SDL_SCANCODE_W);
    in_RegisterKey(SDL_SCANCODE_S);
    in_RegisterKey(SDL_SCANCODE_A);
    in_RegisterKey(SDL_SCANCODE_D);

    init_physics();

    mdl_LoadModel("gun.obj");
    mdl_LoadModel("platform.obj");
}

void g_Shutdown()
{
    
}

union entity_handle_t g_CreatePlayer(char *name, vec3_t *position, mat3_t *orientation)
{
    union entity_handle_t handle;
    struct entity_t* player;
    struct entity_t* gun;
    struct entity_prop_t* prop;
    struct player_entity_props_t* props; 
    struct player_collider_t* collider;

    handle = ent_CreateEntity(name, position, orientation, G_ENTITY_TYPE_PLAYER);

    prop = ent_AddProp(handle, "props", sizeof(struct player_entity_props_t));
    props = (struct player_entity_props_t*)prop->data;

    props->gun = g_CreateProp(name, position, orientation);
    gun = ent_GetEntityPointer(props->gun);
    gun->model = mdl_GetModelHandle("gun");

    props->gun_x = 1.1;
    props->gun_z = -3.2;
    props->gun_y = -0.5;

    props->collider = create_player_collider(position, 0.3, 1.7);
    add_collider_to_world(props->collider);
    return handle;
}

union entity_handle_t g_CreateProp(char* name, vec3_t* position, mat3_t* orientation)
{
    return ent_CreateEntity(name, position, orientation, G_ENTITY_TYPE_PROP);
}

union entity_handle_t g_CreatePlatform(char* name, vec3_t* position, mat3_t* orientation)
{
    // return ent_CreateEntity(name, position, orientation, G_ENTITY_TYPE_PLATFORM);
    union entity_handle_t handle;
    struct entity_t* entity;
    struct entity_prop_t* prop;
    struct platform_entity_props_t* props;
    struct static_collider_t* collider;
    struct model_t* model;
    uint32_t vert_count = 0;
    handle = ent_CreateEntity(name, position, orientation, G_ENTITY_TYPE_PLATFORM);
    entity = ent_GetEntityPointer(handle);
    entity->model = mdl_GetModelHandle("platform");
    prop = ent_AddProp(handle, "props", sizeof(struct platform_entity_props_t));
    props = (struct platform_entity_props_t*)prop->data;


    model = mdl_GetModelPointer(entity->model);
    props->collider = create_static_collider();
    for(uint32_t i = 0, c = model->batch_count / model->lod_count; i < c; i++)
    {
        vert_count += model->batches[i].range.count;
    }
    set_static_collider_triangles(props->collider, model->vertices, vert_count);
    add_collider_to_world(props->collider);

    return handle;
}

void g_DestroyEntity(union entity_handle_t handle)
{
    ent_DestroyEntity(handle);
}

void g_SetPlayer(union entity_handle_t handle)
{
    player_handle = handle;
}

void g_UpdatePlayer(union entity_handle_t handle)
{
    vec3_t movement = vec3_t(0.0);
    vec4_t position;
    vec4_t forward_vec;
    vec4_t right_vec;
    vec4_t gun_position;
    mat4_t yaw_matrix;
    mat4_t pitch_matrix;
    // mat4_t view_matrix;
    float dx;
    float dy;
    struct entity_t* player;
    struct entity_t* gun;
    struct player_entity_props_t* props;
    struct player_collider_t* collider;

    player = ent_GetEntityPointer(handle);
    props = (struct player_entity_props_t*)ent_GetPropData(handle, "props");
    gun = ent_GetEntityPointer(props->gun);
    in_GetMouseDelta(&dx, &dy);
    props->pitch += dy * 0.1;
    props->yaw -= dx * 0.1;

    props->gun_dx -= dx * 0.1;
    props->gun_dy -= dy * 0.1;

    if(props->pitch > 0.5) props->pitch = 0.5;
    else if(props->pitch < -0.5) props->pitch = -0.5; 

    if(props->yaw > 1.0) props->yaw = -2.0 + props->yaw;
    else if(props->yaw < -1.0) props->yaw = 2.0 + props->yaw;
    
    position = player->transform.rows[3];
    mat4_t_yaw(&yaw_matrix, props->yaw);
    mat4_t_pitch(&pitch_matrix, props->pitch);
    pitch_matrix *= yaw_matrix;
    forward_vec = pitch_matrix.rows[2];
    right_vec = pitch_matrix.rows[0];

    if(in_GetKeyState(SDL_SCANCODE_W) & IN_INPUT_STATE_PRESSED)
    {
        movement.z -= 1.0;
    }

    if(in_GetKeyState(SDL_SCANCODE_S) & IN_INPUT_STATE_PRESSED)
    {
        movement.z += 1.0;
    }

    if(in_GetKeyState(SDL_SCANCODE_A) & IN_INPUT_STATE_PRESSED)
    {
        movement.x -= 1.0;
    }

    if(in_GetKeyState(SDL_SCANCODE_D) & IN_INPUT_STATE_PRESSED)
    {
        movement.x += 1.0;
    }

    forward_vec *= movement.z;
    right_vec *= movement.x;
    player->transform = pitch_matrix;
    player->transform.rows[3] = position + forward_vec + right_vec;
    player->transform.rows[3].w = 1.0;
    r_SetViewMatrix(&player->transform);

    gun_position.x = props->gun_x + props->gun_dx;
    gun_position.y = props->gun_y + props->gun_dy;
    gun_position.z = props->gun_z;
    gun_position.w = 1.0;

    props->gun_dy *= 0.8;
    props->gun_dx *= 0.8;

    mat4_t_identity(&gun->transform);
    gun->transform.rows[3] = gun_position;
    gun->transform *= player->transform;


    collider = get_player_collider_pointer(props->collider);
    printf("[%f %f %f]\n", collider->base.position.x, 
                           collider->base.position.y, 
                           collider->base.position.z);
}

void g_UpdateEntities()
{
    struct entity_t *entity;
    struct stack_list_t *entities;
    vec3_t movement;
    vec4_t forward_vec;
    vec4_t right_vec;
    mat4_t view_matrix;
    mat4_t pitch_matrix;
    mat4_t yaw_matrix;

    step_physics();

    entities = ent_GetEntityList();
    for(uint32_t i = 0; i < entities->cursor; i++)
    {
        entity = ent_GetEntityPointer(ENT_ENTITY_HANDLE(i));
        if(entity)
        {
            switch(entity->type)
            {
                case G_ENTITY_TYPE_PLAYER:
                    g_UpdatePlayer(ENT_ENTITY_HANDLE(i));
                break;
            }
        }
    }
}