
#include "obj.h"
#include "file.h"
#include "path.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>


struct face_t
{
    struct list_t vertices;
    char material[64];
};

struct face_vertice_t
{
    int indices[3];
};


void load_wavefront(char *file_name,  struct geometry_data_t *geometry_data)
{
    geometry_data->vertices = create_list(sizeof(vec3_t), 32);
    geometry_data->normals = create_list(sizeof(vec3_t), 32);
    geometry_data->tangents = create_list(sizeof(vec3_t), 32);
    geometry_data->tex_coords = create_list(sizeof(vec2_t), 32);
    geometry_data->batches = create_list(sizeof(struct batch_data_t), 32);

    // printf("here\n");
//    geometry_data->materials.init(sizeof(struct material_data_t), 32);



    struct list_t *attrib_list;

    struct list_t faces = create_list(sizeof(struct face_t), 32);
    struct list_t vertices = create_list(sizeof(vec3_t), 32);
    struct list_t normals = create_list(sizeof(vec3_t), 32);
    struct list_t tex_coords = create_list(sizeof(vec3_t), 32);

    struct batch_data_t *batch;

    struct face_t *face;
    struct face_vertice_t face_vertice;

    int i = 0;
    int j = 0;
    int k = 0;
    char *file_buffer;
    long file_size;
    vec4_t vec_attrib;
    int attrib_size;
    FILE *file;

//    short current_material;
//    struct material_data_t *current_material;
    struct batch_data_t *current_batch;
    int value_string_index;
    char value_string[128];
    char file_path[PATH_MAX];
    char material_path[PATH_MAX];

    file = fopen(file_name, "rb");

    if(file)
    {
        read_file(file, (void **)&file_buffer, &file_size);
        fclose(file);

        strcpy(file_path, get_file_path(file_name));

        while(i < file_size)
        {
            switch(file_buffer[i])
            {
                case 'v':
                    i++;

                    if(file_buffer[i] == 't')
                    {
                        attrib_list = &tex_coords;
                        attrib_size = 2;
                        i++;
                    }
                    else
                    {
                        attrib_size = 3;
                        if(file_buffer[i] == 'n')
                        {
                            attrib_list = &normals;
                            i++;
                        }
                        else
                        {
                            attrib_list = &vertices;
                        }
                    }

                    for(j = 0; j < attrib_size; j++)
                    {
                        while(file_buffer[i] == ' ')i++;

                        value_string_index = 0;

                        while((file_buffer[i] >= '0' && file_buffer[i] <= '9') || file_buffer[i] == '.' || file_buffer[i] == '-')
                        {
                            value_string[value_string_index] = file_buffer[i];
                            value_string_index++;
                            i++;
                        }

                        value_string[value_string_index] = '\0';

                        vec_attrib.comps[j] = atof(value_string);
                    }

                    add_list_element(attrib_list, &vec_attrib);
                break;

                case 'f':
                    i++;
                    /* new face starts... */
                    face = (struct face_t *)get_list_element(&faces, add_list_element(&faces, NULL));
                    face->vertices = create_list(sizeof(struct face_vertice_t), 3);
                    strcpy(face->material, current_batch->material);

//                    batch = obj_GetBatch()

//                    for(j = 0; j < geometry_data->batches.cursor; j++)
//                    {
//                        /* try to find a batch that has this material... */
//                        batch = (struct batch_data_t *)geometry_data->batches.get(j);
//
//                        if(!strcmp(batch->material, current_material->name))
//                        {
//                            break;
//                        }
//                    }
//
//                    if(j >= geometry_data->batches.cursor)
//                    {
//                        /* no batch using the current material exists, so create a new one... */
//                        batch = (struct batch_data_t *)geometry_data->batches.get(geometry_data->batches.add(NULL));
//                        strcpy(batch->material, current_material->name);
//                    }

                    while(file_buffer[i] != '\0' && file_buffer[i] != '\n' &&
                          file_buffer[i] != '\r' && file_buffer[i] != '\t')

                    {
                        for(j = 0; j < 3; j++)
                        {
                            while(file_buffer[i] == ' ')i++;

                            value_string_index = 0;

                            while(file_buffer[i] >= '0' && file_buffer[i] <= '9')
                            {
                                value_string[value_string_index] = file_buffer[i];
                                value_string_index++;
                                i++;
                            }

                            value_string[value_string_index] = '\0';

                            if(file_buffer[i] == '/')
                            {
                                i++;
                            }

                            if(value_string_index)
                            {
                                face_vertice.indices[j] = atoi(value_string) - 1;
                            }
                            else
                            {
                                face_vertice.indices[j] = -1;
                            }
                        }

                        add_list_element(&face->vertices, &face_vertice);
                        while(file_buffer[i] == ' ')i++;
                    }
                break;

                case '#':
                case 's':
                    while(file_buffer[i] != '\0' && file_buffer[i] != '\n' && file_buffer[i] != '\r') i++;
                break;

                case 'u':

                    i++;

                    if(file_buffer[i    ] == 's' &&
                       file_buffer[i + 1] == 'e' &&
                       file_buffer[i + 2] == 'm' &&
                       file_buffer[i + 3] == 't' &&
                       file_buffer[i + 4] == 'l' &&
                       file_buffer[i + 5] == ' ')
                    {
                        i += 6;

                        while(file_buffer[i] == ' ') i++;

                        value_string_index = 0;

                        while(file_buffer[i] != ' ' &&
                              file_buffer[i] != '\n' &&
                              file_buffer[i] != '\r' &&
                              file_buffer[i] != '\0' &&
                              file_buffer[i] != '\t')
                        {
                            value_string[value_string_index] = file_buffer[i];
                            value_string_index++;
                            i++;
                        }

                        value_string[value_string_index] = '\0';

//                        current_material = r_GetMaterialHandle(value_string);
                        current_batch = get_wavefront_batch(value_string, geometry_data);
                    }
                    else
                    {
                        i++;
                    }
                break;

                case 'm':
                    i++;

                    if(file_buffer[i    ] == 't' &&
                       file_buffer[i + 1] == 'l' &&
                       file_buffer[i + 2] == 'l' &&
                       file_buffer[i + 3] == 'i' &&
                       file_buffer[i + 4] == 'b' &&
                       file_buffer[i + 5] == ' ')
                    {
                        i += 6;

                        while(file_buffer[i] == ' ') i++;

                        value_string_index = 0;

                        while(file_buffer[i] != ' ' &&
                              file_buffer[i] != '\n' &&
                              file_buffer[i] != '\r' &&
                              file_buffer[i] != '\0' &&
                              file_buffer[i] != '\t')
                        {
                            value_string[value_string_index] = file_buffer[i];
                            value_string_index++;
                            i++;
                        }

                        value_string[value_string_index] = '\0';

                        strcpy(material_path, append_path_segment(file_path, value_string));
                        load_wavefront_mtl(material_path, geometry_data);
                    }
                    else
                    {
                        i++;
                    }
                break;

                default:
                    i++;
                break;
            }
        }

        for(k = 0; k < geometry_data->batches.cursor; k++)
        {
            batch = (struct batch_data_t *)get_list_element(&geometry_data->batches, k);

            if(k)
            {
                batch->start = (batch - 1)->start + (batch - 1)->count;
            }

            for(i = 0; i < faces.cursor; i++)
            {
                face = (struct face_t *)get_list_element(&faces, i);

                if(!strcmp(face->material, batch->material))
                {
                    for(j = 0; j < face->vertices.cursor; j++)
                    {
                        face_vertice = *(struct face_vertice_t *)get_list_element(&face->vertices, j);

                        add_list_element(&geometry_data->vertices, get_list_element(&vertices, face_vertice.indices[0]));
                        add_list_element(&geometry_data->normals, get_list_element(&normals, face_vertice.indices[2]));

                        if(face_vertice.indices[1] != -1)
                        {
                            add_list_element(&geometry_data->tex_coords, get_list_element(&tex_coords, face_vertice.indices[1]));
                        }
                    }

                    batch->count += face->vertices.cursor;
                }
            }
        }
    }
}


