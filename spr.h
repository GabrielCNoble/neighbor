#ifndef SPR_H
#define SPR_H

#include "r_common.h"

struct spr_sprite_entry_t
{
    float x;
    float y;
    float w;
    float h;
};

struct spr_sprite_sheet_t
{
    uint32_t width;
    uint32_t height;
    struct r_texture_handle_t texture;
    uint32_t sprite_count;
    struct spr_sprite_t *sprites;
    struct spr_sprite_entry_t *entries;
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
#define SPR_SPRITE_HANDLE(index)(struct spr_sprite_h){index}
#define SPR_INVALID_SPRITE_HANDLE SPR_SPRITE_HANDLE(SPR_INVALID_SPRITE_INDEX)


void spr_Init();

void spr_Shutdown();

struct spr_sprite_sheet_h spr_CreateSpriteSheet();

void spr_DestroySpriteSheet(struct spr_sprite_sheet_h handle);

struct spr_sprite_sheet_t *spr_GetSpriteSheetPointer(struct spr_sprite_sheet_h handle);

void spr_CreateDefaultSpriteSheet();

struct spr_sprite_sheet_h spr_GetDefaultSpriteSheetHandle();

struct spr_sprite_sheet_h spr_LoadSpriteSheeet(char *file_name);




struct spr_sprite_h spr_GetSpriteHandle(struct spr_sprite_sheet_h handle, char *name);

struct spr_sprite_t *spr_GetSpritePointer(struct spr_sprite_h handle);





#endif // SPR_H
