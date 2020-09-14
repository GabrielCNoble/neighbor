#include "lib/SDL/include/SDL.h"
#include "g.h"
#include "in.h"
#include "r_nvkl.h"
#include "r_draw.h"
#include "phy.h"
#include "ui.h"
#include "scr.h"
#include "ent.h"
#include "lib/dstuff/ds_mem.h"
#include <stdio.h>
#include <stdint.h>

void (*g_InitCallback)() = NULL;
void (*g_MainLoopCallback)(float delta_time) = NULL;
void (*g_ShutdownCallback)() = NULL;
uint32_t g_run_loop = 1;

uint64_t g_timer_frequency;
uint64_t g_timer_count;
float g_delta_time;

//SDL_Window *g_window;

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

    g_timer_frequency = SDL_GetPerformanceFrequency();
    
    float delta_time;
    r_DrawInit();
    scr_Init();
    phy_Init();
    spr_Init();
    ui_Init();
    mdl_Init();
    ent_Init();

    if(g_InitCallback)
    {
        g_InitCallback();
    }

    g_UpdateDeltaTime();

    while(g_run_loop)
    {
        g_UpdateDeltaTime();
        in_ReadInput();
        r_BeginFrame();
        ui_BeginFrame();
        phy_Step(g_delta_time);
        g_MainLoopCallback(g_delta_time);
        scr_UpdateScripts(g_delta_time);
        ui_EndFrame();
        r_EndFrame();
        mem_CheckGuards();
        
        while(g_GetCurrentDeltaTime(NULL) < 16.666);
    }

    if(g_ShutdownCallback)
    {
        g_ShutdownCallback();
    }

    r_DrawShutdown();
}

void g_Quit()
{
    g_run_loop = 0;
}

void g_UpdateDeltaTime()
{
    uint64_t current_count;
    g_delta_time = g_GetCurrentDeltaTime(&current_count);
    g_timer_count = current_count;
}

float g_GetCurrentDeltaTime(uint64_t *current_count_value)
{
    uint64_t current_count = SDL_GetPerformanceCounter();
    uint64_t prev_count = g_timer_count;
    float delta_time;

    if(current_count < prev_count)
    {
        /* impressively enough, the counter wrapped around, so we'll adjust
        things up a bit here to allow consistent timing */
        current_count += 0xffffffffffffffff - prev_count;
        prev_count = 0;
    }

    delta_time = (double)((current_count - prev_count) * 1000) / (double)g_timer_frequency;
    if(current_count_value)
    {
        *current_count_value = current_count;
    }
    return delta_time;
}



