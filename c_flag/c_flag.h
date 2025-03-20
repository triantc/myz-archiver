#ifndef C_FLAG_H
#define C_FLAG_H

/* 
 * Creates a new archive file.
 * archive_name: The name/path of the archive to be created.
 * files: Array of file/directory paths to include in the archive.
 * file_count: Number of items in 'files'.
 */
void create_archive(const char *archive_name, char *files[], int file_count);

#endif // C_FLAG_H
