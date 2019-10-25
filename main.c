#define SDL_MAIN_HANDLED
#include "SDL/include/SDL2/SDL.h"
#include "SDL/include/SDL2/SDL_syswm.h"
#include "Windows.h"

#include "r_vk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL error. %s\n", SDL_GetError());
        return -1;
    }

    r_InitBackend();
    
    while(1)
    {
        SDL_PollEvent(NULL);
        r_UploadData();
        r_BeginRenderpass();
        r_Draw();
        r_EndRenderpass();
        r_Present();
        SDL_Delay(16);
    }

    return 0;
}