#define SDL_MAIN_HANDLED
#include "SDL/include/SDL2/SDL.h"
#include "SDL/include/SDL2/SDL_syswm.h"
// #include "Windows.h"

#include "r.h"
#include "in.h"
#include "mdl.h"
#include "ent.h"
#include "g.h"
#include "dstuff/loaders/bsp.h"
#include "dstuff/math/geometry.h"
#include "dstuff/math/matrix.h"
#include "dstuff/math/vector.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


struct thing_t
{
    mat4_t transform;
    struct model_handle_t model;
    // struct r_alloc_handle_t handle;
    // struct r_material_handle_t material;
};

extern struct r_renderer_t r_renderer;

void draw_things(struct thing_t *things, uint32_t thing_count)
{
    struct model_t *model;
    struct model_batch_t *batch;
    struct r_material_t *material;

    r_BeginSubmission(&r_renderer.view_projection_matrix, &r_renderer.view_matrix);
    for(uint32_t i = 0; i < thing_count; i++)
    {
        model = mdl_GetModelPointer(things[i].model);
        // printf("before drawing thing %d\n", i);
        for(uint32_t j = 0; j < model->batch_count; j++)
        {
            // printf("before submit\n");
            batch = model->batches + j;
            r_SubmitDrawCmd(&things[i].transform, batch->material, batch->range.start, batch->range.count);
            // printf("after submit\n");
        }
        // printf("after drawing thing %d\n", i);
    }
    r_EndSubmission();
}

#define CAMERA_SPEED 1.0

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL error. %s\n", SDL_GetError());
        return -1;
    }


    r_InitRenderer();
    mdl_Init();
    ent_Init();
    g_Init();

//    struct r_texture_handle_t logo = r_LoadTexture("logo_fuck.png", "logo_fuck");
//    struct r_texture_handle_t logo2 = r_LoadTexture("logo2.png", "logo2");
//    struct r_texture_handle_t doggo = r_LoadTexture("doggo.png", "doggo");
    // struct r_alloc_handle_t quad;
    // struct r_material_handle_t material_handle;
//    struct r_material_t *material;
    // struct vertex_t *vertices;
    // struct geometry_data_t data;
    // struct batch_data_t *batch;
//    struct model_handle_t model;
//    struct model_handle_t gun;
//    mat4_t projection_matrix;
//    mat4_t view_matrix;
//    mat4_t pitch_matrix;
//    mat4_t yaw_matrix;
    struct thing_t things[1];
//    vec3_t vec3;
//    vec2_t vec2;

    things[0].model = mdl_LoadModel("q3dm9.bsp");
    mat4_t_identity(&things[0].transform);
    things[0].transform.comps[3][2] = -5.0;


    // printf("load world\n");
    // mat4_t_identity(&things[0].transform);
    // things[0].transform.vcomps[3].comps[2] = -5.0;

    // things[1].model = mdl_LoadModel("cube2.obj");
    // mat4_t_identity(&things[1].transform);
    // things[1].transform.vcomps[3].comps[1] = 5.0;


    // things[1].handle = quad;
    // mat4_t_identity(&things[1].transform);
    // things[1].transform.vcomps[3].comps[2] = -15.0;

    // things[2].handle = quad;
    // mat4_t_identity(&things[2].transform);
    // mat4_t_yaw(&things[2].transform, 0.2);
    // things[2].transform.vcomps[3].comps[2] = -7.0;
    // things[2].transform.vcomps[3].comps[0] = -6.0;

    SDL_ShowCursor(0);

    mat3_t orientation;
    vec3_t position = vec3_t_c(0.0, 0.0, 0.0);
    mat3_t_identity(&orientation);
    union entity_handle_t player = g_CreatePlayer("main player", &position, &orientation);
    g_SetPlayer(player);

    position.z = -3.5;
    g_CreatePlatform("FUCK", &position, &orientation);

//    position.y = 10.0;
//    g_CreatePlatform("ASS", &position, &orientation);

    while(1)
    {
        in_ReadInput();
        g_UpdateEntities();
        r_QueueCmd(R_CMD_TYPE_BEGIN_FRAME, NULL, 0);
        draw_things(things, sizeof(things) / sizeof(things[0]));
        ent_DrawEntities();
        r_QueueCmd(R_CMD_TYPE_END_FRAME, NULL, 0);
        r_WaitEmptyQueue();
    }

    return 0;
}
