#include <stddef.h>
#include <stdint.h>
#include "mm.h"

/* Hardware text mode color constants. */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

// Simple string comparison function
int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Helper function to write hexadecimal numbers
void terminal_write_hex(uint32_t value) {
    char hex_digits[] = "0123456789ABCDEF";
    char buffer[9]; // 8 hex digits + null terminator
    buffer[8] = '\0';
    
    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex_digits[value & 0xF];
        value >>= 4;
    }
    
    terminal_writestring(buffer);
}

// Helper function to write decimal numbers
void terminal_write_dec(uint32_t value) {
    if (value == 0) {
        terminal_putchar('0');
        return;
    }
    
    char buffer[11]; // Max 10 digits for 32-bit + null terminator
    int pos = 0;
    
    while (value > 0) {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Print in reverse order
    for (int i = pos - 1; i >= 0; i--) {
        terminal_putchar(buffer[i]);
    }
}

// Simple command processor for memory management
void process_command(const char* command) {
    if (strcmp(command, "memstat") == 0) {
        mm_print_stats();
    } else if (strcmp(command, "memmap") == 0) {
        mm_print_memory_map();
    } else if (strcmp(command, "heapdbg") == 0) {
        mm_debug_heap();
    } else if (strcmp(command, "memtest") == 0) {
        // Perform a simple memory allocation test
        terminal_writestring("=== Memory Test ===\n");
        
        void* ptr1 = kmalloc(100);
        terminal_writestring("Allocated 100 bytes at: 0x");
        terminal_write_hex((uint32_t)ptr1);
        terminal_writestring("\n");
        
        void* ptr2 = kmalloc(200);
        terminal_writestring("Allocated 200 bytes at: 0x");
        terminal_write_hex((uint32_t)ptr2);
        terminal_writestring("\n");
        
        void* ptr3 = kcalloc(50, sizeof(int));
        terminal_writestring("Allocated 50 ints (zeroed) at: 0x");
        terminal_write_hex((uint32_t)ptr3);
        terminal_writestring("\n");
        
        kfree(ptr1);
        terminal_writestring("Freed first allocation\n");
        
        kfree(ptr2);
        terminal_writestring("Freed second allocation\n");
        
        kfree(ptr3);
        terminal_writestring("Freed third allocation\n");
        
        terminal_writestring("Memory test complete!\n");
    } else if (strcmp(command, "help") == 0) {
        terminal_writestring("Available commands:\n");
        terminal_writestring("  memstat  - Show memory statistics\n");
        terminal_writestring("  memmap   - Show memory map\n");
        terminal_writestring("  heapdbg  - Debug heap structure\n");
        terminal_writestring("  memtest  - Run memory allocation test\n");
        terminal_writestring("  help     - Show this help\n");
    } else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(command);
        terminal_writestring("\nType 'help' for available commands.\n");
    }
}

void kernel_main(void) {
    /* Initialize terminal interface */
    terminal_initialize();

    /* Display welcome message */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Welcome to MiniCore-OS!\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("Kernel successfully loaded and running in protected mode.\n");
    
    /* Initialize memory management */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    terminal_writestring("Initializing memory management...\n");
    mm_init(NULL, 0); // Initialize with default heap
    terminal_writestring("Memory management initialized!\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    terminal_writestring("Bootloader Phase 1 Complete!\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
    terminal_writestring("Phase 2: Memory Management Active!\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("\nSystem Information:\n");
    terminal_writestring("- Architecture: x86 (32-bit)\n");
    terminal_writestring("- Mode: Protected Mode\n");
    terminal_writestring("- Memory Management: Active (Heap + Free-list)\n");
    terminal_writestring("- Heap Size: 1MB (0x200000 - 0x300000)\n");
    terminal_writestring("- Display: VGA Text Mode (80x25)\n");
    terminal_writestring("- Paging: Simulated structures\n");
    
    /* Demonstrate memory management */
    terminal_writestring("\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("=== Memory Management Demo ===\n");
    
    // Show initial memory statistics
    process_command("memstat");
    terminal_writestring("\n");
    
    // Run a memory test
    process_command("memtest");
    terminal_writestring("\n");
    
    // Show memory statistics after test
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("Memory statistics after test:\n");
    process_command("memstat");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    terminal_writestring("\nAvailable commands: memstat, memmap, heapdbg, memtest, help\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("Phase 2 demonstration complete!\n");
    
    /* Kernel main loop - for now, just halt */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
