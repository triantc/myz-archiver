#ifndef UTILS_H
#define UTILS_H

#include "structs.h"
#include <stdio.h>

void mode_to_string(mode_t mode, char *str);
void init_metadata_array(MetadataArray *arr);
void add_metadata(MetadataArray *arr, FileMetadata meta);
void free_metadata_array(MetadataArray *arr);
void ensure_parent_dirs(const char *filepath);
void generate_unique_filename(char *filepath);
int should_extract(const char *metadata_path, char **filter, int filter_count);
void get_top_component(const char *path, char *top, size_t size);
void process_path(const char *path, FILE *archive, long *data_offset, MetadataArray *marr);
void compress_file_to_archive(const char *fs_path, FILE *archive, long *data_offset, off_t *size_out);

#endif // UTILS_H
