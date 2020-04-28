#include "in.h"
#include "r_common.h"
#include <stdio.h>
#include "r.h"



SDL_Event event;
uint16_t keys[SDL_NUM_SCANCODES];
uint16_t registered_keys[SDL_NUM_SCANCODES];
uint32_t registered_keys_count = 0;
struct key_state_t key_states[SDL_NUM_SCANCODES];
float normalized_x = 0.0;
float normalized_y = 0.0;
float normalized_dx = 0.0;
float normalized_dy = 0.0;

extern struct r_renderer_t r_renderer;

void in_ReadInput()
{
    const uint8_t *keyboard_state;
    struct key_state_t *key_state;
    uint32_t scancode;
    int mouse_x;
    int mouse_y;
    uint32_t window_width;
    uint32_t window_height;

    SDL_PollEvent(&event);
    keyboard_state = SDL_GetKeyboardState(NULL);
    SDL_GetMouseState(&mouse_x, &mouse_y);
    window_width = 800;
    window_height = 600;
//    r_GetWindowSize(&window_width, &window_height);

    normalized_dx = ((float)mouse_x / (float)window_width) * 2.0 - 1.0;
    normalized_dy = -((float)mouse_y / (float)window_height) * 2.0 + 1.0;
//    SDL_WarpMouseInWindow(r_renderer.window, window_width >> 1, window_height >> 1);



    for(uint32_t i = 0; i < registered_keys_count; i++)
    {
        scancode = registered_keys[i];
        key_state = key_states + scancode;

        key_state->flags &= ~(IN_INPUT_STATE_JUST_PRESSED | IN_INPUT_STATE_JUST_RELEASED);

        if(keyboard_state[scancode])
        {
            if(!(key_state->flags & IN_INPUT_STATE_PRESSED))
            {
                key_state->flags |= IN_INPUT_STATE_JUST_PRESSED;
            }

            key_state->flags |= IN_INPUT_STATE_PRESSED;
        }
        else
        {
            if(key_state->flags & IN_INPUT_STATE_PRESSED)
            {
                key_state->flags |= IN_INPUT_STATE_JUST_RELEASED;
            }

            key_state->flags &= ~IN_INPUT_STATE_PRESSED;
        }
    }
}

void in_RegisterKey(uint16_t key)
{
    if(key < SDL_NUM_SCANCODES)
    {
        for(uint32_t i = 0; i < registered_keys_count; i++)
        {
            if(registered_keys[i] == key)
            {
                return;
            }
        }

        registered_keys[registered_keys_count] = key;
        registered_keys_count++;
    }
}

void in_UnregisterKey(uint16_t key)
{
    if(key < SDL_NUM_SCANCODES)
    {
        for(uint32_t i = 0; i < registered_keys_count; i++)
        {
            if(registered_keys[i] == key)
            {
                if(i < registered_keys_count - 1)
                {
                    registered_keys[i] = registered_keys[registered_keys_count - 1];
                }

                registered_keys_count--;
                return;
            }
        }
    }
}

uint32_t in_GetKeyState(uint16_t key)
{
    if(key < SDL_NUM_SCANCODES)
    {
        return key_states[key].flags;
    }

    return 0;
}

void in_GetMouseDelta(float *dx, float *dy)
{
    *dx = normalized_dx;
    *dy = normalized_dy;
}
