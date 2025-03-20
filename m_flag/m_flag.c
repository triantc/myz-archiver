#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../structs.h"
#include "../utils.h"
#include "m_flag.h"

void print_metadata_from_archive(const char *archive_name)
{
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Error opening archive");
        return;
    }
    ArchiveHeader header;
    if (fread(&header, 1, HEADER_SIZE, archive) != HEADER_SIZE) {
        perror("Error reading header");
        fclose(archive);
        return;
    }
    size_t meta_count = header.metadata_count;
    FileMetadata *metas = malloc(meta_count * sizeof(FileMetadata));
    if (!metas) {
        perror("malloc");
        fclose(archive);
        return;
    }
    if (fseek(archive, header.metadata_offset, SEEK_SET) != 0) {
        perror("fseek error");
        free(metas);
        fclose(archive);
        return;
    }
    if (fread(metas, sizeof(FileMetadata), meta_count, archive) != meta_count) {
        perror("Error reading metadata");
        free(metas);
        fclose(archive);
        return;
    }
    for (size_t i = 0; i < meta_count; i++) {
        printf("Path: %s\n", metas[i].path);
        printf("Owner (UID): %u\n", metas[i].uid);
        printf("Group (GID): %u\n", metas[i].gid);
        char mode_str[10];
        mode_to_string(metas[i].mode, mode_str);
        printf("Permissions: %s\n", mode_str);
        printf("--------------------------\n");
    }
    free(metas);
    fclose(archive);
}
