#include "r.h"
#include "spr.h"
#include "lib/dstuff/containers/stack_list.h"
#include "lib/dstuff/containers/list.h"
#include "lib/dstuff/sprite/sprpk.h"
#include "lib/stb/stb_image.h"


struct stack_list_t spr_sprite_sheets;
struct stack_list_t spr_animations;
struct list_t spr_active_players;
struct stack_list_t spr_players;

void spr_Init()
{
    spr_sprite_sheets = create_stack_list(sizeof(struct spr_sprite_sheet_t), 16);
    spr_animations = create_stack_list(sizeof(struct spr_animation_t), 128);
    spr_players = create_stack_list(sizeof(struct spr_anim_player_t), 512);
    spr_active_players = create_list(sizeof(struct spr_anim_player_h), 512);
    spr_CreateDefaultSpriteSheet();
}

void spr_Shutdown()
{

}

void spr_UpdateAnimPlayers(float delta_time)
{
    struct spr_anim_player_t *player;
    struct spr_anim_player_h *handle;
    struct spr_animation_t *animation;
    struct spr_anim_frame_t *frame;
//    float frame_elapsed;

    for(uint32_t player_index = 0; player_index < spr_active_players.cursor; player_index++)
    {
        handle = get_list_element(&spr_active_players, player_index);
        player = get_stack_list_element(&spr_players,handle->index);
        animation = spr_GetAnimationPointer(player->animation);
        frame = animation->frames + player->current_frame;
        player->frame_elapsed += delta_time;

        while(player->frame_elapsed >= frame->duration)
        {
            player->frame_elapsed -= frame->duration;
            player->current_frame = (player->current_frame + 1) % animation->frame_count;
            frame = animation->frames + player->current_frame;
        }
    }
}

struct spr_sprite_sheet_h spr_CreateSpriteSheet()
{
    struct spr_sprite_sheet_t *sprite_sheet;
    struct spr_sprite_sheet_h handle;

    handle.index = add_stack_list_element(&spr_sprite_sheets, NULL);
    sprite_sheet = get_stack_list_element(&spr_sprite_sheets, handle.index);
    sprite_sheet->texture = R_INVALID_TEXTURE_HANDLE;
    sprite_sheet->sprites = NULL;
    sprite_sheet->entries = NULL;

    return handle;
}

void spr_DestroySpriteSheet(struct spr_sprite_sheet_h handle)
{
    if(handle.index != SPR_DEFAULT_SPRITE_SHEET_INDEX)
    {

    }
}

struct spr_sprite_sheet_t *spr_GetSpriteSheetPointer(struct spr_sprite_sheet_h handle)
{
    struct spr_sprite_sheet_t *sprite_sheet;
    sprite_sheet = get_stack_list_element(&spr_sprite_sheets, handle.index);
    if(sprite_sheet && sprite_sheet->texture.index == R_INVALID_TEXTURE_INDEX)
    {
        sprite_sheet = NULL;
    }

    return sprite_sheet;
}

void spr_CreateDefaultSpriteSheet()
{
    struct spr_sprite_sheet_h handle;
    struct spr_sprite_sheet_t *sprite_sheet;
    struct spr_sprite_t *sprite;
    struct spr_sprite_entry_t *entry;

    handle = spr_CreateSpriteSheet();
    sprite_sheet = get_stack_list_element(&spr_sprite_sheets, handle.index);
    sprite_sheet->texture = r_GetDefaultTextureHandle();
    sprite_sheet->sprite_count = 1;
    sprite_sheet->sprites = calloc(sizeof(struct spr_sprite_t), sprite_sheet->sprite_count);
    sprite_sheet->entries = calloc(sizeof(struct spr_sprite_entry_t), sprite_sheet->sprite_count);

    entry = sprite_sheet->entries;
    entry->normalized_x = 0.0;
    entry->normalized_y = 0.0;
    entry->normalized_width = 1.0;
    entry->normalized_height = 1.0;

    sprite = sprite_sheet->sprites;
    sprite->frame_count = 1;
    sprite->sprite_sheet = handle;
    sprite->name = "default_sprite";
    sprite->first_entry = 0;
}

