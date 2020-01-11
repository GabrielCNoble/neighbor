#ifndef IN_H
#define IN_H

#include <stdint.h>
#include "SDL/include/SDL2/SDL.h"

enum IN_INPUT_STATES
{
    IN_INPUT_STATE_PRESSED = 1,
    IN_INPUT_STATE_JUST_PRESSED = 1 << 1,
    IN_INPUT_STATE_JUST_RELEASED = 1 << 2,
};

struct key_state_t
{
    uint16_t flags;
};

void in_ReadInput();

void in_RegisterKey(uint16_t key);

void in_UnregisterKey(uint16_t key);

uint32_t in_GetKeyState(uint16_t key);

void in_GetMouseDelta(float *dx, float *dy);

#endif