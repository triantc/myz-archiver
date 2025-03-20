#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <dirent.h>
#include <sys/wait.h>
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>

extern int compress_flag;

// Turns access rights into a string representation
void mode_to_string(mode_t mode, char *str) {
    str[0] = (mode & S_IRUSR) ? 'r' : '-';
    str[1] = (mode & S_IWUSR) ? 'w' : '-';
    str[2] = (mode & S_IXUSR) ? 'x' : '-';
    str[3] = (mode & S_IRGRP) ? 'r' : '-';
    str[4] = (mode & S_IWGRP) ? 'w' : '-';
    str[5] = (mode & S_IXGRP) ? 'x' : '-';
    str[6] = (mode & S_IROTH) ? 'r' : '-';
    str[7] = (mode & S_IWOTH) ? 'w' : '-';
    str[8] = (mode & S_IXOTH) ? 'x' : '-';
    str[9] = '\0';
}

// Initializes the metadata array
void init_metadata_array(MetadataArray *arr) {
    arr->count = 0;
    arr->capacity = 10;
    arr->records = malloc(arr->capacity * sizeof(FileMetadata));
    if (!arr->records) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
}

// Adds a metadata record to the array
void add_metadata(MetadataArray *arr, FileMetadata meta) {
    if (arr->count == arr->capacity) {
        arr->capacity *= 2;
        arr->records = realloc(arr->records, arr->capacity * sizeof(FileMetadata));
        if (!arr->records) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
    }
    arr->records[arr->count++] = meta;
}

// Frees the metadata array
void free_metadata_array(MetadataArray *arr) {
    free(arr->records);
    arr->records = NULL;
    arr->count = arr->capacity = 0;
}

// Ensures that the parent directories of a file exist
void ensure_parent_dirs(const char *filepath) {
    char path_copy[1024];
    strncpy(path_copy, filepath, sizeof(path_copy));
    path_copy[sizeof(path_copy) - 1] = '\0';
    char *dir = dirname(path_copy);
    struct stat st;
    if (stat(dir, &st) != 0) {
        // Recursively create parent directories
        ensure_parent_dirs(dir);
        if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
            perror("Error creating parent directories");
        }
    }
}

/* 
   Generates a unique filename by appending a number in parentheses.
   If for example the name "file.c" already exists,
    it produces "file(1).c", "file(2).c", etc.
   Also works for paths with directories, e.g. "dir/file.c" -> "dir/file(1).c"
*/
void generate_unique_filename(char *filepath) {
    char dir[1024] = "";
    char filename[256] = "";
    char candidate[1024] = "";
    char base[256] = "";
    char ext[256] = "";
    char new_filename[256] = "";
    
    // Split the path into directory and filename
    char *slash = strrchr(filepath, '/');
    if (slash) {
        size_t dir_len = slash - filepath + 1; // Include the last slash
        strncpy(dir, filepath, dir_len);
        dir[dir_len] = '\0';
        strncpy(filename, slash + 1, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    } else {
        // No directory part
        strncpy(filename, filepath, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }
    
    // Split the filename into base name and extension
    char *dot = strrchr(filename, '.');
    if (dot) {
        size_t base_len = dot - filename;
        strncpy(base, filename, base_len);
        base[base_len] = '\0';
        strncpy(ext, dot, sizeof(ext) - 1);
        ext[sizeof(ext) - 1] = '\0';
    } else {
        strncpy(base, filename, sizeof(base) - 1);
        base[sizeof(base) - 1] = '\0';
        ext[0] = '\0';
    }
    
    // Check if the base name already has a number in parentheses
    // If it does, increment the number
    // Otherwise, append "(1)"
    int counter = 1;
    int base_len = strlen(base);
    if (base_len >= 3 && base[base_len - 1] == ')') {
        char *open_paren = strrchr(base, '(');
        if (open_paren) {
            int num;
            if (sscanf(open_paren, "(%d)", &num) == 1) {
                counter = num + 1;
                *open_paren = '\0';  // Remove the number from the base name
            }
        }
    }
    
    // Find a unique filename
    while (1) {
        snprintf(new_filename, sizeof(new_filename), "%s(%d)%s", base, counter, ext);
        snprintf(candidate, sizeof(candidate), "%s%s", dir, new_filename);
        if (access(candidate, F_OK) != 0) {
            // File does not exist
            strncpy(filepath, candidate, MAX_PATH_LENGTH - 1);
            filepath[MAX_PATH_LENGTH - 1] = '\0';
            break;
        }
        counter++;
    }
}

// Checks if a path should be extracted based on a list of filters
int should_extract(const char *metadata_path, char **filter, int filter_count) {
    if (filter_count == 0)
        return 1;
    for (int i = 0; i < filter_count; i++) {
        int len = strlen(filter[i]);
        // Exact match
        if (strcmp(metadata_path, filter[i]) == 0)
            return 1;
        // If the path starts with the filter and is followed by a slash or the end of the string or '(' (for names with collisions, e.g. "abc(1)")
        if (strncmp(metadata_path, filter[i], len) == 0) {
            char c = metadata_path[len];
            if (c == '/' || c == '\0' || c == '(')
                return 1;
        }
    }
    return 0;
}

// Extracts the top component of a path (the first directory or filename)
void get_top_component(const char *path, char *top, size_t size) {
    const char *slash = strchr(path, '/');
    if (slash) {
        size_t len = slash - path;
        if (len >= size) len = size - 1;
        strncpy(top, path, len);
        top[len] = '\0';
    } else {
        strncpy(top, path, size - 1);
        top[size - 1] = '\0';
    }
}

// Compresses a file to an archive
void compress_file_to_archive(const char *fs_path, FILE *archive, long *data_offset, off_t *size_out) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe error");
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork error");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    if (pid == 0) {
        // Child process
        close(pipefd[0]); // close read end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2 error");
            exit(1);
        }
        close(pipefd[1]);
        execlp("gzip", "gzip", "-c", fs_path, (char *)NULL);
        perror("execlp error");
        exit(1);
    }
    // Parent process
    close(pipefd[1]); // close write end
    char buffer[1024];
    ssize_t bytes;
    off_t total_bytes = 0;
    while ((bytes = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, bytes, archive) != (size_t)bytes) {
            perror("Error writing compressed data to archive");
            break;
        }
        total_bytes += bytes;
        *data_offset += bytes;
    }
    close(pipefd[0]);
    int status;
    waitpid(pid, &status, 0);
    *size_out = total_bytes;
}

