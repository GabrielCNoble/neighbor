#define SDL_MAIN_HANDLED
#include "SDL/include/SDL2/SDL.h"
#include "SDL/include/SDL2/SDL_syswm.h"
// #include "Windows.h"

#include "r_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <math.h>


extern struct r_renderer_t r_renderer;

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL error. %s\n", SDL_GetError());
        return -1;
    }

    r_InitRenderer();

    struct r_texture_handle_t logo = r_LoadTexture("logo_fuck.png");
    struct r_texture_handle_t logo2 = r_LoadTexture("logo2.png");
    struct r_texture_handle_t doggo = r_LoadTexture("doggo.png");
    mat4_t projection_matrix;
    mat4_t view_matrix;
    mat4_t pitch_matrix;
    mat4_t yaw_matrix;

    mat4_t_persp(&projection_matrix, 0.68, 800.0 / 600.0, 0.01, 100.0);
    mat4_t_identity(&view_matrix);

    r_SetProjectionMatrix(&projection_matrix);
    r_SetViewMatrix(&view_matrix);
    r_SetModelMatrix(&view_matrix);
    r_SetTexture(logo, 0);
    r_SetTexture(logo2, 1);

    // float f = 0.0;
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
    

    position = vec4_t_zero;
    while(1)
    {
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


        // view_matrix.vcomps[3].comps[2] = 6.0;
        // view_matrix.vcomps[3].comps[1] = 5.0;

        r_SetViewMatrix(&view_matrix);
        r_UpdateMatrices();
        r_UploadData();
        r_BeginRenderpass();
        r_Draw();
        r_EndRenderpass();
        r_Present();
        SDL_Delay(16);
        
        // f += 0.05; 
    }

    return 0;
}