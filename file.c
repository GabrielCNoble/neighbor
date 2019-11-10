#include "file.h"
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


long file_size(FILE *file)
{
    long size;
    long offset;

    offset = ftell(file);
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, offset, SEEK_SET);

    return size;
}

void read_file(FILE *file, void **buffer, long *buffer_size)
{
    char *file_buffer = NULL;
    long size = 0;

    if(file)
    {
        size = file_size(file);
        file_buffer = (char *)calloc(size + 1, 1);
        fread(file_buffer, size, 1, file);
        file_buffer[size] = '\0';
    }

    *buffer = (void *)file_buffer;
    *buffer_size = size;
}

void write_file(void **buffer, long *buffer_size)
{

}

int file_exists(char *file_name)
{
    FILE *file;

    file = fopen(file_name, "r");

    if(file)
    {
        fclose(file);
        return 1;
    }

    return 0;
}




// struct file_section_t file_CreateFileSection(char *section_name)
// {
//     struct file_section_t section;

//     memset(&section, 0, sizeof(struct file_section_t));
//     section.info.section_size = 0;
//     strcpy(section.info.section_name, section_name);

//     return section;
// }

// void file_FreeSection(struct file_section_t *section)
// {
//     struct file_entry_t *entry;
//     struct file_entry_t *next_entry;

//     entry = section->entries;

//     if(section->section_buffer)
//     {
//         /* if section_buffer is not null, this section is holding deserialized data. To
//         avoid unnecessary allocations while deserializing, all the entries will be
//         contained inside section_buffer */
//         while(entry)
//         {
//             next_entry = entry->next;

//             if(entry->entry_buffer > ((char *)section->section_buffer + section->info.section_size) ||
//                (char *)entry->entry_buffer + entry->info.entry_size < section->section_buffer)
//             {
//                 /* entry_buffer is not contained in section_buffer, so it's
//                 safe to free it here. */
//                 free(entry->entry_buffer);
//             }

//             if((char *)entry > ((char *)section->section_buffer + section->info.section_size) ||
//                (char *)entry < (char *)section->section_buffer)
//             {
//                 /* entry is not contained in section_buffer, so it's safe to
//                 free it here. */
//                 free(entry);
//             }

//             entry = next_entry;
//         }

//         free(section->section_buffer);
//     }
//     else
//     {
//         /* data is not contained inside section_buffer, so free it all. */
//         while(entry)
//         {
//             next_entry = entry->next;
//             free(entry->entry_buffer);
//             free(entry);
//             entry = next_entry;
//         }
//     }

//     memset(section, 0, sizeof(struct file_section_t));
// }

// struct file_entry_t *file_CreateEntry(void *buffer, uint64_t buffer_size, char *name)
// {
//     struct file_entry_t *entry;

//     entry = (struct file_entry_t *)calloc(sizeof(struct file_entry_t), 1);

//     entry->info.entry_size = buffer_size;
//     strcpy(entry->info.entry_name, name);
//     entry->entry_buffer = buffer;

//     return entry;
// }

// void file_AddEntry(struct file_section_t *section, struct file_entry_t *entry)
// {
//     if(section && entry)
//     {
//         if(!section->entries)
//         {
//             section->entries = entry;
//         }
//         else
//         {
//             section->last_entry->next = entry;
//         }

//         section->last_entry = entry;
//         entry->next = NULL;

//         section->info.section_size += entry->info.entry_size;
//         section->info.entry_count++;
//     }
// }

// struct file_entry_t *file_RemoveEntry(struct file_section_t *section, char *entry)
// {

// }

// void file_SerializeSection(struct file_section_t *section, void **buffer, uint64_t *buffer_size)
// {
//     char *output = NULL;
//     char *out;
//     uint64_t output_size = 0;
//     struct file_section_header_t *section_header;
//     struct file_section_tail_t *section_tail;

//     struct file_entry_header_t *entry_header;
//     struct file_entry_tail_t *entry_tail;

//     struct file_entry_t *entry;

//     if(section)
//     {
//         output_size = sizeof(struct file_section_header_t) +
//                       sizeof(struct file_section_tail_t) +

//                       /* how many bytes stored in the entries, without taking into
//                       account the file_entry_t structs */
//                       section->info.section_size +

//                       /* each entry will need a header and a tail... */
//                       section->info.entry_count * (sizeof(struct file_entry_header_t) + sizeof(struct file_entry_tail_t));

//         output = (char *)calloc(1, output_size);

//         out = output;

//         section_header = (struct file_section_header_t *)out;
//         section_header->info = section->info;
//         strcpy(section_header->tag, file_section_header_tag);
//         out += sizeof(struct file_section_header_t);

//         entry = section->entries;

//         while(entry)
//         {
//             entry_header = (struct file_entry_header_t *)out;
//             entry_header->info = entry->info;
//             strcpy(entry_header->tag, file_entry_header_tag);
//             out += sizeof(struct file_entry_header_t);

//             memcpy(out, entry->entry_buffer, entry->info.entry_size);
//             out += entry->info.entry_size;

//             entry_tail = (struct file_entry_tail_t *)out;
//             strcpy(entry_tail->tag, file_entry_tail_tag);
//             out += sizeof(struct file_entry_tail_t);

//             entry = entry->next;
//         }

//         section_tail = (struct file_section_tail_t *)out;
//         strcpy(section_tail->tag, file_section_tail_tag);

//     }

//     *buffer = output;
//     *buffer_size = output_size;
// }

// struct file_section_t file_DeserializeSection(void **buffer)
// {
//     char *in;
//     char *out;
//     uint64_t output_size;
//     struct file_section_header_t *section_header;
//     struct file_entry_header_t *entry_header;
//     struct file_entry_t *entry;
//     struct file_section_t section;

//     in = *(char **)buffer;

//     memset(&section, 0, sizeof(struct file_section_t));

//     while(in)
//     {
//         if(!strcmp(in, file_section_header_tag))
//         {
//             section_header = (struct file_section_header_t *)in;
//             in += sizeof(struct file_section_header_t);

//                           /* how many bytes were serialized, disregarding headers / tails. */
//             output_size = section_header->info.section_size +

//                           /* to avoid unnecessary allocations during deserialization, a single buffer
//                           will be allocated, and the entries data and buffers will live in it.*/
//                           section_header->info.entry_count * sizeof(struct file_entry_t);

//             section.info = section_header->info;
//             section.section_buffer = calloc(output_size, 1);

//             out = (char *)section.section_buffer;

//             while(strcmp(in, file_section_tail_tag))
//             {
//                 if(!strcmp(in, file_entry_header_tag))
//                 {
//                     entry_header = (struct file_entry_header_t *)in;
//                     in += sizeof(struct file_entry_header_t );

//                     entry = (struct file_entry_t *)out;
//                     out += sizeof(struct file_entry_t);

//                     entry->info = entry_header->info;

//                     entry->entry_buffer = out;
//                     out += entry->info.entry_size;

//                     memcpy(entry->entry_buffer, in, entry->info.entry_size);

//                     if(!section.entries)
//                     {
//                         section.entries = entry;
//                     }
//                     else
//                     {
//                         section.last_entry->next = entry;
//                     }

//                     section.last_entry = entry;

//                     while(strcmp(in, file_entry_tail_tag)) in++;

//                     in += sizeof(struct file_entry_tail_t);
//                 }
//                 else
//                 {
//                     in++;
//                 }
//             }

//             break;
//         }
//         else
//         {
//             in++;
//         }
//     }

//     return section;
// }

// struct file_entry_t *file_GetEntry(struct file_section_t *section, char *entry)
// {

// }




