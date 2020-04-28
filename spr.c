#include "r.h"
#include "spr.h"
#include "dstuff/containers/stack_list.h"
#include "dstuff/sprite/sprpk.h"
#include "stb/stb_image.h"


struct stack_list_t spr_sprite_sheets;


void spr_Init()
{
    spr_sprite_sheets = create_stack_list(sizeof(struct spr_sprite_sheet_t), 16);
    spr_CreateDefaultSpriteSheet();
}

void spr_Shutdown()
{

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
    entry->x = 0.0;
    entry->y = 0.0;
    entry->w = 1.0;
    entry->h = 1.0;

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
                sprite_entry = sprite_entries + sprite_index;
                sprite_entry->w = entry->frames[entry_index].width;
                sprite_entry->h = entry->frames[entry_index].height;
                sprite_entry->x = entry->frames[entry_index].offset_x;
                sprite_entry->y = entry->frames[entry_index].offset_y;
            }

            entry_offset += entry->frame_count;
        }

        data_size -= header->data_offset;
        get_sprpk_data(header, &data);
        pixels = stbi_load_from_memory((char *)header + header->data_offset, header->data_size, &sprite_sheet->width, &sprite_sheet->height, &channels, STBI_rgb_alpha);

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
    }
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

        for(uint32_t sprite_index = 0; sprite_index < sprite_sheet->sprite_count; sprite_sheet++)
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


