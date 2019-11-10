#ifndef XCHG_H
#define XCHG_H

#include "list.h"
#include "vector.h"

struct batch_data_t
{
    unsigned int start;
    unsigned int count;

    vec4_t base_color;
    char material[64];
    char diffuse_texture[64];
    char normal_texture[64];
};

#define DEFAULT_MATERIAL_NAME "default_material"
#define DEFAULT_BATCH (struct batch_data_t){0, 0, {1.0, 1.0, 1.0, 1.0}, DEFAULT_MATERIAL_NAME, "", ""}

//struct material_data_t
//{
//    vec4_t base;
//    char name[64];
//    char diffuse_texture[64];
//    char normal_texture[64];
//};
//
//#define DEFAULT_MATERIAL_NAME "default_material"
//#define DEFAULT_MATERIAL (struct material_data_t){vec4_t(1.0, 1.0, 1.0, 1.0), DEFAULT_MATERIAL_NAME, "", ""}

struct geometry_data_t
{
    struct list_t vertices;
    struct list_t normals;
    struct list_t tangents;
    struct list_t tex_coords;
    struct list_t batches;
};

#endif // XCHG_G