struct spr_sprite_sheet_h spr_GetDefaultSpriteSheetHandle()
{
    return SPR_SPRITE_SHEET_HANDLE(SPR_DEFAULT_SPRITE_SHEET_INDEX);
}

struct spr_sprite_sheet_h spr_LoadSpriteSheeet(char *file_name)
{
    struct header_t *header;
    struct entry_t *entry;
    void *data;
    uint32_t data_size;
    unsigned char *pixels;
    struct spr_sprite_sheet_h handle;
    struct spr_sprite_sheet_t *sprite_sheet;
    struct spr_sprite_t *sprite;
    struct spr_sprite_entry_t *sprite_entry;
    struct spr_sprite_entry_t *sprite_entries;
    struct r_texture_t *texture;
    uint32_t entry_offset = 0;
    int channels;
    int width;
    int height;
    struct r_texture_description_t texture_description = {};
    read_sprpk(file_name, &header, &data_size);

    if(header)
    {
        handle = spr_CreateSpriteSheet();
        sprite_sheet = get_stack_list_element(&spr_sprite_sheets, handle.index);

        sprite_sheet->sprite_count = header->entry_count;
        sprite_sheet->sprites = calloc(sizeof(struct spr_sprite_t), header->entry_count);
        sprite_sheet->entries = calloc(sizeof(struct spr_sprite_entry_t), header->frame_count);
        sprite_sheet->width = header->width;
        sprite_sheet->height = header->height;

        for(uint32_t sprite_index = 0; sprite_index < header->entry_count; sprite_index++)
        {
            get_sprpk_entry(header, &entry, sprite_index);

            sprite = sprite_sheet->sprites + sprite_index;
            sprite->frame_count = entry->frame_count;
            sprite->first_entry = entry_offset;
            sprite->name = strdup(entry->name);

            sprite_entries = sprite_sheet->entries + entry_offset;

            for(uint32_t entry_index = 0; entry_index < entry->frame_count; entry_index++)
            {
                sprite_entry = sprite_entries + entry_index;
                sprite_entry->normalized_width = (float)entry->frames[entry_index].width / (float)sprite_sheet->width;
                sprite_entry->normalized_height = (float)entry->frames[entry_index].height / (float)sprite_sheet->height;
                sprite_entry->normalized_x = (float)entry->frames[entry_index].offset_x / (float)sprite_sheet->width;
                sprite_entry->normalized_y = (float)entry->frames[entry_index].offset_y / (float)sprite_sheet->height;
                sprite_entry->width = (float)entry->frames[entry_index].width;
                sprite_entry->height = (float)entry->frames[entry_index].height;
            }

            entry_offset += entry->frame_count;
        }

        data_size -= header->data_offset;
        get_sprpk_data(header, &data);
        pixels = stbi_load_from_memory((const unsigned char *)header + header->data_offset, header->data_size, &width, &height, &channels, STBI_rgb_alpha);
        sprite_sheet->width = width;
        sprite_sheet->height = height;

        texture_description.image_type = VK_IMAGE_TYPE_2D;
        texture_description.format = VK_FORMAT_R8G8B8A8_UNORM;
        texture_description.extent.width = sprite_sheet->width;
        texture_description.extent.height = sprite_sheet->height;
        texture_description.extent.depth = 1;
        texture_description.sampler_params.addr_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texture_description.sampler_params.addr_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texture_description.sampler_params.addr_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texture_description.sampler_params.min_filter = VK_FILTER_NEAREST;
        texture_description.sampler_params.mag_filter = VK_FILTER_NEAREST;
        texture_description.sampler_params.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        sprite_sheet->texture = r_CreateTexture(&texture_description);
        texture = r_GetTexturePointer(sprite_sheet->texture);
        r_FillImageChunk(texture->image, pixels, NULL);
        r_SetImageLayout(texture->image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return handle;
}

struct spr_sprite_h spr_GetSpriteHandle(struct spr_sprite_sheet_h handle, char *name)
{
    struct spr_sprite_sheet_t *sprite_sheet;
    struct spr_sprite_t *sprites;
    struct spr_sprite_h sprite = SPR_INVALID_SPRITE_HANDLE;

    sprite_sheet = spr_GetSpriteSheetPointer(handle);
    if(sprite_sheet)
    {
        sprites = sprite_sheet->sprites;

        for(uint32_t sprite_index = 0; sprite_index < sprite_sheet->sprite_count; sprite_index++)
        {
            if(!strcmp(name, sprites[sprite_index].name))
            {
                sprite.index = sprite_index;
                sprite.sprite_sheet = handle;
                break;
            }
        }
    }

    return sprite;
}

struct spr_sprite_t *spr_GetSpritePointer(struct spr_sprite_h handle)
{
    struct spr_sprite_sheet_t *sprite_sheet;
    struct spr_sprite_t *sprite = NULL;

    sprite_sheet = spr_GetSpriteSheetPointer(handle.sprite_sheet);
    if(sprite_sheet)
    {
        sprite = sprite_sheet->sprites + handle.index;
    }
    return sprite;
}

struct spr_sprite_entry_t *spr_GetSpriteEntry(struct spr_sprite_h handle, uint32_t frame)
{
    struct spr_sprite_sheet_t *sprite_sheet;
    struct spr_sprite_entry_t *entry = NULL;
    struct spr_sprite_t *sprite;

    sprite_sheet = spr_GetSpriteSheetPointer(handle.sprite_sheet);
    sprite = spr_GetSpritePointer(handle);

    if(sprite_sheet && sprite)
    {
        if(frame < sprite->frame_count)
        {
            entry = sprite_sheet->entries + sprite->first_entry + frame;
        }
    }

    return entry;
}

struct spr_animation_h spr_CreateAnimation(uint32_t frame_count)
{
    struct spr_animation_h handle = SPR_INVALID_ANIMATION_HANDLE;
    struct spr_animation_t *animation;

    handle.index = add_stack_list_element(&spr_animations, NULL);
    animation = get_stack_list_element(&spr_animations, handle.index);

    memset(animation, 0, sizeof(struct spr_animation_t));
    animation->frame_count = frame_count;
    animation->frames = calloc(sizeof(struct spr_anim_frame_t), frame_count);

    return handle;
}

struct spr_animation_h spr_CreateAnimationFromSprite(struct spr_sprite_h handle)
{
    struct spr_animation_h animation_handle = SPR_INVALID_ANIMATION_HANDLE;
    struct spr_animation_t *animation;
//    struct spr_sprite_sheet_t *sprite_sheet;
    struct spr_sprite_t *sprite;
    struct spr_anim_frame_t *frame;

    sprite = spr_GetSpritePointer(handle);

    if(sprite)
    {
        animation_handle = spr_CreateAnimation(sprite->frame_count);
        animation = spr_GetAnimationPointer(animation_handle);
        animation->sprite = handle;

        for(uint32_t frame_index = 0; frame_index < animation->frame_count; frame_index++)
        {
            frame = animation->frames + frame_index;
            frame->sprite_frame = frame_index;
            frame->scale = 1.0;
        }
    }

    return animation_handle;
}

struct spr_animation_t *spr_GetAnimationPointer(struct spr_animation_h handle)
{
    struct spr_animation_t *animation;

    animation = get_stack_list_element(&spr_animations, handle.index);
    if(animation && animation->frame_count == 0xffffffff)
    {
        animation = NULL;
    }

    return animation;
}

struct spr_anim_frame_t *spr_GetAnimationFrame(struct spr_animation_h handle, uint32_t frame_index)
{
    struct spr_animation_t *animation;
    struct spr_anim_frame_t *frame = NULL;
    animation = spr_GetAnimationPointer(handle);

    if(animation)
    {
        frame = animation->frames + (frame_index % animation->frame_count);
    }

    return frame;
}

struct spr_anim_player_h spr_StartAnimation(struct spr_anim_player_h player_handle, struct spr_animation_h animation_handle)
{
    struct spr_anim_player_t *player;
    struct spr_animation_t *animation;

    animation = spr_GetAnimationPointer(animation_handle);
    if(animation)
    {
        player = spr_GetAnimationPlayerPointer(player_handle);
        if(!player)
        {
            player_handle.index = add_stack_list_element(&spr_players, NULL);
            player = get_stack_list_element(&spr_players, player_handle.index);
            player->active_index = 0xffffffff;
        }

        player->animation = animation_handle;
        player->current_frame = 0;
        player->frame_elapsed = 0.0;

        if(player->active_index == 0xffffffff)
        {
            player->active_index = add_list_element(&spr_active_players, &player_handle);
        }
    }

    return player_handle;
}

void spr_PauseAnimation(struct spr_anim_player_h player_handle)
{
    struct spr_anim_player_t *player;

    player = spr_GetAnimationPlayerPointer(player_handle);

    if(player && player->active_index < 0xffffffff)
    {
        remove_list_element(&spr_active_players, player->active_index);
        player->active_index = 0xffffffff;
    }
}

void spr_ResumeAnimation(struct spr_anim_player_h player_handle)
{
    struct spr_anim_player_t *player;

    player = spr_GetAnimationPlayerPointer(player_handle);

    if(player && player->active_index == 0xffffffff)
    {
        player->active_index = add_list_element(&spr_active_players, &player_handle);
    }
}

void spr_StopAnimation(struct spr_anim_player_h player_handle)
{
    struct spr_anim_player_t *player;

    player = spr_GetAnimationPlayerPointer(player_handle);

    if(player)
    {
        if(player->active_index < 0xffffffff)
        {
            remove_list_element(&spr_active_players, player->active_index);
        }
        player->animation = SPR_INVALID_ANIMATION_HANDLE;

        remove_stack_list_element(&spr_players, player_handle.index);
    }
}

struct spr_anim_player_t *spr_GetAnimationPlayerPointer(struct spr_anim_player_h player_handle)
{
    struct spr_anim_player_t *player;
    player = get_stack_list_element(&spr_players, player_handle.index);
    if(player && player->animation.index == SPR_INVALID_ANIMATION_INDEX && player->active_index == 0xffffffff)
    {
        player = NULL;
    }

    return player;
}

void spr_SetAnimationFrame(struct spr_anim_player_h player_handle, uint32_t frame)
{
    struct spr_anim_player_t *player;
    struct spr_animation_t *animation;
    player = spr_GetAnimationPlayerPointer(player_handle);

    if(player)
    {
        animation = spr_GetAnimationPointer(player->animation);
        player->frame_elapsed = 0.0;
        player->current_frame = frame % animation->frame_count;
    }
}

struct spr_anim_frame_t *spr_GetCurrentAnimationFrame(struct spr_anim_player_h player_handle)
{
    struct spr_anim_player_t *player;
    struct spr_animation_t *animation;
    struct spr_anim_frame_t *frame = NULL;

    player = spr_GetAnimationPlayerPointer(player_handle);
    if(player)
    {
        animation = spr_GetAnimationPointer(player->animation);
        frame = animation->frames + player->current_frame;
    }

    return frame;
}

uint32_t spr_IsLastAnimationFrame(struct spr_anim_player_h player_handle)
{
    struct spr_anim_player_t *player;
    struct spr_animation_t *animation;
    player = spr_GetAnimationPlayerPointer(player_handle);
    animation = spr_GetAnimationPointer(player->animation);
    return player->current_frame == animation->frame_count - 1;
}

