void load_wavefront_mtl(char *file_name, struct geometry_data_t *geometry_data)
{
    int i = 0;
    int j;
    char *file_buffer;
    long file_size;
//    struct material_data_t *current_material = NULL;
    struct batch_data_t *current_batch = NULL;
    int value_str_index;
    unsigned short material_handle;
    char value_str[64];
    vec4_t color;
    FILE *file;

    int texture_type;

    file = fopen(file_name, "rb");

    if(file)
    {
//        file_buffer = (char *)aux_ReadFile(file);
//        file_size = aux_FileSize(file);
//        fclose(file);
        read_file(file, (void **)&file_buffer, &file_size);
        fclose(file);

//        fseek(file, 0, SEEK_END);
//        file_size = ftell(file);
//        rewind(file);
//        file_buffer = (char *)calloc(file_size + 1, 1);
//        fread(file_buffer, file_size, 1, file);
//        fclose(file);
//        file_buffer[file_size] = '\0';

        while(i < file_size)
        {
            switch(file_buffer[i])
            {
                case 'K':
                    i++;

                    if(file_buffer[i] == 'a' || file_buffer[i] == 's')
                    {
                        while(file_buffer[i] != '\n' && file_buffer[i] != '\r' && file_buffer[i] != '\0')i++;
                    }
                    else if(file_buffer[i] == 'd')
                    {
                        i++;

                        for(j = 0; j < 3; j++)
                        {
                            value_str_index = 0;

                            while(file_buffer[i] == ' ') i++;

                            while(file_buffer[i] != ' ' &&
                                  file_buffer[i] != '\n' &&
                                  file_buffer[i] != '\r' &&
                                  file_buffer[i] != '\0' &&
                                  file_buffer[i] != '\t')
                            {
                                value_str[value_str_index] = file_buffer[i];
                                i++;
                                value_str_index++;
                            }

                            value_str[value_str_index] = '\0';

                            color.comps[j] = atof(value_str);
                        }

                        color.comps[3] = 1.0;

                        current_batch->base_color = color;

//                        r_SetMaterialColorPointer(current_material, color);
                    }
                break;

                case 'm':
                    i++;

                    if(file_buffer[i    ] == 'a' &&
                       file_buffer[i + 1] == 'p' &&
                       file_buffer[i + 2] == '_')
                    {
                        i += 3;

                        if(file_buffer[i    ] == 'K' &&
                           file_buffer[i + 1] == 'd')
                        {
                            i += 2;
                            texture_type = 0;
                        }
                        else if(file_buffer[i   ] == 'B' &&
                                file_buffer[i + 1] == 'u' &&
                                file_buffer[i + 2] == 'm' &&
                                file_buffer[i + 3] == 'p')
                        {
                            i += 4;
                            texture_type = 1;
                        }
                        else if(file_buffer[i    ] == 'K' &&
                                file_buffer[i + 1] == 'a')
                        {
                            i += 2;
                            texture_type = 2;

                            /* don't care about ambient texture... */
                            while(file_buffer[i] != '\n' && file_buffer[i] != '\0')i++;
                        }

                        /* according to the format specification there can be options
                        and arguments for those options before the actual texture name. We
                        won't bother with those, since they won't be used. The loader may
                        fail in loading the texture file if there are options specified, but
                        until this day comes we won't bother dealing with that... */

                        while(file_buffer[i] == ' ') i++;

                        value_str_index = 0;
                        while(file_buffer[i] != '\n' && file_buffer[i] != '\t' &&
                              file_buffer[i] != '\0')
                        {
                            value_str[value_str_index] = file_buffer[i];
                            value_str_index++;
                            i++;
                        }

                        value_str[value_str_index] = '\0';

                        switch(texture_type)
                        {
                            case 0:
                                strcpy(current_batch->diffuse_texture, value_str);
                            break;

                            case 1:
                                strcpy(current_batch->normal_texture, value_str);
                            break;
                        }
                    }
                break;

                case 'n':
                    i++;

                    if(file_buffer[i] == 'e' &&
                       file_buffer[i + 1] == 'w' &&
                       file_buffer[i + 2] == 'm' &&
                       file_buffer[i + 3] == 't' &&
                       file_buffer[i + 4] == 'l' &&
                       file_buffer[i + 5] == ' ')
                    {
                        i += 6;

                        while(file_buffer[i] == ' ')i++;

                        value_str_index = 0;

                        while(file_buffer[i] != '\n' &&
                              file_buffer[i] != '\r' && file_buffer[i] != '\0' &&
                              file_buffer[i] != '\t')
                        {
                            value_str[value_str_index] = file_buffer[i];
                            value_str_index++;
                            i++;
                        }

                        value_str[value_str_index] = '\0';

                        current_batch = (struct batch_data_t *)get_list_element(&geometry_data->batches, add_list_element(&geometry_data->batches, NULL));
                        memset(current_batch, 0, sizeof(struct batch_data_t ));
                        strcpy(current_batch->material, value_str);


//                        material_handle = r_CreateEmptyMaterial();
//                        current_material = r_GetMaterialPointer(material_handle);
//
//                        current_material->name = strdup(value_str);
                    }
                    else
                    {
                        i++;
                    }
                break;

                case 'd':
                case '#':
                    while(file_buffer[i] != '\n' && file_buffer[i] != '\r' && file_buffer[i] != '\0' && file_buffer[i] != '\t')i++;
                break;

                case 'i':
                    if(!strcmp(file_buffer + i, "illum"))
                    {
                        while(file_buffer[i] != '\n' && file_buffer[i] != '\r' && file_buffer[i] != '\0' && file_buffer[i] != '\t')i++;
                    }
                    else
                    {
                        i++;
                    }
                break;

                default:
                    i++;
                break;
            }
        }
    }
}


struct batch_data_t *get_wavefront_batch(char *material_name, struct geometry_data_t *geometry_data)
{
    struct batch_data_t *batch;
    int i;
    int c;

    // batches = (struct batch_data_t *)geometry_data->batches.;
    c = geometry_data->batches.cursor;

    for(i = 0; i < c; i++)
    {
        batch = get_list_element(&geometry_data->batches, i);

        if(!strcmp(material_name, batch->material))
        {
            return batch;
        }
    }

    for(i = 0; i < c; i++)
    {
        batch = get_list_element(&geometry_data->batches, i);
        
        if(!strcmp(DEFAULT_MATERIAL_NAME, batch->material))
        {
            return batch;
        }
    }

    struct batch_data_t default_batch = DEFAULT_BATCH;
    i = add_list_element(&geometry_data->batches, &default_batch);
    return (struct batch_data_t *)get_list_element(&geometry_data->batches, i);
}











