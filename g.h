#ifndef G_H
#define G_H

//#include "SDL/include/SDL2/SDL.h"
//#undef main

void g_SetInitCallback(void (*callback)());

void g_SetMainLoopCallback(void (*callback)(float delta_time));

void g_SetShutdownCallback(void (*callback)());

void g_MainLoop();

void g_Quit();


#endif // G_H
