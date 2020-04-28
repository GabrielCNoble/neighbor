//#define SDL_MAIN_HANDLED
#include "SDL/include/SDL2/SDL.h"

#include "g.h"
#include "in.h"
#include "r.h"
#include "r_draw.h"
#include <stdio.h>
#include <stdint.h>

void (*g_InitCallback)() = NULL;
void (*g_MainLoopCallback)(float delta_time) = NULL;
void (*g_ShutdownCallback)() = NULL;
uint32_t g_run_loop = 1;

void g_SetInitCallback(void (*callback)())
{
    g_InitCallback = callback;
}

void g_SetMainLoopCallback(void (*callback)(float delta_time))
{
    g_MainLoopCallback = callback;
}

void g_SetShutdownCallback(void (*callback)())
{
    g_ShutdownCallback = callback;
}

void g_MainLoop()
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL error. %s\n", SDL_GetError());
        return;
    }

    r_Init();
    r_DrawInit();
    spr_Init();

    if(g_InitCallback)
    {
        g_InitCallback();
    }

    while(g_run_loop)
    {
        in_ReadInput();
        r_BeginFrame();
        g_MainLoopCallback(0.0);
        r_EndFrame();
        SDL_Delay(16);
    }

    if(g_ShutdownCallback)
    {
        g_ShutdownCallback();
    }

    r_Shutdown();
}

void g_Quit()
{
    g_run_loop = 0;
}



