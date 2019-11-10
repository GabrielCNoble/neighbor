#include "path.h"
#include <string.h>
#include <limits.h>



char *get_file_path(char *file_name)
{
    static char path[PATH_MAX];
    int index;
    int len;

    len = strlen(file_name);

    file_name = format_path(file_name);

    for(index = len; index >= 0; index--)
    {
        if(file_name[index] == '/')
        {
            break;
        }
    }

    if(index < 0)
    {
        strcpy(path, "./");
    }
    else
    {
        path[index] = '\0';

        for(index--; index >= 0; index--)
        {
            path[index] = file_name[index];
        }
    }

    return path;
}


char *format_path(char *path)
{
    static char formatted_path[PATH_MAX];
    int i;
    int j;

    memset(formatted_path, 0, PATH_MAX);

    for(i = 0, j = 0; path[i]; i++)
    {
        formatted_path[j] = path[i];

        if(path[i] == '\\')
        {
            formatted_path[j] = '/';

            if(path[i + 1] == '\\')
            {
                i++;
            }
        }

        j++;
    }

    formatted_path[j] = '\0';

    return formatted_path;
}

char *append_path_segment(char *path, char *segment)
{
    static char new_path[PATH_MAX];
    
    int path_len = strlen(path);
    path_len = path_len ? path_len - 1: path_len;
    
    strcpy(new_path, path);

    if(path[path_len] != '/')
    {
        strcat(new_path, "/");
    }
    
    strcat(new_path, format_path(segment));

    return new_path;
}
