#define SDL_MAIN_HANDLED
#include "SDL/include/SDL2/SDL.h"
#include "SDL/include/SDL2/SDL_syswm.h"
// #include "Windows.h"

#include "r_main.h"
#include "mdl.h"

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
    // struct r_draw_cmd_t *draw_cmd;
    // struct r_alloc_t *alloc;

    // r_BeginBatch(&r_renderer.view_projection_matrix, things[0].material);

    // for(uint32_t i = 0; i < thing_count; i++)
    // {
    //     r_AddDrawCmd(&things[i].transform, things[i].handle);
    // }

    // r_EndBatch();
}

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL error. %s\n", SDL_GetError());
        return -1;
    }

    r_InitRenderer();
    mdl_Init();

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
    mat4_t projection_matrix;
    mat4_t view_matrix;
    mat4_t pitch_matrix;
    mat4_t yaw_matrix;
    struct thing_t things[1];
    vec3_t vec3;
    vec2_t vec2;

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

    model = mdl_LoadModel("cube.obj");

    // things[0].handle = quad;
    things[0].model = model;
    // things[0].material = R_MATERIAL_HANDLE(0);
    mat4_t_identity(&things[0].transform);
    things[0].transform.vcomps[3].comps[2] = -5.0;

    // things[1].handle = quad;
    // mat4_t_identity(&things[1].transform);
    // things[1].transform.vcomps[3].comps[2] = -15.0;

    // things[2].handle = quad;
    // mat4_t_identity(&things[2].transform);
    // mat4_t_yaw(&things[2].transform, 0.2);
    // things[2].transform.vcomps[3].comps[2] = -7.0;
    // things[2].transform.vcomps[3].comps[0] = -6.0;

    mat4_t_persp(&projection_matrix, 0.68, 800.0 / 600.0, 0.01, 1000.0);
    mat4_t_identity(&view_matrix);

    r_SetViewProjectionMatrix(NULL, &projection_matrix);
    r_SetTexture(logo, 0);
    r_SetTexture(doggo, 1);

    float pitch = 0.0;
    float yaw = 0.0;
    float mouse_dx;
    float mouse_dy;
    int mouse_x;
    int mouse_y;
    uint8_t *keys;

    vec4_t forward_vec;
    vec4_t right_vec;
    vec4_t translation;
    vec4_t position;

    SDL_ShowCursor(0);
    
    float r = 0.0;

    position = vec4_t_zero;
    while(1)
    {

        mat4_t_yaw(&things[1].transform, r);
        things[1].transform.vcomps[3].comps[2] = -15.0;

        r += 0.06;
        things[2].transform.vcomps[3].comps[1] = sin(r * 3.14159265) * 1.5;

        SDL_PollEvent(NULL);
        SDL_GetMouseState(&mouse_x, &mouse_y);
        mouse_dx = ((float)mouse_x - 400.0) / 400.0;
        mouse_dy = (300.0 - (float)mouse_y) / 300.0;
        SDL_WarpMouseInWindow(r_renderer.window, 400, 300);
        pitch += mouse_dy * 0.1;
        yaw -= mouse_dx * 0.1;
        // printf("%f %f\n", mouse_dx, mouse_dy);

        mat4_t_pitch(&pitch_matrix, pitch);
        mat4_t_yaw(&yaw_matrix, yaw);
        mat4_t_mul(&view_matrix, &pitch_matrix, &yaw_matrix);

        forward_vec = view_matrix.vcomps[2];
        right_vec = view_matrix.vcomps[0];

        translation = vec4_t_zero;

        keys = SDL_GetKeyboardState(NULL);

        if(keys[SDL_SCANCODE_W])
        {
            translation.comps[2] = -0.2;
        }
        if(keys[SDL_SCANCODE_S])
        {
            translation.comps[2] = 0.2;
        }

        if(keys[SDL_SCANCODE_A])
        {
            translation.comps[0] = -0.2;
        }
        if(keys[SDL_SCANCODE_D])
        {
            translation.comps[0] = 0.2;
        }

        vec4_t_mul(&forward_vec, translation.comps[2]);
        vec4_t_mul(&right_vec, translation.comps[0]);
        vec4_t_add(&position, &position, &forward_vec);
        vec4_t_add(&position, &position, &right_vec);

        position.comps[3] = 1.0;
        view_matrix.vcomps[3] = position;

        r_SetViewProjectionMatrix(&view_matrix, NULL);
        r_QueueCmd(R_CMD_TYPE_BEGIN_FRAME, NULL, 0);
        draw_things(things, sizeof(things) / sizeof(things[0]));
        r_QueueCmd(R_CMD_TYPE_END_FRAME, NULL, 0);
        r_ExecuteCmds();
        // SDL_Delay(16);
    }

    return 0;
}