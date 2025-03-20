#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include "../structs.h"
#include "../utils.h"
#include "a_flag.h"

void append_archive(const char *archive_name, char *files[], int file_count)
{
    FILE *archive = fopen(archive_name, "r+b");
    if (!archive) {
        perror("Error opening archive for appending");
        return;
    }
    ArchiveHeader header;
    if (fread(&header, 1, HEADER_SIZE, archive) != HEADER_SIZE) {
        perror("Error reading header");
        fclose(archive);
        return;
    }
    uint32_t old_meta_count = header.metadata_count;
    FileMetadata *old_metas = NULL;
    if (old_meta_count > 0) {
        old_metas = malloc(old_meta_count * sizeof(FileMetadata));
        if (!old_metas) {
            perror("malloc");
            fclose(archive);
            return;
        }
        if (fseek(archive, header.metadata_offset, SEEK_SET) != 0) {
            perror("fseek error");
            free(old_metas);
            fclose(archive);
            return;
        }
        if (fread(old_metas, sizeof(FileMetadata), old_meta_count, archive) != old_meta_count) {
            perror("Error reading old metadata");
            free(old_metas);
            fclose(archive);
            return;
        }
    }
    long new_data_offset = header.metadata_offset;
    if (fseek(archive, new_data_offset, SEEK_SET) != 0) {
        perror("fseek error");
        free(old_metas);
        fclose(archive);
        return;
    }
    MetadataArray new_marr;
    init_metadata_array(&new_marr);

    for (int i = 0; i < file_count; i++) {
        struct stat st;
        if (lstat(files[i], &st) == -1) {
            fprintf(stderr, "Error: file/directory '%s' not found on filesystem.\n", files[i]);
            continue;
        }
        char meta_entry[MAX_PATH_LENGTH];
        strncpy(meta_entry, files[i], MAX_PATH_LENGTH);
        meta_entry[MAX_PATH_LENGTH - 1] = '\0';

        int duplicate = 0;
        if (S_ISDIR(st.st_mode)) {
            /* Check if directory already exists */
            for (size_t j = 0; j < old_meta_count; j++) {
                if (S_ISDIR(old_metas[j].mode) && strcmp(old_metas[j].path, meta_entry) == 0) {
                    duplicate = 1;
                    break;
                }
            }
            if (duplicate) {
                fprintf(stderr, "Error: directory '%s' already exists in archive.\n", meta_entry);
                continue;
            }
        } else if (S_ISREG(st.st_mode)) {
            char tmp[1024];
            strncpy(tmp, files[i], sizeof(tmp));
            tmp[sizeof(tmp) - 1] = '\0';
            char *parent = dirname(tmp);
            int parent_exists = 0;
            for (size_t j = 0; j < old_meta_count; j++) {
                if (S_ISDIR(old_metas[j].mode) && strcmp(old_metas[j].path, parent) == 0) {
                    parent_exists = 1;
                    break;
                }
            }
            if (parent_exists) {
                char *base = basename(files[i]);
                strncpy(meta_entry, base, MAX_PATH_LENGTH);
                meta_entry[MAX_PATH_LENGTH - 1] = '\0';
            }
            /* Check for existing file with same path */
            for (size_t j = 0; j < old_meta_count; j++) {
                if (!S_ISDIR(old_metas[j].mode) && strcmp(old_metas[j].path, meta_entry) == 0) {
                    duplicate = 1;
                    break;
                }
            }
            if (duplicate) {
                fprintf(stderr, "Error: file '%s' already exists in archive.\n", meta_entry);
                continue;
            }
        }
        /* Process path for the new data */
        process_path(files[i], archive, &new_data_offset, &new_marr);
    }

    if (new_marr.count == 0) {
        fprintf(stderr, "No new entries were appended.\n");
        free_metadata_array(&new_marr);
        free(old_metas);
        fclose(archive);
        return;
    }
    long new_metadata_offset = new_data_offset;
    if (fseek(archive, new_data_offset, SEEK_SET) != 0) {
        perror("fseek error");
        free_metadata_array(&new_marr);
        free(old_metas);
        fclose(archive);
        return;
    }
    /* Write new metadata */
    for (size_t i = 0; i < new_marr.count; i++) {
        if (fwrite(&new_marr.records[i], sizeof(FileMetadata), 1, archive) != 1) {
            perror("Error writing new metadata");
        }
    }
    /* Write old metadata */
    if (old_meta_count > 0) {
        if (fwrite(old_metas, sizeof(FileMetadata), old_meta_count, archive) != old_meta_count) {
            perror("Error writing old metadata");
        }
    }
    uint32_t total_meta_count = new_marr.count + old_meta_count;
    header.metadata_count = total_meta_count;
    header.metadata_offset = new_metadata_offset;
    if (fseek(archive, 0, SEEK_SET) != 0) {
        perror("fseek error");
        free_metadata_array(&new_marr);
        free(old_metas);
        fclose(archive);
        return;
    }
    if (fwrite(&header, 1, HEADER_SIZE, archive) != HEADER_SIZE) {
        perror("Error writing updated header");
    }
    free_metadata_array(&new_marr);
    free(old_metas);
    fclose(archive);
    printf("Archive %s appended successfully.\n", archive_name);
}
