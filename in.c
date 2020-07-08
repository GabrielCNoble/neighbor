#include "in.h"
#include "lib/SDL/include/SDL2/SDL_events.h"
#include "lib/SDL/include/SDL2/SDL_video.h"
#include <stdio.h>
//#include "r_nvkl.h"



SDL_Event event;
uint16_t keys[SDL_NUM_SCANCODES];
uint16_t registered_keys[SDL_NUM_SCANCODES];
uint32_t registered_keys_count = 0;
struct key_state_t key_states[SDL_NUM_SCANCODES];
uint32_t mouse_state[IN_MOUSE_BUTTON_LAST];
float normalized_x = 0.0;
float normalized_y = 0.0;
float normalized_dx = 0.0;
float normalized_dy = 0.0;
int32_t in_mouse_x;
int32_t in_mouse_y;

uint32_t in_relative_mode;

//extern struct r_renderer_t r_renderer;
extern SDL_Window *r_window;

void in_ReadInput()
{
    const uint8_t *keyboard_state;
    struct key_state_t *key_state;
    uint32_t scancode;
    int32_t window_width;
    int32_t window_height;
    uint32_t mouse_buttons;
    int32_t mouse_x;
    int32_t mouse_y;
    
    SDL_GetWindowSize(r_window, &window_width, &window_height);
    SDL_GetMouseState(&in_mouse_x, &in_mouse_y);
    
    SDL_PollEvent(&event);
    keyboard_state = SDL_GetKeyboardState(NULL);
    mouse_buttons = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

    normalized_dx = ((float)mouse_x / (float)window_width);
    normalized_dy = -((float)mouse_y / (float)window_height);
    
    

    for(uint32_t mouse_button = IN_MOUSE_BUTTON_LEFT; mouse_button < IN_MOUSE_BUTTON_LAST; mouse_button++)
    {
        mouse_state[mouse_button] &= ~(IN_INPUT_STATE_JUST_PRESSED | IN_INPUT_STATE_JUST_RELEASED);
        if(mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT + mouse_button))
        {
            if(!(mouse_state[mouse_button] & IN_INPUT_STATE_PRESSED))
            {
                mouse_state[mouse_button] |= IN_INPUT_STATE_JUST_PRESSED;
            }
            
            mouse_state[mouse_button] |= IN_INPUT_STATE_PRESSED;
        }
        else
        {
            if(mouse_state[mouse_button] & IN_INPUT_STATE_PRESSED)
            {
                mouse_state[mouse_button] |= IN_INPUT_STATE_JUST_RELEASED;
            }
            
            mouse_state[mouse_button] &= ~IN_INPUT_STATE_PRESSED;
        }
    }

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

uint32_t in_GetMouseState(uint32_t button)
{
    if(button < IN_MOUSE_BUTTON_LAST)
    {
        return mouse_state[button];
    }
    
    return 0;
}

void in_GetMouseDelta(float *dx, float *dy)
{
    *dx = normalized_dx;
    *dy = normalized_dy;
}

void in_GetMousePos(int32_t *x, int32_t *y)
{
    *x = in_mouse_x;
    *y = in_mouse_y;
}

void in_RelativeMode(uint32_t enable)
{
    SDL_SetRelativeMouseMode(enable);
}