// Manages files, directories, symlinks, and hard links
// For regular files: if compress_flag is active, reads through compress_file_to_archive
// Also checks if the inode has already been stored (hard link): if so, sets is_hardlink = 1 and
// copies the data_offset from the first occurrence (without storing data again)
// For symlinks: reads the target with readlink and stores it in link_target
void process_path(const char *path, FILE *archive, long *data_offset, MetadataArray *marr) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        perror("lstat error");
        return;
    }
    FileMetadata meta;
    memset(&meta, 0, sizeof(meta));
    strncpy(meta.path, path, MAX_PATH_LENGTH-1);
    meta.path[MAX_PATH_LENGTH-1] = '\0';
    meta.mode = st.st_mode;
    meta.uid = st.st_uid;
    meta.gid = st.st_gid;
    meta.atime = st.st_atime;
    meta.mtime = st.st_mtime;
    meta.ctime = st.st_ctime;
    meta.inode = st.st_ino;
    meta.is_hardlink = 0;
    meta.link_target[0] = '\0';
    
    if (S_ISDIR(st.st_mode)) {
        meta.data_offset = 0;
        add_metadata(marr, meta);
        DIR *dir = opendir(path);
        if (!dir) {
            perror("opendir error");
            return;
        }
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0)
                continue;
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            process_path(full_path, archive, data_offset, marr);
        }
        closedir(dir);
    }
    else if (S_ISLNK(st.st_mode)) {
        // Read the target of the symlink
        ssize_t len = readlink(path, meta.link_target, MAX_PATH_LENGTH-1);
        if (len == -1) {
            perror("readlink error");
            meta.link_target[0] = '\0';
        } else {
            meta.link_target[len] = '\0';
        }
        meta.data_offset = 0;
        add_metadata(marr, meta);
    }
    else if (S_ISREG(st.st_mode)) {
        // Check if the file is a hard link by comparing inodes
        for (size_t i = 0; i < marr->count; i++) {
            if (!S_ISDIR(marr->records[i].mode) && 
                marr->records[i].inode == st.st_ino) {
                // Same inode, hard link
                meta.is_hardlink = 1;
                meta.data_offset = marr->records[i].data_offset;
                meta.size = 0;
                add_metadata(marr, meta);
                return;
            }
        }
        // If the file is not a hard link, store the data
        meta.data_offset = *data_offset;
        char buffer[1024];
        size_t bytes;
        if (compress_flag) {
            off_t comp_size = 0;
            compress_file_to_archive(path, archive, data_offset, &comp_size);
            meta.size = comp_size;
        } else {
            FILE *file = fopen(path, "rb");
            if (!file) {
                perror("Error opening file for archiving");
                return;
            }
            while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                if (fwrite(buffer, 1, bytes, archive) != bytes) {
                    perror("Error writing file data to archive");
                    fclose(file);
                    return;
                }
                *data_offset += bytes;
            }
            fclose(file);
            meta.size = st.st_size;
        }
        add_metadata(marr, meta);
    }
    else {
        fprintf(stderr, "Skipping unsupported file type: %s\n", path);
    }
}