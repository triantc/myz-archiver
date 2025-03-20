#ifndef STRUCTS_H
#define STRUCTS_H

#include <sys/types.h>
#include <time.h>
#include <stdint.h>

#define MAX_PATH_LENGTH 255
#define HEADER_SIZE 256

typedef struct {
    char path[MAX_PATH_LENGTH];
    mode_t mode;
    uid_t uid;
    gid_t gid;
    off_t size;
    time_t atime;
    time_t mtime;
    time_t ctime;
    long data_offset;
    ino_t inode;                // For hard links
    int is_hardlink;            // 1 if it's a hard link, 0 otherwise
    char link_target[MAX_PATH_LENGTH]; // For symlinks
} FileMetadata;

typedef struct {
    uint32_t metadata_count;
    long metadata_offset;
    char reserved[HEADER_SIZE - sizeof(uint32_t) - sizeof(long)]; // In case I need to add more fields
} ArchiveHeader;

typedef struct {
    FileMetadata *records;
    size_t count;
    size_t capacity;
} MetadataArray;

#endif // STRUCTS_H
