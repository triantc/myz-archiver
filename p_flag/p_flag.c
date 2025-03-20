#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include "../structs.h"
#include "../utils.h"
#include "p_flag.h"

static int count_slashes_local(const char *s)
{
    int count = 0;
    for (; *s; s++) {
        if (*s == '/')
            count++;
    }
    return count;
}

static int cmp_metadata_local(const void *a, const void *b)
{
    const FileMetadata *ma = (const FileMetadata *)a;
    const FileMetadata *mb = (const FileMetadata *)b;
    return strcmp(ma->path, mb->path);
}

void print_hierarchy(const char *archive_name)
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

    qsort(metas, meta_count, sizeof(FileMetadata), cmp_metadata_local);

    for (size_t i = 0; i < meta_count; i++) {
        int depth = count_slashes_local(metas[i].path);
        for (int d = 0; d < depth; d++) {
            printf("  ");
        }
        char *name = basename(metas[i].path);
        if (S_ISDIR(metas[i].mode))
            printf("%s/\n", name);
        else
            printf("%s\n", name);
    }
    free(metas);
}
