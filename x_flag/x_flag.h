#ifndef X_FLAG_H
#define X_FLAG_H

/*
 * Extracts the contents of an archive.
 * archive_name: The name/path of the archive to extract.
 * filter: Array of file/directory paths to extract (if any).
 * filter_count: Number of items in 'filter'.
 * If filter_count == 0, everything is extracted.
 */
void extract_archive(const char *archive_name, char **filter, int filter_count);

#endif // X_FLAG_H
