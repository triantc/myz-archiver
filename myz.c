#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"   // Struct definition (FileMetadata, ArchiveHeader, MetadataArray)
#include "utils.h"     // Helper functions (mode_to_string, init_metadata_array, generate_unique_filename, κλπ.)

#include "c_flag/c_flag.h"   // Flag -c (create archive)
#include "x_flag/x_flag.h"   // Flag -x (extract archive)
#include "a_flag/a_flag.h"   // Flag -a (append)
#include "d_flag/d_flag.h"   // Flag -d (delete entities)
#include "m_flag/m_flag.h"   // Flag -m (print metadata)
#include "q_flag/q_flag.h"   // Flag -q (query if entities exist)
#include "p_flag/p_flag.h"   // Flag -p (print file hierarchy)

/* Global compression flag (-j) */
int compress_flag = 0;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s {-c|-a|-x|-m|-d|-p|-j} <archive-file> [files/dirs...]\nUsage of -j: %s {-c|-a} <archive-file> -j [files/dirs...]\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "-c") == 0) {
        if (argc >= 4 && strcmp(argv[3], "-j") == 0) {
            compress_flag = 1;
            create_archive(argv[2], &argv[4], argc - 4);
        } else {
            create_archive(argv[2], &argv[3], argc - 3);
        }
    } else if (strcmp(argv[1], "-x") == 0) {
        int filter_count = (argc > 3) ? (argc - 3) : 0;
        extract_archive(argv[2], &argv[3], filter_count);
    } else if (strcmp(argv[1], "-a") == 0) {
        if (argc >= 4 && strcmp(argv[3], "-j") == 0) {
            compress_flag = 1;
            append_archive(argv[2], &argv[4], argc - 4);
        } else {
            append_archive(argv[2], &argv[3], argc - 3);
        }
    } else if (strcmp(argv[1], "-m") == 0) {
        print_metadata_from_archive(argv[2]);
    } else if (strcmp(argv[1], "-q") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s -q <archive-file> <file/dir> ...\n", argv[0]);
            return EXIT_FAILURE;
        }
        query_archive(argv[2], &argv[3], argc - 3);
    } else if (strcmp(argv[1], "-p") == 0) {
        print_hierarchy(argv[2]);
    } else if (strcmp(argv[1], "-d") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s -d <archive-file> <file/dir> ...\n", argv[0]);
            return EXIT_FAILURE;
        }
        delete_entities(argv[2], &argv[3], argc - 3);
    } else {
        fprintf(stderr, "Usage: %s {-c|-a|-x|-m|-d|-p|-j} <archive-file> [files/dirs...]\nUsage of -j: %s {-c|-a} <archive-file> -j [files/dirs...]\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}