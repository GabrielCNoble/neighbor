#ifndef G_H
#define G_H

//#include "SDL/include/SDL2/SDL.h"
//#undef main

void g_SetInitCallback(void (*callback)());

void g_SetMainLoopCallback(void (*callback)(float delta_time));

void g_SetShutdownCallback(void (*callback)());

void g_MainLoop();

void g_Quit();

void g_UpdateDeltaTime();

float g_GetCurrentDeltaTime(uint64_t *current_count_value);


#endif // G_H
