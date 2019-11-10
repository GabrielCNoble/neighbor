#ifndef OBJ_H
#define OBJ_H


#include "xchg.h"

void load_wavefront(char *file_name, struct geometry_data_t *geometry_data);

void load_wavefront_mtl(char *file_name, struct geometry_data_t *geometry_data);

struct batch_data_t *get_wavefront_batch(char *material_name, struct geometry_data_t *geometry_data);


#endif // OBJ_H
