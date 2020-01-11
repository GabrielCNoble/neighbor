#include "g.h"
#include "in.h"
#include "dstuff/containers/stack_list.h"
#include "r_main.h"
#include <stdio.h>

// struct stack_list_t entities[G_ENTITY_TYPE_LAST];
// struct stack_list_t entities;
union entity_handle_t player_handle;


void g_Init()
{
    player_handle = ENT_INVALID_ENTITY_HANDLE;

    in_RegisterKey(SDL_SCANCODE_W);
    in_RegisterKey(SDL_SCANCODE_S);
    in_RegisterKey(SDL_SCANCODE_A);
    in_RegisterKey(SDL_SCANCODE_D);
}

void g_Shutdown()
{
    
}

union entity_handle_t g_CreatePlayerEntity(char *name, vec3_t *position, mat3_t *orientation)
{
    union entity_handle_t handle;
    struct entity_t* player;

    handle = ent_CreateEntity(name, position, orientation, G_ENTITY_TYPE_PLAYER);
    ent_AddProp(handle, "pitch", sizeof(float));
    ent_AddProp(handle, "yaw", sizeof(float));
    ent_AddProp(handle, "life", sizeof(float));
    ent_AddProp(handle, "gun_x", sizeof(float));
    ent_AddProp(handle, "gun_y", sizeof(float));
    ent_AddProp(handle, "player_model", sizeof(struct model_handle_t));
    ent_AddProp(handle, "gun_model", sizeof(struct model_handle_t));

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

void g_PlayerInput()
{
    struct entity_t *player;
    float dx;
    float dy;
    float *pitch;
    float *yaw;
    struct entity_prop_t *prop;
    player = ent_GetEntityPointer(player_handle);
    mat4_t pitch_matrix;
    mat4_t yaw_matrix;
    mat4_t view_matrix;

    if(player && player->type == G_ENTITY_TYPE_PLAYER)
    {
        in_GetMouseDelta(&dx, &dy);

        pitch = (float *)ent_GetPropData(player_handle, "pitch");
        yaw = (float *)ent_GetPropData(player_handle, "yaw");

        *pitch += dy * 0.1;
        *yaw -= dx * 0.1;

        if(*pitch > 0.5) *pitch = 0.5;
        else if(*pitch < -0.5) *pitch = -0.5; 

        if(*yaw > 1.0) *yaw = -2.0 + *yaw;
        else if(*yaw < -1.0) *yaw = 2.0 + *yaw;

        player->move_flags &= ~(ENT_ENTITY_MOVE_FLAG_WALK_FORWARD | 
                                ENT_ENTITY_MOVE_FLAG_WALK_BACKWARD |
                                ENT_ENTITY_MOVE_FLAG_STRAFE_LEFT | 
                                ENT_ENTITY_MOVE_FLAG_STRAFE_RIGHT);

        if(in_GetKeyState(SDL_SCANCODE_W) & IN_INPUT_STATE_PRESSED)
        {
            player->move_flags |= ENT_ENTITY_MOVE_FLAG_WALK_FORWARD;
        }

        if(in_GetKeyState(SDL_SCANCODE_S) & IN_INPUT_STATE_PRESSED)
        {
            player->move_flags |= ENT_ENTITY_MOVE_FLAG_WALK_BACKWARD;
        }

        if(in_GetKeyState(SDL_SCANCODE_A) & IN_INPUT_STATE_PRESSED)
        {
            player->move_flags |= ENT_ENTITY_MOVE_FLAG_STRAFE_LEFT;
        }

        if(in_GetKeyState(SDL_SCANCODE_D) & IN_INPUT_STATE_PRESSED)
        {
            player->move_flags |= ENT_ENTITY_MOVE_FLAG_STRAFE_RIGHT;
        }
    }
}

void g_MonsterInput()
{

}

void g_UpdatePlayer(union entity_handle_t handle)
{
    float *pitch;
    float *yaw;
    vec3_t movement = vec3_t(0.0);
    vec4_t forward_vec;
    vec4_t right_vec;
    mat4_t yaw_matrix;
    mat4_t pitch_matrix;
    mat4_t view_matrix;
    struct entity_t *player;

    player = ent_GetEntityPointer(handle);

    pitch = (float*)ent_GetPropData(handle, "pitch");
    yaw = (float*)ent_GetPropData(handle, "yaw");

    mat4_t_yaw(&yaw_matrix, *yaw);
    forward_vec = yaw_matrix.rows[2];
    right_vec = yaw_matrix.rows[0];

    if(player->move_flags & ENT_ENTITY_MOVE_FLAG_WALK_FORWARD)
    {
        movement.z -= 1.0;
    }

    if(player->move_flags & ENT_ENTITY_MOVE_FLAG_WALK_BACKWARD)
    {
        movement.z += 1.0;
    }

    if(player->move_flags & ENT_ENTITY_MOVE_FLAG_STRAFE_LEFT)
    {
        movement.x -= 1.0;
    }

    if(player->move_flags & ENT_ENTITY_MOVE_FLAG_STRAFE_RIGHT)
    {
        movement.x += 1.0;
    }

    forward_vec *= movement.z;
    right_vec *= movement.x;
    mat4_t_pitch(&pitch_matrix, *pitch);
    view_matrix = pitch_matrix * yaw_matrix;
    r_SetViewMatrix(&view_matrix);
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

    g_PlayerInput();
    g_MonsterInput();

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

void g_DrawFrame()
{

}

void g_DrawWorld(mat4_t *view_projection_matrix, mat4_t *view_matrix)
{

}