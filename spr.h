#ifndef SPR_H
#define SPR_H

#include "r_common.h"

struct spr_sprite_entry_t
{
    float normalized_x;
    float normalized_y;
    float normalized_width;
    float normalized_height;
    float width;
    float height;
};

struct spr_sprite_sheet_t
{
//    uint32_t width;
//    uint32_t height;
//    struct r_texture_handle_t texture;
//    uint32_t sprite_count;
//    struct spr_sprite_t *sprites;
//    struct spr_sprite_entry_t *entries;
};

struct spr_sprite_sheet_h
{
    uint32_t index;
};

#define SPR_INVALID_SPRITE_SHEET_INDEX 0xffffffff
#define SPR_DEFAULT_SPRITE_SHEET_INDEX 0x0
#define SPR_SPRITE_SHEET_HANDLE(index) (struct spr_sprite_sheet_h){index}
#define SPR_INVALID_SPRITE_SHEET_HANDLE SPR_SPRITE_SHEET_HANDLE(SPR_INVALID_SPRITE_SHEET_INDEX)

struct spr_sprite_t
{
    struct spr_sprite_sheet_h sprite_sheet;
    uint32_t frame_count;
    uint32_t first_entry;
    char *name;
};

struct spr_sprite_h
{
    struct spr_sprite_sheet_h sprite_sheet;
    uint32_t index;
};

#define SPR_INVALID_SPRITE_INDEX 0xffffffff
#define SPR_SPRITE_HANDLE(index)(struct spr_sprite_h){{index}}
#define SPR_INVALID_SPRITE_HANDLE SPR_SPRITE_HANDLE(SPR_INVALID_SPRITE_INDEX)

struct spr_anim_frame_t
{
    vec2_t offset;
    float rotation;
    float scale;
    float duration;
    uint16_t sprite_frame;
};

struct spr_animation_t
{
    struct spr_sprite_h sprite;
    struct spr_anim_frame_t *frames;
    float duration;
    uint16_t frame_count;
};

struct spr_animation_h
{
    uint32_t index;
};

#define SPR_INVALID_ANIMATION_INDEX 0xffffffff
#define SPR_ANIMATION_HANDLE(index) (struct spr_animation_h){index}
#define SPR_INVALID_ANIMATION_HANDLE SPR_ANIMATION_HANDLE(SPR_INVALID_ANIMATION_INDEX)

struct spr_anim_player_t
{
    struct spr_animation_h animation;
    float frame_elapsed;
    uint32_t active_index;
    uint16_t current_frame;
};

struct spr_anim_player_h
{
    uint32_t index;
};

#define SPR_INVALID_ANIM_PLAYER_INDEX 0xffffffff
#define SPR_ANIM_PLAYER_HANDLE(index) (struct spr_anim_player_h){index}
#define SPR_INVALID_ANIM_PLAYER_HANDLE SPR_ANIM_PLAYER_HANDLE(SPR_INVALID_ANIM_PLAYER_INDEX)



void spr_Init();

void spr_Shutdown();

void spr_UpdateAnimPlayers(float delta_time);

struct spr_sprite_sheet_h spr_CreateSpriteSheet();

void spr_DestroySpriteSheet(struct spr_sprite_sheet_h handle);

struct spr_sprite_sheet_t *spr_GetSpriteSheetPointer(struct spr_sprite_sheet_h handle);

void spr_CreateDefaultSpriteSheet();

struct spr_sprite_sheet_h spr_GetDefaultSpriteSheetHandle();

struct spr_sprite_sheet_h spr_LoadSpriteSheeet(char *file_name);



struct spr_sprite_h spr_GetSpriteHandle(struct spr_sprite_sheet_h handle, char *name);

struct spr_sprite_t *spr_GetSpritePointer(struct spr_sprite_h handle);

struct spr_sprite_entry_t *spr_GetSpriteEntry(struct spr_sprite_h handle, uint32_t frame);



struct spr_animation_h spr_CreateAnimation(uint32_t frame_count);

struct spr_animation_h spr_CreateAnimationFromSprite(struct spr_sprite_h handle);

struct spr_animation_t *spr_GetAnimationPointer(struct spr_animation_h handle);

struct spr_anim_frame_t *spr_GetAnimationFrame(struct spr_animation_h handle, uint32_t frame_index);



struct spr_anim_player_h spr_StartAnimation(struct spr_anim_player_h player_handle, struct spr_animation_h animation_handle);

void spr_PauseAnimation(struct spr_anim_player_h player_handle);

void spr_StopAnimation(struct spr_anim_player_h player_handle);

struct spr_anim_player_t *spr_GetAnimationPlayerPointer(struct spr_anim_player_h player_handle);

void spr_SetAnimationFrame(struct spr_anim_player_h player_handle, uint32_t frame);

struct spr_anim_frame_t *spr_GetCurrentAnimationFrame(struct spr_anim_player_h player_handle);

uint32_t spr_IsLastAnimationFrame(struct spr_anim_player_h player_handle);


#endif // SPR_H
