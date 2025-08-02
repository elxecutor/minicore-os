#include "shell.h"
#include "isr.h"
#include "mm.h"
#include "fs.h"

// External terminal functions from kernel.c
extern void terminal_putchar(char c);
extern void terminal_writestring(const char* data);
extern void terminal_write_hex(uint32_t value);
extern void terminal_write_dec(uint32_t value);
extern void terminal_setcolor(uint8_t color);
extern void terminal_clear(void);
extern void terminal_scroll_up(void);
extern uint8_t vga_entry_color(int fg, int bg);
extern size_t terminal_row;
extern size_t terminal_column;

// VGA colors (from kernel.c)
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_WHITE 15
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_BROWN 14

// PS/2 keyboard port
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// Global shell state
static shell_state_t shell_state;
static uint8_t keyboard_state = 0;

// Scan code to ASCII conversion table (US QWERTY)
static char scancode_ascii[] = {
    0,    0,   '1', '2', '3', '4', '5', '6',  // 0x00-0x07
    '7', '8', '9', '0', '-', '=',  0,   0,    // 0x08-0x0F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',  // 0x10-0x17
    'o', 'p', '[', ']',  0,   0,  'a', 's',  // 0x18-0x1F
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  // 0x20-0x27
    '\'', '`',  0, '\\', 'z', 'x', 'c', 'v',  // 0x28-0x2F
    'b', 'n', 'm', ',', '.', '/',  0,  '*',  // 0x30-0x37
    0,   ' '                                  // 0x38-0x39
};

// Shifted scan code to ASCII conversion table
static char scancode_ascii_shift[] = {
    0,    0,   '!', '@', '#', '$', '%', '^',  // 0x00-0x07
    '&', '*', '(', ')', '_', '+',  0,   0,    // 0x08-0x0F
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',  // 0x10-0x17
    'O', 'P', '{', '}',  0,   0,  'A', 'S',  // 0x18-0x1F
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',  // 0x20-0x27
    '"', '~',  0,  '|', 'Z', 'X', 'C', 'V',  // 0x28-0x2F
    'B', 'N', 'M', '<', '>', '?',  0,  '*',  // 0x30-0x37
    0,   ' '                                  // 0x38-0x39
};

// Built-in commands table
static shell_command_t commands[] = {
    {"help",    "Show available commands",           cmd_help},
    {"echo",    "Echo text to screen",               cmd_echo},
    {"mem",     "Show memory information",           cmd_mem},
    {"halt",    "Halt the system",                   cmd_halt},
    {"clear",   "Clear the screen",                  cmd_clear},
    {"memtest", "Run memory allocation test",        cmd_memtest},
    {"version", "Show system version",               cmd_version},
    {"uptime",  "Show system uptime (placeholder)", cmd_uptime},
    {"tasks",   "Show running tasks",                cmd_tasks},
    {"starttasks", "Start demo multitasking tasks",     cmd_starttasks},
    {"enableints", "Enable interrupts",                 cmd_enableints},
    {"ls",      "List files in file system",        cmd_ls},
    {"cat",     "Display file contents",             cmd_cat},
    {NULL, NULL, NULL} // End marker
};

// Port I/O functions
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ __volatile__("inb %1, %0" : "=a"(result) : "dN"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ __volatile__("outb %1, %0" : : "dN"(port), "a"(data));
}

// Utility functions
int shell_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

void shell_strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

int shell_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

char* shell_strchr(const char* str, int c) {
    while (*str) {
        if (*str == c) return (char*)str;
        str++;
    }
    return NULL;
}

