#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

// File system constants
#define FS_MAX_FILES 16
#define FS_MAX_FILENAME 32
#define FS_MAX_FILESIZE 4096
#define FS_MAGIC 0x4D494E49  // "MINI" magic number

// File types
typedef enum {
    FS_FILE_TYPE_TEXT = 0,
    FS_FILE_TYPE_BINARY = 1
} fs_file_type_t;

// File entry structure
typedef struct {
    char name[FS_MAX_FILENAME];
    uint32_t size;
    fs_file_type_t type;
    uint8_t* data;
    uint32_t permissions;  // Simple read-only flag
} fs_file_t;

// File system structure
typedef struct {
    uint32_t magic;
    uint32_t file_count;
    fs_file_t files[FS_MAX_FILES];
    uint8_t file_data[FS_MAX_FILES * FS_MAX_FILESIZE];  // Static storage
} fs_t;

// File system functions
void fs_init(void);
int fs_list(void);
int fs_read(const char* filename, uint8_t** data, uint32_t* size);
fs_file_t* fs_find_file(const char* filename);
int fs_file_exists(const char* filename);
void fs_print_file_info(const fs_file_t* file);

// Demo file creation (for preloading files)
void fs_create_demo_files(void);
int fs_add_file(const char* name, const char* content, fs_file_type_t type);

#endif // FS_H
