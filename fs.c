#include "fs.h"

// External terminal functions from kernel.c
extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void terminal_setcolor(uint8_t color);
extern uint8_t vga_entry_color(int fg, int bg);
extern void terminal_write_dec(uint32_t value);

// External memory functions
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

// VGA colors
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_WHITE 15
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_BROWN 14

// Global file system instance
static fs_t filesystem;
static int fs_initialized = 0;

// String utility functions
static size_t fs_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static int fs_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

static void fs_strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// Initialize the file system
void fs_init(void) {
    if (fs_initialized) {
        return;
    }
    
    // Initialize file system structure
    filesystem.magic = FS_MAGIC;
    filesystem.file_count = 0;
    
    // Clear all file entries
    for (int i = 0; i < FS_MAX_FILES; i++) {
        filesystem.files[i].name[0] = '\0';
        filesystem.files[i].size = 0;
        filesystem.files[i].type = FS_FILE_TYPE_TEXT;
        filesystem.files[i].data = NULL;
        filesystem.files[i].permissions = 0; // Read-only
    }
    
    // Clear file data storage
    for (int i = 0; i < FS_MAX_FILES * FS_MAX_FILESIZE; i++) {
        filesystem.file_data[i] = 0;
    }
    
    fs_initialized = 1;
    
    // Create demo files
    fs_create_demo_files();
}

// Add a file to the file system
int fs_add_file(const char* name, const char* content, fs_file_type_t type) {
    if (!fs_initialized || filesystem.file_count >= FS_MAX_FILES) {
        return -1; // File system not initialized or full
    }
    
    size_t name_len = fs_strlen(name);
    size_t content_len = fs_strlen(content);
    
    if (name_len >= FS_MAX_FILENAME || content_len >= FS_MAX_FILESIZE) {
        return -2; // Name or content too long
    }
    
    // Check if file already exists
    if (fs_find_file(name) != NULL) {
        return -3; // File already exists
    }
    
    // Find the next available file slot
    int file_index = filesystem.file_count;
    fs_file_t* file = &filesystem.files[file_index];
    
    // Set up file metadata
    fs_strcpy(file->name, name);
    file->size = content_len;
    file->type = type;
    file->permissions = 0; // Read-only
    
    // Calculate data offset
    file->data = &filesystem.file_data[file_index * FS_MAX_FILESIZE];
    
    // Copy content to file data area
    for (size_t i = 0; i < content_len; i++) {
        file->data[i] = content[i];
    }
    
    filesystem.file_count++;
    return 0; // Success
}

// Find a file by name
fs_file_t* fs_find_file(const char* filename) {
    if (!fs_initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < filesystem.file_count; i++) {
        if (fs_strcmp(filesystem.files[i].name, filename) == 0) {
            return &filesystem.files[i];
        }
    }
    
    return NULL; // File not found
}

// Check if a file exists
int fs_file_exists(const char* filename) {
    return fs_find_file(filename) != NULL;
}

