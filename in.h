#ifndef IN_H
#define IN_H

#include <stdint.h>
#include "lib/SDL/include/SDL.h"

enum IN_INPUT_STATES
{
    IN_INPUT_STATE_PRESSED = 1,
    IN_INPUT_STATE_JUST_PRESSED = 1 << 1,
    IN_INPUT_STATE_JUST_RELEASED = 1 << 2,
};

enum IN_DRAG_STATES
{
    IN_DRAG_STATE_JUST_STARTED_DRAGGING = 1,
    IN_DRAG_STATE_JUST_STOPPED_DRAGGING = 1 << 1,
    IN_DRAG_STATE_DRAGGING = 1 << 2
};

enum IN_MOUSE_BUTTON
{
    IN_MOUSE_BUTTON_LEFT = 0,
    IN_MOUSE_BUTTON_MIDDLE,
    IN_MOUSE_BUTTON_RIGHT,
    IN_MOUSE_BUTTON_LAST
};

struct key_state_t
{
    uint16_t flags;
};

void in_ReadInput();

void in_RegisterKey(uint16_t key);

void in_UnregisterKey(uint16_t key);

uint32_t in_GetKeyState(uint16_t key);

uint32_t in_GetMouseState(uint32_t button);

uint32_t in_GetDragState();

void in_GetMouseDelta(float *dx, float *dy);

void in_GetMousePos(int32_t *x, int32_t *y);

void in_RelativeMode(uint32_t enable);

void in_TextInput(uint32_t enable);

char *in_GetInputText();

uint8_t *in_GetKeyboardState();

#endif
