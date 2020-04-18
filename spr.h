#ifndef SPR_H
#define SPR_H

#include "r_common.h"

struct spr_sprite_t
{
    uint32_t frame_count;
    uint32_t entry;
};

struct spr_sprite_entry_t
{
    float x;
    float y;
    float w;
    float h;
};

struct spr_sprite_sheet_t
{

    struct r_chunk_handle_t sprite_sheet;
};


#endif // SPR_H
