#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../structs.h"
#include "../utils.h"
#include "c_flag.h"

/* External global flag for compression (declared in myz.c) */
extern int compress_flag;

void create_archive(const char *archive_name, char *files[], int file_count)
{
    FILE *archive = fopen(archive_name, "wb+");
    if (!archive) {
        perror("Error creating archive");
        return;
    }
    /* Reserve space for header */
    if (fseek(archive, HEADER_SIZE, SEEK_SET) != 0) {
        perror("fseek error");
        fclose(archive);
        return;
    }
    long data_offset = HEADER_SIZE;

    MetadataArray marr;
    init_metadata_array(&marr);

    /* Process each file/directory */
    for (int i = 0; i < file_count; i++) {
        process_path(files[i], archive, &data_offset, &marr);
    }

    long metadata_offset = data_offset;
    /* Write all metadata entries */
    for (size_t i = 0; i < marr.count; i++) {
        if (fwrite(&marr.records[i], sizeof(FileMetadata), 1, archive) != 1) {
            perror("Error writing metadata");
            fclose(archive);
            free_metadata_array(&marr);
            return;
        }
    }

    /* Construct header */
    ArchiveHeader header;
    header.metadata_count = marr.count;
    header.metadata_offset = metadata_offset;
    memset(header.reserved, 0, sizeof(header.reserved));

    /* Write header at the beginning */
    if (fseek(archive, 0, SEEK_SET) != 0) {
        perror("fseek error");
        fclose(archive);
        free_metadata_array(&marr);
        return;
    }
    if (fwrite(&header, 1, HEADER_SIZE, archive) != HEADER_SIZE) {
        perror("Error writing header");
    }

    fclose(archive);
    free_metadata_array(&marr);
    printf("Archive %s created successfully.\n", archive_name);
}