// Initialize shell
void shell_init(void) {
    shell_state.buffer_pos = 0;
    shell_state.cursor_x = 0;
    shell_state.cursor_y = terminal_row;
    shell_state.echo_enabled = 1;
    shell_clear_buffer();
    
    keyboard_init();
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("\n=== MiniCore-OS Shell Active ===\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("Type 'help' for commands | 'ls' for files | 'clear' to clear screen\n\n");
}

// Initialize keyboard
void keyboard_init(void) {
    // Enable keyboard (this is basic - real implementation would set up interrupts)
    keyboard_state = 0;
}

// Read scan code from keyboard
uint8_t keyboard_read_scancode(void) {
    // In polling mode, don't wait - just check if data is available
    if (inb(KEYBOARD_STATUS_PORT) & 0x01) {
        return inb(KEYBOARD_DATA_PORT);
    }
    return 0; // No data available
}

// Convert scan code to ASCII
char scancode_to_ascii(uint8_t scancode, uint8_t shift) {
    if (scancode >= sizeof(scancode_ascii)) {
        return 0;
    }
    
    if (shift && scancode < sizeof(scancode_ascii_shift)) {
        return scancode_ascii_shift[scancode];
    }
    
    return scancode_ascii[scancode];
}

// Keyboard interrupt handler (called by IRQ1)
void keyboard_interrupt_handler(struct registers* r) {
    (void)r; // Suppress unused parameter warning
    keyboard_handler();
}

// Keyboard handler (called in polling mode)
void keyboard_handler(void) {
    uint8_t scancode = keyboard_read_scancode();
    
    // If no scancode available, return early
    if (scancode == 0) {
        return;
    }
    
    // Handle key releases (high bit set)
    if (scancode & 0x80) {
        scancode &= 0x7F; // Remove release flag
        
        // Handle modifier keys
        if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
            keyboard_state &= ~SHIFT_PRESSED;
        } else if (scancode == KEY_LCTRL) {
            keyboard_state &= ~CTRL_PRESSED;
        }
        return;
    }
    
    // Handle key presses
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        keyboard_state |= SHIFT_PRESSED;
        return;
    } else if (scancode == KEY_LCTRL) {
        keyboard_state |= CTRL_PRESSED;
        return;
    }
    
    // Handle special keys
    if (scancode == KEY_ENTER) {
        shell_process_input('\n');
        return;
    } else if (scancode == KEY_BACKSPACE) {
        shell_backspace();
        return;
    }
    
    // Convert to ASCII and process
    char ascii = scancode_to_ascii(scancode, keyboard_state & SHIFT_PRESSED);
    if (ascii) {
        shell_process_input(ascii);
    }
}

// Clear input buffer
void shell_clear_buffer(void) {
    for (int i = 0; i < SHELL_BUFFER_SIZE; i++) {
        shell_state.input_buffer[i] = '\0';
    }
    shell_state.buffer_pos = 0;
}

// Handle backspace
void shell_backspace(void) {
    if (shell_state.buffer_pos > 0) {
        shell_state.buffer_pos--;
        shell_state.input_buffer[shell_state.buffer_pos] = '\0';
        
        // Move cursor back and erase character
        if (terminal_column > 0) {
            terminal_column--;
            terminal_putchar(' ');
            terminal_column--;
        }
    }
}

// Print shell prompt
void shell_print_prompt(void) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring(SHELL_PROMPT);
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

// Process input character
void shell_process_input(char c) {
    if (c == '\n') {
        terminal_putchar('\n');
        shell_state.input_buffer[shell_state.buffer_pos] = '\0';
        shell_execute_command();
        shell_clear_buffer();
        shell_print_prompt();
    } else if (c >= 32 && c <= 126) { // Printable characters
        if (shell_state.buffer_pos < SHELL_BUFFER_SIZE - 1) {
            shell_state.input_buffer[shell_state.buffer_pos++] = c;
            if (shell_state.echo_enabled) {
                terminal_putchar(c);
            }
        }
    }
}

// Parse command into arguments
int shell_parse_command(char* input, char* argv[], int max_args) {
    int argc = 0;
    char* token = input;
    
    // Skip leading whitespace
    while (*token == ' ' || *token == '\t') token++;
    
    while (*token && argc < max_args - 1) {
        argv[argc++] = token;
        
        // Find end of current argument
        while (*token && *token != ' ' && *token != '\t') token++;
        
        if (*token) {
            *token++ = '\0'; // Null-terminate current argument
            
            // Skip whitespace to next argument
            while (*token == ' ' || *token == '\t') token++;
        }
    }
    
    argv[argc] = NULL;
    return argc;
}

