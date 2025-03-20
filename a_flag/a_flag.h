#ifndef A_FLAG_H
#define A_FLAG_H

/*
 * Appends new entities to an existing archive.
 * archive_name: The existing archive to append to.
 * files: Array of file/directory paths to add.
 * file_count: Number of items in 'files'.
 */
void append_archive(const char *archive_name, char *files[], int file_count);

#endif // A_FLAG_H
