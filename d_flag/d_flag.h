#ifndef D_FLAG_H
#define D_FLAG_H

/*
 * Deletes specified entities from the archive.
 * archive_name: The existing archive.
 * del_list: Array of file/directory paths to remove.
 * del_count: Number of items in 'del_list'.
 */
void delete_entities(const char *archive_name, char *del_list[], int del_count);

#endif // D_FLAG_H
