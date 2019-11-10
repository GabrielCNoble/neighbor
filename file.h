#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdint.h>


// struct file_entry_info_t
// {
//     char entry_name[64];
//     uint64_t entry_size;    /* how many bytes of data, NOT including the size of this struct. */
// };

// static char file_entry_header_tag[] = "[entry start]";

// struct file_entry_header_t
// {
//     char tag[(sizeof(file_entry_header_tag) + 3) &  (~3)];
//     struct file_entry_info_t info;
// };

// static char file_entry_tail_tag[] = "[entry end]";

// struct file_entry_tail_t
// {
//     char tag[(sizeof(file_entry_tail_tag) + 3) & (~3)];
// };





// struct file_section_info_t
// {
//     char section_name[64];
//     uint64_t section_size;      /* how many bytes of data, NOT including the size of this struct. */
//     uint64_t entry_count;
// };

// static char file_section_header_tag[] = "[section start]";

// struct file_section_header_t
// {
//     char tag[(sizeof(file_section_header_tag) + 3) & (~3)];
//     struct file_section_info_t info;
// };

// static char file_section_tail_tag[] = "[section end]";

// struct file_section_tail_t
// {
//     char tag[(sizeof(file_section_tail_tag) + 3) & (~3)];
// };






// struct file_entry_t
// {
//     struct file_entry_t *next;
//     struct file_entry_info_t info;
//     void *entry_buffer;
// };

// struct file_section_t
// {
//     struct file_section_info_t info;
//     void *section_buffer;
//     struct file_entry_t *entries;
//     struct file_entry_t *last_entry;
// };



long file_size(FILE *file);

void read_file(FILE *file, void **buffer, long *buffer_size);

void write_file(void **buffer, long *buffer_size);

int file_exists(char *file_name);


// struct file_section_t file_CreateFileSection(char *section_name);

// void file_FreeSection(struct file_section_t *section);

// struct file_entry_t *file_CreateEntry(void *buffer, uint64_t buffer_size, char *name);

// void file_AddEntry(struct file_section_t *section, struct file_entry_t *entry);

// struct file_entry_t *file_RemoveEntry(struct file_section_t *section, char *entry);

// void file_SerializeSection(struct file_section_t *section, void **buffer, uint64_t *buffer_size);

// struct file_section_t file_DeserializeSection(void **buffer);

// struct file_entry_t *file_GetEntry(struct file_section_t *section, char *entry);






#endif // FILE_H






