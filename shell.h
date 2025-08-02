#ifndef SHELL_H
#define SHELL_H

#include <stddef.h>
#include <stdint.h>

// Shell constants
#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "minicore> "

// Command structure
typedef struct shell_command {
    const char* name;
    const char* description;
    int (*handler)(int argc, char* argv[]);
} shell_command_t;

// Shell state
typedef struct shell_state {
    char input_buffer[SHELL_BUFFER_SIZE];
    size_t buffer_pos;
    int cursor_x;
    int cursor_y;
    int echo_enabled;
} shell_state_t;

// Keyboard scan codes (US QWERTY layout)
#define KEY_ESC         0x01
#define KEY_1           0x02
#define KEY_2           0x03
#define KEY_3           0x04
#define KEY_4           0x05
#define KEY_5           0x06
#define KEY_6           0x07
#define KEY_7           0x08
#define KEY_8           0x09
#define KEY_9           0x0A
#define KEY_0           0x0B
#define KEY_MINUS       0x0C
#define KEY_EQUALS      0x0D
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F
#define KEY_Q           0x10
#define KEY_W           0x11
#define KEY_E           0x12
#define KEY_R           0x13
#define KEY_T           0x14
#define KEY_Y           0x15
#define KEY_U           0x16
#define KEY_I           0x17
#define KEY_O           0x18
#define KEY_P           0x19
#define KEY_LBRACKET    0x1A
#define KEY_RBRACKET    0x1B
#define KEY_ENTER       0x1C
#define KEY_LCTRL       0x1D
#define KEY_A           0x1E
#define KEY_S           0x1F
#define KEY_D           0x20
#define KEY_F           0x21
#define KEY_G           0x22
#define KEY_H           0x23
#define KEY_J           0x24
#define KEY_K           0x25
#define KEY_L           0x26
#define KEY_SEMICOLON   0x27
#define KEY_QUOTE       0x28
#define KEY_BACKTICK    0x29
#define KEY_LSHIFT      0x2A
#define KEY_BACKSLASH   0x2B
#define KEY_Z           0x2C
#define KEY_X           0x2D
#define KEY_C           0x2E
#define KEY_V           0x2F
#define KEY_B           0x30
#define KEY_N           0x31
#define KEY_M           0x32
#define KEY_COMMA       0x33
#define KEY_PERIOD      0x34
#define KEY_SLASH       0x35
#define KEY_RSHIFT      0x36
#define KEY_SPACE       0x39

// Keyboard state flags
#define SHIFT_PRESSED   0x01
#define CTRL_PRESSED    0x02
#define ALT_PRESSED     0x04

// Shell initialization and main functions
void shell_init(void);
void shell_run(void);
void shell_print_prompt(void);
void shell_process_input(char c);
void shell_execute_command(void);

// Keyboard handling
void keyboard_init(void);
void keyboard_handler(void);
uint8_t keyboard_read_scancode(void);
char scancode_to_ascii(uint8_t scancode, uint8_t shift);

// Command parsing
int shell_parse_command(char* input, char* argv[], int max_args);
int shell_find_command(const char* name);

// Built-in command handlers
int cmd_help(int argc, char* argv[]);
int cmd_echo(int argc, char* argv[]);
int cmd_mem(int argc, char* argv[]);
int cmd_halt(int argc, char* argv[]);
int cmd_clear(int argc, char* argv[]);
int cmd_memtest(int argc, char* argv[]);
int cmd_version(int argc, char* argv[]);
int cmd_uptime(int argc, char* argv[]);
int cmd_tasks(int argc, char* argv[]);
int cmd_starttasks(int argc, char* argv[]);
int cmd_enableints(int argc, char* argv[]);
int cmd_ls(int argc, char* argv[]);
int cmd_cat(int argc, char* argv[]);

// Utility functions
void shell_clear_buffer(void);
void shell_backspace(void);
int shell_strlen(const char* str);
void shell_strcpy(char* dest, const char* src);
int shell_strcmp(const char* str1, const char* str2);
char* shell_strchr(const char* str, int c);

// Terminal control functions
void shell_clear_screen(void);
void shell_move_cursor(int x, int y);
void shell_scroll_up(void);

// Interrupt handlers
struct registers; // Forward declaration
void keyboard_interrupt_handler(struct registers* r);

#endif // SHELL_H