// List all files in the file system
int fs_list(void) {
    if (!fs_initialized) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("File system not initialized!\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        return -1;
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("=== File System Contents ===\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    if (filesystem.file_count == 0) {
        terminal_writestring("No files found.\n");
        return 0;
    }
    
    terminal_writestring("Name                     Size   Type\n");
    terminal_writestring("------------------------ ------ --------\n");
    
    for (uint32_t i = 0; i < filesystem.file_count; i++) {
        fs_file_t* file = &filesystem.files[i];
        
        // Print filename (padded to 24 chars)
        terminal_writestring(file->name);
        int name_len = fs_strlen(file->name);
        for (int j = name_len; j < 24; j++) {
            terminal_putchar(' ');
        }
        
        // Print size
        terminal_putchar(' ');
        terminal_write_dec(file->size);
        
        // Print padding for size (assume max 6 digits)
        int size_digits = 1;
        uint32_t temp_size = file->size;
        while (temp_size >= 10) {
            size_digits++;
            temp_size /= 10;
        }
        for (int j = size_digits; j < 6; j++) {
            terminal_putchar(' ');
        }
        
        // Print type
        terminal_putchar(' ');
        if (file->type == FS_FILE_TYPE_TEXT) {
            terminal_writestring("TEXT");
        } else {
            terminal_writestring("BINARY");
        }
        
        terminal_putchar('\n');
    }
    
    terminal_writestring("\nTotal files: ");
    terminal_write_dec(filesystem.file_count);
    terminal_writestring(" / ");
    terminal_write_dec(FS_MAX_FILES);
    terminal_putchar('\n');
    
    return 0;
}

// Read a file's content
int fs_read(const char* filename, uint8_t** data, uint32_t* size) {
    fs_file_t* file = fs_find_file(filename);
    
    if (file == NULL) {
        return -1; // File not found
    }
    
    *data = file->data;
    *size = file->size;
    
    return 0; // Success
}

// Print detailed file information
void fs_print_file_info(const fs_file_t* file) {
    if (file == NULL) {
        terminal_writestring("File is NULL\n");
        return;
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("=== File Information ===\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    terminal_writestring("Name: ");
    terminal_writestring(file->name);
    terminal_putchar('\n');
    
    terminal_writestring("Size: ");
    terminal_write_dec(file->size);
    terminal_writestring(" bytes\n");
    
    terminal_writestring("Type: ");
    if (file->type == FS_FILE_TYPE_TEXT) {
        terminal_writestring("TEXT\n");
    } else {
        terminal_writestring("BINARY\n");
    }
    
    terminal_writestring("Permissions: READ-ONLY\n");
}

// Create demo files for testing
void fs_create_demo_files(void) {
    // Welcome file
    fs_add_file("welcome.txt", 
                "Welcome to MiniCore-OS!\n"
                "This is a simple read-only file system.\n"
                "Try 'ls' to list files and 'cat <filename>' to read them.\n"
                "\n"
                "Available commands:\n"
                "- help: Show all commands\n"
                "- ls: List files\n"
                "- cat <file>: Display file contents\n"
                "- clear: Clear screen\n"
                "- mem: Memory information\n"
                "- version: System version\n", 
                FS_FILE_TYPE_TEXT);
    
    // System info file
    fs_add_file("system.txt",
                "MiniCore-OS System Information\n"
                "=============================\n"
                "Architecture: x86 (32-bit)\n"
                "Mode: Protected Mode\n"
                "Memory Management: Active\n"
                "File System: Read-only in-memory\n"
                "Multitasking: Cooperative\n"
                "VGA Text Mode: 80x25\n"
                "Build Date: August 2025\n",
                FS_FILE_TYPE_TEXT);
    
    // README file
    fs_add_file("readme.txt",
                "MiniCore-OS Phase 5: File System\n"
                "=================================\n"
                "\n"
                "This file system implementation provides:\n"
                "- Read-only access to preloaded files\n"
                "- Fixed-size file allocation\n"
                "- Directory-like abstraction\n"
                "- Shell integration with 'ls' and 'cat'\n"
                "\n"
                "Files are stored in memory and preloaded at boot.\n"
                "Maximum file size: 4KB\n"
                "Maximum files: 16\n",
                FS_FILE_TYPE_TEXT);
    
    // Demo code file
    fs_add_file("hello.c",
                "#include <stdio.h>\n"
                "\n"
                "int main(void) {\n"
                "    printf(\"Hello from MiniCore-OS!\\n\");\n"
                "    return 0;\n"
                "}\n",
                FS_FILE_TYPE_TEXT);
    
    // License file
    fs_add_file("license.txt",
                "MiniCore-OS License\n"
                "==================\n"
                "\n"
                "This is a demonstration operating system.\n"
                "Created for educational purposes.\n"
                "\n"
                "Feel free to study, modify, and learn from this code.\n",
                FS_FILE_TYPE_TEXT);
    
    // Files created silently - use 'ls' command to see them
}