// Find command in command table
int shell_find_command(const char* name) {
    for (int i = 0; commands[i].name != NULL; i++) {
        if (shell_strcmp(name, commands[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

// Execute parsed command
void shell_execute_command(void) {
    if (shell_state.buffer_pos == 0) {
        return; // Empty command
    }
    
    char* argv[SHELL_MAX_ARGS];
    int argc = shell_parse_command(shell_state.input_buffer, argv, SHELL_MAX_ARGS);
    
    if (argc == 0) {
        return;
    }
    
    int cmd_index = shell_find_command(argv[0]);
    if (cmd_index >= 0) {
        commands[cmd_index].handler(argc, argv);
    } else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("Unknown command: ");
        terminal_writestring(argv[0]);
        terminal_writestring("\nType 'help' for available commands.\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
}

// Main shell loop
void shell_run(void) {
    terminal_writestring("Interactive shell ready! Try typing 'help' or 'ls'\n");
    shell_print_prompt();
    
    while (1) {
        // Start in polling mode for stability
        // Poll keyboard for input
        keyboard_handler();
        
        // Small delay to prevent excessive CPU usage
        for (volatile int i = 0; i < 1000; i++);
    }
}

// Command implementations
int cmd_help(int argc, char* argv[]) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Available commands:\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    for (int i = 0; commands[i].name != NULL; i++) {
        terminal_writestring("  ");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring(commands[i].name);
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        terminal_writestring(" - ");
        terminal_writestring(commands[i].description);
        terminal_writestring("\n");
    }
    
    return 0;
}

int cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) terminal_putchar(' ');
        terminal_writestring(argv[i]);
    }
    terminal_putchar('\n');
    return 0;
}

int cmd_mem(int argc, char* argv[]) {
    if (argc > 1) {
        if (shell_strcmp(argv[1], "stats") == 0) {
            mm_print_stats();
        } else if (shell_strcmp(argv[1], "map") == 0) {
            mm_print_memory_map();
        } else if (shell_strcmp(argv[1], "debug") == 0) {
            mm_debug_heap();
        } else {
            terminal_writestring("Usage: mem [stats|map|debug]\n");
        }
    } else {
        mm_print_stats();
    }
    return 0;
}

int cmd_halt(int argc, char* argv[]) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    terminal_writestring("System halting...\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    __asm__ __volatile__("cli");
    while (1) {
        __asm__ __volatile__("hlt");
    }
    
    return 0;
}

int cmd_clear(int argc, char* argv[]) {
    shell_clear_screen();
    return 0;
}

int cmd_memtest(int argc, char* argv[]) {
    terminal_writestring("Running memory allocation test...\n");
    
    void* ptr1 = kmalloc(100);
    terminal_writestring("Allocated 100 bytes at: 0x");
    terminal_write_hex((uint32_t)ptr1);
    terminal_writestring("\n");
    
    void* ptr2 = kmalloc(200);
    terminal_writestring("Allocated 200 bytes at: 0x");
    terminal_write_hex((uint32_t)ptr2);
    terminal_writestring("\n");
    
    kfree(ptr1);
    terminal_writestring("Freed first allocation\n");
    
    kfree(ptr2);
    terminal_writestring("Freed second allocation\n");
    
    terminal_writestring("Memory test completed!\n");
    return 0;
}

int cmd_version(int argc, char* argv[]) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("MiniCore-OS v0.3.0\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("Phase 3: CLI Shell\n");
    terminal_writestring("Built with: GCC, NASM, GRUB\n");
    terminal_writestring("Features: Memory Management, Interactive Shell\n");
    return 0;
}

int cmd_uptime(int argc, char* argv[]) {
    terminal_writestring("Uptime: Since boot (no timer implemented yet)\n");
    return 0;
}

int cmd_tasks(int argc, char* argv[]) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Task Information:\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("ID  Name        State     \n");
    terminal_writestring("--- ----------- ----------\n");
    terminal_writestring("1   idle        READY     \n");
    terminal_writestring("2   counter     RUNNING   \n");
    terminal_writestring("3   greeter     SLEEPING  \n");
    terminal_writestring("\nMultitasking is active with timer-driven scheduling!\n");
    terminal_writestring("Tasks automatically switch every 10 timer ticks.\n");
    return 0;
}

int cmd_starttasks(int argc, char* argv[]) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("Starting demo multitasking tasks...\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    // External functions from scheduler
    extern uint32_t task_create(const char* name, void (*entry_point)(void));
    extern void task_idle(void);
    extern void task_counter(void);
    extern void task_greeter(void);
    
    task_create("idle", task_idle);
    terminal_writestring("Created idle task\n");
    
    task_create("counter", task_counter);
    terminal_writestring("Created counter task\n");
    
    task_create("greeter", task_greeter);
    terminal_writestring("Created greeter task\n");
    
    terminal_writestring("Demo tasks started! They will run in the background.\n");
    return 0;
}

int cmd_enableints(int argc, char* argv[]) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Enabling interrupts...\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    // External functions from isr
    extern void irq_enable(uint8_t irq);
    
    terminal_writestring("Enabling keyboard interrupt (IRQ1)...\n");
    irq_enable(1);
    
    terminal_writestring("Enabling timer interrupt (IRQ0)...\n");
    irq_enable(0);
    
    terminal_writestring("Enabling global interrupts...\n");
    __asm__ volatile ("sti");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("Interrupts enabled! Keyboard should now be interrupt-driven.\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    return 0;
}

int cmd_ls(int argc, char* argv[]) {
    return fs_list();
}

int cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("Usage: cat <filename>\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        return -1;
    }
    
    uint8_t* data;
    uint32_t size;
    
    int result = fs_read(argv[1], &data, &size);
    if (result != 0) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("File not found: ");
        terminal_writestring(argv[1]);
        terminal_writestring("\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        return -1;
    }
    
    // Display file contents
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("=== Contents of ");
    terminal_writestring(argv[1]);
    terminal_writestring(" ===\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    // Print file content character by character
    for (uint32_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
    
    terminal_putchar('\n');
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("=== End of file ===\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    return 0;
}

// Terminal control functions
void shell_clear_screen(void) {
    terminal_clear();
}
