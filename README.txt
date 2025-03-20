# Myz Archiver
**Myz Archiver** is a modular archiving utility/system program similar in functionality to common archiving tools like `tar` or `zip`, but with custom behavior. It allows users to create, extract, append, delete, and query archives containing files, directories, as well as symbolic and hard links. Additionally, it supports optional compression (using `gzip`) during archive creation or append (enabled with the `-j` flag), ensuring that files are restored in their original, uncompressed form upon extraction.

## Features

- Create, extract, append, delete, and query archives.
- Compression support with `gzip`.
- Support for hard links and symbolic links.
- Low-level metadata handling for files in the archive.
- Modular design to keep the code clean and maintainable.

## Archive File Structure

The archive file consists of three main sections:

- **Header**: A fixed-size header (256 bytes) that contains:
  - The total number of metadata entries.
  - The offset in the archive where the metadata block begins.
  - Reserved bytes for future use.

- **File Data Block**: The concatenated binary data of all archived files (only for regular files that store data).

- **Metadata Block**: A sequence of metadata entries (one for each archived entity) that describes each entity’s properties (path, mode, owner, group, timestamps, size, data offset, inode, hardlink flag, and, for symlinks, the link target).

## Project Structure and Modular Design

The project is organized to keep the code clean, maintainable, and easy to extend. The code is divided into modules for different functionality, such as flag handling, utility functions, and data structures:

### Common Modules:

- `structs.h`: Contains definitions for all core data structures such as `FileMetadata`, `ArchiveHeader`, and `MetadataArray`.
- `utils.h` / `utils.c`: Provides utility functions used across the project.

### Flag-Specific Modules:

- `c_flag/`: Implements the `-c` flag for archive creation.
- `x_flag/`: Implements the `-x` flag for extraction.
- `a_flag/`: Implements the `-a` flag for appending new entities to an existing archive.
- `d_flag/`: Implements the `-d` flag for deleting specific entities from an archive.
- `m_flag/`: Implements the `-m` flag for printing metadata.
- `q_flag/`: Implements the `-q` flag for querying the existence of specific files or directories in the archive.
- `p_flag/`: Implements the `-p` flag for printing the archive’s hierarchy in a tree-like format.

### Main Module:

- `myz.c`: Contains the `main()` function, which dispatches execution based on the provided command-line flag.

## Key Implementation Details

### 1. Single `process_path` Function

All archive creation and append operations use a single function `process_path()` (implemented in `utils.c`) that:

- Recursively traverses directories.
- For regular files:
  - If `-j` (compression) is enabled, it compresses file data by forking and executing `gzip -c <file>` via a pipe.
  - Otherwise, it reads file data normally.
- For symbolic links: It uses `readlink()` to obtain the target and stores it in the metadata.
- For hard links: It checks (via inode comparisons) if a file with the same inode has already been archived. If so, the new entry is marked as a hard link and shares the same data offset as the original entry.
- The metadata for each entry (file, directory, symlink) is stored in a dynamically managed array (`MetadataArray`).

### 2. Compression with `-j`

When the `-j` flag is active, the global variable `compress_flag` is set. The function `process_path()` checks if `compress_flag` is true, and if the current entity is a regular file, the file is compressed before writing its data into the archive. The compression is implemented using a helper function `compress_file_to_archive()`.

### 3. Extraction Process (`-x`)

The extraction function reads the header and metadata from the archive, recreating the directory structure and handling regular files, hard links, and symbolic links.

### 4. Append (`-a`) and Delete (`-d`) Operations

- **Append (`-a`)**: Reads the existing archive and adds new entries if they do not already exist.
- **Delete (`-d`)**: Reads the existing archive and filters out the metadata entries corresponding to files or directories specified for deletion. A new archive is created, and the original archive is replaced.

## Build System

A sample `Makefile` is provided to compile the project. Object files are placed into a separate folder (e.g., `build/`) to keep the source directory clean. You can compile the project with:

```bash
make
```

To clean the build directory:

```bash
make clean
```

## Installation

To build the project, clone the repository and run the `make` command:

```bash
git clone https://github.com/your-username/myz-archiver.git
cd myz-archiver
make
```

## Usage

To use the `myz` archiver, use the following command-line options:

- `-c`: Create a new archive.
- `-x`: Extract files from an archive.
- `-a`: Append files to an existing archive.
- `-j`: Compress files during archive creation or append.
- `-d`: Delete files from an archive.
- `-m`: Print metadata of an archive.
- `-q`: Query the existence of files in an archive.
- `-p`: Print the archive’s hierarchy in a tree-like format.

Example usage:

```bash
./myz -c archive.myz file1.txt file2.txt DIR1 DIR2 DIR3
./myz -x archive.myz 
./myz -a archive.myz -j file1.txt DIR1
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
```
