#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <utime.h>
#include <libgen.h>
#include <sys/types.h>
#include "../structs.h"
#include "../utils.h"
#include "x_flag.h"

/*
 * This function extracts all items from the archive, optionally filtering
 * which paths are extracted (if filter_count > 0).
 * Compressed files are automatically decompressed.
 * Hard links and symbolic links are recreated appropriately.
 */
 void extract_archive(const char *archive_name, char **filter, int filter_count) {
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
    
    /* Extract directories first */
    for (size_t i = 0; i < meta_count; i++) {
        if (!should_extract(metas[i].path, filter, filter_count))
            continue;
        if (S_ISDIR(metas[i].mode)) {
            ensure_parent_dirs(metas[i].path);
            if (access(metas[i].path, F_OK) != 0) {
                if (mkdir(metas[i].path, metas[i].mode) != 0 && errno != EEXIST) {
                    perror("Error creating directory");
                }
            }
        }
    }
    
    /* Extract regular files, hard links, and symbolic links */
    for (size_t i = 0; i < meta_count; i++) {
        if (!should_extract(metas[i].path, filter, filter_count))
            continue;
        if (S_ISREG(metas[i].mode)) {
            if (metas[i].is_hardlink) {
                /* For hard links: find the first occurrence (not marked as hard link)
                   with the same inode */
                const char *orig_path = NULL;
                for (size_t j = 0; j < i; j++) {
                    if (!S_ISDIR(metas[j].mode) && !metas[j].is_hardlink &&
                        metas[j].inode == metas[i].inode) {
                        orig_path = metas[j].path;
                        break;
                    }
                }
                if (orig_path) {
                    if (link(orig_path, metas[i].path) == -1) {
                        perror("Error creating hard link");
                    } else {
                        printf("Created hard link: %s -> %s\n", metas[i].path, orig_path);
                    }
                    continue;  // No need to extract file data again
                }
            }
            /* Extract regular file (or first occurrence of a hard link) */
            char extraction_path[1024];
            strncpy(extraction_path, metas[i].path, sizeof(extraction_path));
            extraction_path[sizeof(extraction_path)-1] = '\0';
            
            /* Ensure the parent directories exist before creating the file */
            ensure_parent_dirs(extraction_path);

            if (access(extraction_path, F_OK) == 0) {
                generate_unique_filename(extraction_path);
                printf("File collision: extracted file renamed to '%s'.\n  Original archive path: '%s'\n",
                       extraction_path, metas[i].path);
            }
            FILE *out = fopen(extraction_path, "wb");
            if (!out) {
                perror("Error creating output file");
                continue;
            }
            if (fseek(archive, metas[i].data_offset, SEEK_SET) != 0) {
                perror("fseek error");
                fclose(out);
                continue;
            }
            unsigned char magic[2];
            if (fread(magic, 1, 2, archive) != 2) {
                perror("Error reading magic bytes");
                fclose(out);
                continue;
            }
            if (fseek(archive, metas[i].data_offset, SEEK_SET) != 0) {
                perror("fseek error");
                fclose(out);
                continue;
            }
            if (magic[0] == 0x1F && magic[1] == 0x8B) {
                /* Compressed file: decompress via gunzip */
                unsigned char *comp_data = malloc((size_t)metas[i].size);
                if (!comp_data) {
                    perror("malloc error");
                    fclose(out);
                    continue;
                }
                if (fread(comp_data, 1, (size_t)metas[i].size, archive) != (size_t)metas[i].size) {
                    perror("Error reading compressed data");
                    free(comp_data);
                    fclose(out);
                    continue;
                }
                char temp_filename[] = "/tmp/myzXXXXXX";
                int temp_fd = mkstemp(temp_filename);
                if (temp_fd == -1) {
                    perror("mkstemp error");
                    free(comp_data);
                    fclose(out);
                    continue;
                }
                if (write(temp_fd, comp_data, metas[i].size) != metas[i].size) {
                    perror("Error writing to temp file");
                    close(temp_fd);
                    remove(temp_filename);
                    free(comp_data);
                    fclose(out);
                    continue;
                }
                close(temp_fd);
                free(comp_data);
                char cmd[1024];
                snprintf(cmd, sizeof(cmd), "gunzip -c %s", temp_filename);
                FILE *pipe = popen(cmd, "r");
                if (!pipe) {
                    perror("popen error");
                    remove(temp_filename);
                    fclose(out);
                    continue;
                }
                char buffer[1024];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
                    fwrite(buffer, 1, bytes, out);
                }
                pclose(pipe);
                remove(temp_filename);
            } else {
                off_t remaining = metas[i].size;
                char buffer[1024];
                while (remaining > 0) {
                    off_t chunk_off = (remaining < (off_t)sizeof(buffer)) ? remaining : (off_t)sizeof(buffer);
                    size_t chunk = (size_t)chunk_off;
                    size_t bytes = fread(buffer, 1, chunk, archive);
                    if (bytes != chunk) {
                        perror("Error reading file data");
                        break;
                    }
                    fwrite(buffer, 1, bytes, out);
                    remaining -= (off_t)bytes;
                }
            }
            fclose(out);
            chmod(extraction_path, metas[i].mode);
            chown(extraction_path, metas[i].uid, metas[i].gid);
            struct utimbuf times;
            times.actime = metas[i].atime;
            times.modtime = metas[i].mtime;
            utime(extraction_path, &times);
        }
        else if (S_ISLNK(metas[i].mode)) {
            /* For symbolic links: create the symlink using the stored target */
            if (symlink(metas[i].link_target, metas[i].path) == -1) {
                perror("Error creating symbolic link");
            } else {
                printf("Created symbolic link: %s -> %s\n", metas[i].path, metas[i].link_target);
            }
        }
    }
    
    free(metas);
    fclose(archive);
    printf("Archive %s extracted successfully.\n", archive_name);
}

