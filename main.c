#define SDL_MAIN_HANDLED
#include "SDL/include/SDL2/SDL.h"
#include "SDL/include/SDL2/SDL_syswm.h"
// #include "Windows.h"

#include "r_main.h"
#include "in.h"
#include "mdl.h"
#include "ent.h"
#include "g.h"
#include "dstuff/loaders/bsp.h"
#include "dstuff/math/geometry.h"

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

    struct r_texture_handle_t logo = r_LoadTexture("logo_fuck.png", "logo_fuck");
    struct r_texture_handle_t logo2 = r_LoadTexture("logo2.png", "logo2");
    struct r_texture_handle_t doggo = r_LoadTexture("doggo.png", "doggo");
    // struct r_alloc_handle_t quad;
    // struct r_material_handle_t material_handle;
    struct r_material_t *material;
    // struct vertex_t *vertices;
    // struct geometry_data_t data;
    // struct batch_data_t *batch;
    struct model_handle_t model;
    struct model_handle_t gun;
    mat4_t projection_matrix;
    mat4_t view_matrix;
    mat4_t pitch_matrix;
    mat4_t yaw_matrix;
    struct thing_t things[1];
    vec3_t vec3;
    vec2_t vec2;


    // vec3_t p = closest_point_on_triangle(&vec3_t_c(0.0, 5.0, 0.0), 
    //                                      &vec3_t_c(2.0, 0.0, -2.0),
    //                                      &vec3_t_c(-2.0, 0.0, -2.0),
    //                                      &vec3_t_c(0.0, 0.0, 2.0));
    

    // printf("[%f %f %f]\n", p.x, p.y, p.z);

    // load_wavefront("level0.obj", &data);
    // vertices = calloc(sizeof(struct vertex_t), data.vertices.cursor);
    // for(uint32_t i = 0; i < data.vertices.cursor; i++)
    // {
    //     vec3 = *(vec3_t *)get_list_element(&data.vertices, i);
    //     vertices[i].position.comps[0] = vec3.comps[0];
    //     vertices[i].position.comps[1] = vec3.comps[1];
    //     vertices[i].position.comps[2] = vec3.comps[2];
    //     vertices[i].position.comps[3] = 1.0;

    //     vertices[i].color.comps[0] = 1.0;
    //     vertices[i].color.comps[1] = 1.0;
    //     vertices[i].color.comps[2] = 1.0;
    //     vertices[i].color.comps[3] = 1.0;

    //     vec2 = *(vec2_t *)get_list_element(&data.tex_coords, i);
    //     vertices[i].tex_coords.comps[0] = vec2.comps[0];
    //     vertices[i].tex_coords.comps[1] = vec2.comps[1];
    //     vertices[i].tex_coords.comps[2] = 1.0;
    //     vertices[i].tex_coords.comps[3] = 1.0;
    // }

    // for(uint32_t i = 0; i < data.batches.cursor; i++)
    // {
    //     batch = (struct batch_data_t *)get_list_element(&data.batches, i);
    //     material_handle = r_AllocMaterial();
    //     material = r_GetMaterialPointer(material_handle);
    //     material->diffuse_texture = r_LoadTexture(batch->diffuse_texture);
    //     material->normal_texture = R_INVALID_TEXTURE_HANDLE;
    // }

    // quad = r_Alloc(sizeof(struct vertex_t) * data.vertices.cursor, sizeof(struct vertex_t), 0);
    // r_Memcpy(quad, vertices, sizeof(struct vertex_t) * data.vertices.cursor);
    // free(vertices);

    // load_bsp("q3ctf1.bsp");



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


    // mat4_t_persp(&projection_matrix, 0.68, 1366.0 / 768.0, 0.01, 1000.0);
    // mat4_t_identity(&view_matrix);

    // r_SetViewProjectionMatrix(NULL, &projection_matrix);
    // r_SetTexture(logo, 0);
    // r_SetTexture(doggo, 1);

    // float pitch = 0.0;
    // float yaw = 0.0;
    // float mouse_dx;
    // float mouse_dy;
    // int mouse_x;
    // int mouse_y;
    // uint8_t *keys;

    // vec4_t forward_vec;
    // vec4_t right_vec;
    // vec4_t translation;
    // vec4_t position;

    SDL_ShowCursor(0);
    
    // float r = 0.0;

    // position = vec4_t_zero;
    mat3_t orientation;
    vec3_t position(0.0, 0.0, 0.0);
    mat3_t_identity(&orientation);
    union entity_handle_t player = g_CreatePlayer("main player", &position, &orientation);
    g_SetPlayer(player);


    // position.x = 20.0;
    // gun = mdl_LoadModel("gun.obj");
    // union entity_handle_t handle = g_CreatePlatform("FUCK", &position, &orientation);
    // struct entity_t* platform = ent_GetEntityPointer(handle);
    // platform->model = gun;

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