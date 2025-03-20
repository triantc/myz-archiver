#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../structs.h"
#include "../utils.h"
#include "q_flag.h"

void query_archive(const char *archive_name, char *queries[], int query_count)
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
    fclose(archive);

    for (int i = 0; i < query_count; i++) {
        int found = 0;
        for (size_t j = 0; j < meta_count; j++) {
            if (strcmp(queries[i], metas[j].path) == 0) {
                found = 1;
                break;
            }
        }
        printf("%s: %s\n", queries[i], found ? "YES" : "NO");
    }
    free(metas);
}
