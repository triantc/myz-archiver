#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include "../structs.h"
#include "../utils.h"
#include "d_flag.h"

void delete_entities(const char *archive_name, char *del_list[], int del_count)
{
    FILE *orig = fopen(archive_name, "rb");
    if (!orig) {
        perror("Error opening archive for deletion");
        return;
    }
    ArchiveHeader header;
    if (fread(&header, 1, HEADER_SIZE, orig) != HEADER_SIZE) {
        perror("Error reading header");
        fclose(orig);
        return;
    }
    size_t meta_count = header.metadata_count;
    FileMetadata *metas = malloc(meta_count * sizeof(FileMetadata));
    if (!metas) {
        perror("malloc");
        fclose(orig);
        return;
    }
    if (fseek(orig, header.metadata_offset, SEEK_SET) != 0) {
        perror("fseek error");
        free(metas);
        fclose(orig);
        return;
    }
    if (fread(metas, sizeof(FileMetadata), meta_count, orig) != meta_count) {
        perror("Error reading metadata");
        free(metas);
        fclose(orig);
        return;
    }
    /* Filter out the entries to delete */
    FileMetadata *new_metas = malloc(meta_count * sizeof(FileMetadata));
    if (!new_metas) {
        perror("malloc");
        free(metas);
        fclose(orig);
        return;
    }
    size_t new_count = 0;
    for (size_t i = 0; i < meta_count; i++) {
        int to_delete = 0;
        for (int j = 0; j < del_count; j++) {
            size_t len = strlen(del_list[j]);
            /* If exactly the same path or a subpath (like "dir" -> "dir/file") */
            if (strcmp(metas[i].path, del_list[j]) == 0 ||
                (strncmp(metas[i].path, del_list[j], len) == 0 &&
                 (metas[i].path[len] == '/' || metas[i].path[len] == '\0'))) {
                to_delete = 1;
                break;
            }
        }
        if (!to_delete) {
            new_metas[new_count++] = metas[i];
        }
    }
    free(metas);

    /* Create a temporary archive file */
    char temp_archive_name[] = "temp_archiveXXXXXX";
    int temp_fd = mkstemp(temp_archive_name);
    if (temp_fd == -1) {
        perror("mkstemp error");
        free(new_metas);
        fclose(orig);
        return;
    }
    FILE *temp_archive = fdopen(temp_fd, "wb+");
    if (!temp_archive) {
        perror("fdopen error");
        close(temp_fd);
        remove(temp_archive_name);
        free(new_metas);
        fclose(orig);
        return;
    }
    /* Write placeholder header */
    char header_placeholder[HEADER_SIZE];
    memset(header_placeholder, 0, HEADER_SIZE);
    if (fwrite(header_placeholder, 1, HEADER_SIZE, temp_archive) != HEADER_SIZE) {
        perror("Error writing placeholder header");
        fclose(temp_archive);
        remove(temp_archive_name);
        free(new_metas);
        fclose(orig);
        return;
    }
    long new_data_offset = HEADER_SIZE;
    /* Copy file data for the remaining entries */
    for (size_t i = 0; i < new_count; i++) {
        if (S_ISREG(new_metas[i].mode)) {
            if (fseek(orig, new_metas[i].data_offset, SEEK_SET) != 0) {
                perror("fseek error in original archive");
                continue;
            }
            size_t remaining = (size_t)new_metas[i].size;
            char buffer[1024];
            while (remaining > 0) {
                size_t chunk = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
                size_t r = fread(buffer, 1, chunk, orig);
                if (r != chunk) {
                    perror("Error reading file data from original archive");
                    break;
                }
                if (fwrite(buffer, 1, r, temp_archive) != r) {
                    perror("Error writing file data to new archive");
                    break;
                }
                remaining -= r;
            }
            new_metas[i].data_offset = new_data_offset;
            new_data_offset += new_metas[i].size;
        } else {
            new_metas[i].data_offset = 0;
        }
    }
    /* Write the updated metadata */
    for (size_t i = 0; i < new_count; i++) {
        if (fwrite(&new_metas[i], sizeof(FileMetadata), 1, temp_archive) != 1) {
            perror("Error writing new metadata");
        }
    }
    /* Create new header */
    ArchiveHeader new_header;
    new_header.metadata_count = new_count;
    new_header.metadata_offset = new_data_offset;
    memset(new_header.reserved, 0, sizeof(new_header.reserved));
    if (fseek(temp_archive, 0, SEEK_SET) != 0) {
        perror("fseek error while writing new header");
    }
    if (fwrite(&new_header, 1, HEADER_SIZE, temp_archive) != HEADER_SIZE) {
        perror("Error writing new header");
    }
    fclose(temp_archive);
    free(new_metas);
    fclose(orig);

    /* Replace original archive with the new one */
    if (rename(temp_archive_name, archive_name) != 0) {
        perror("rename error");
        remove(temp_archive_name);
        return;
    }
    printf("Entities deleted successfully from archive %s.\n", archive_name);
}
