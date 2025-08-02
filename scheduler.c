#include <stddef.h>
#include "scheduler.h"
#include "isr.h"

// Task management
static task_t tasks[MAX_TASKS];
static task_t* task_queue_head = NULL;
static task_t* task_queue_tail = NULL;
task_t* current_task = NULL;
uint32_t next_task_id = 1;
static uint32_t system_ticks = 0;

// External functions
extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);
extern void terminal_putchar(char c);

// Helper functions
static void task_queue_add(task_t* task);
static task_t* task_queue_remove_next(void);
static void switch_to_task(task_t* task);

// VGA colors
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_YELLOW        14
#define VGA_COLOR_WHITE         15
#define VGA_COLOR_BLACK         0

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

// Initialize scheduler
void scheduler_init(void) {
    // Clear task table
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_TERMINATED;
        tasks[i].id = 0;
    }
    
    task_queue_head = NULL;
    task_queue_tail = NULL;
    current_task = NULL;
    system_ticks = 0;
    
    terminal_writestring("Scheduler initialized\n");
    
    // For stability, don't create demo tasks immediately
    // They can be created later once the system is fully stable
    terminal_writestring("Demo tasks disabled for stability\n");
}

// Create a new task
uint32_t task_create(const char* name, void (*entry_point)(void)) {
    // Find free task slot
    task_t* task = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_TERMINATED) {
            task = &tasks[i];
            break;
        }
    }
    
    if (!task) {
        return 0; // No free slots
    }
    
    // Initialize task
    task->id = next_task_id++;
    for (int i = 0; i < 32 && name[i]; i++) {
        task->name[i] = name[i];
    }
    task->name[31] = '\0';
    
    task->state = TASK_READY;
    task->time_slice = 10; // 10 timer ticks
    task->time_remaining = task->time_slice;
    task->sleep_until = 0;
    
    // Set up stack (grows downward)
    task->esp = (uint32_t)(task->stack + TASK_STACK_SIZE - 4);
    task->ebp = task->esp;
    task->eip = (uint32_t)entry_point;
    task->eflags = 0x202; // Enable interrupts
    
    // Initialize registers
    task->ebx = 0;
    task->esi = 0;
    task->edi = 0;
    
    // Add to ready queue
    task_queue_add(task);
    
    return task->id;
}

// Add task to ready queue
static void task_queue_add(task_t* task) {
    task->next = NULL;
    
    if (task_queue_tail) {
        task_queue_tail->next = task;
        task_queue_tail = task;
    } else {
        task_queue_head = task;
        task_queue_tail = task;
    }
}

// Remove next task from ready queue
static task_t* task_queue_remove_next(void) {
    if (!task_queue_head) {
        return NULL;
    }
    
    task_t* task = task_queue_head;
    task_queue_head = task->next;
    
    if (!task_queue_head) {
        task_queue_tail = NULL;
    }
    
    task->next = NULL;
    return task;
}

// Timer interrupt handler for scheduling
void scheduler_tick(struct registers* r) {
    (void)r; // Suppress unused parameter warning
    system_ticks++;
    
    // For now, just increment ticks - don't do complex scheduling
    // until we're sure the basic interrupt system is stable
    
    // Wake up sleeping tasks
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_SLEEPING && 
            tasks[i].sleep_until <= system_ticks) {
            tasks[i].state = TASK_READY;
            task_queue_add(&tasks[i]);
        }
    }
    
    // Only do scheduling if we have tasks and system is stable
    if (current_task && system_ticks > 100) { // Wait 100 ticks before scheduling
        current_task->time_remaining--;
        
        // Time slice expired?
        if (current_task->time_remaining == 0) {
            schedule();
        }
    }
}

// Perform task switch
void schedule(void) {
    task_t* next_task = task_queue_remove_next();
    
    if (!next_task) {
        return; // No tasks to run
    }
    
    // If we have a current task, save it
    if (current_task && current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
        current_task->time_remaining = current_task->time_slice;
        task_queue_add(current_task);
    }
    
    // Switch to next task
    switch_to_task(next_task);
}

// Switch to specific task
static void switch_to_task(task_t* task) {
    task->state = TASK_RUNNING;
    task->time_remaining = task->time_slice;
    
    uint32_t old_esp = 0;
    if (current_task) {
        old_esp = current_task->esp;
    }
    
    current_task = task;
    
    // For now, simulate task switching without actual assembly
    // In a real implementation, this would switch CPU state
    if (old_esp != 0) {
        // Simulate saving old task state and loading new task state
        // This is where you'd call the assembly task_switch function
    }
}

// Yield CPU to next task
void task_yield(void) {
    schedule();
}

// Put current task to sleep
void task_sleep(uint32_t ticks) {
    if (current_task) {
        current_task->state = TASK_SLEEPING;
        current_task->sleep_until = system_ticks + ticks;
        schedule();
    }
}

// Terminate current task
void task_exit(void) {
    if (current_task) {
        current_task->state = TASK_TERMINATED;
        current_task = NULL;
        schedule();
    }
}

// Demo task: Idle task (runs when nothing else is scheduled)
void task_idle(void) {
    while (1) {
        // Just wait
        __asm__ volatile ("hlt");
    }
}

// Demo task: Counter task
void task_counter(void) {
    static int counter = 0;
    
    while (1) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("[Counter: ");
        
        // Simple integer to string conversion
        char num_str[16];
        int num = counter++;
        int pos = 0;
        
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                num_str[pos++] = temp[i];
            }
        }
        num_str[pos] = '\0';
        
        terminal_writestring(num_str);
        terminal_writestring("] ");
        
        task_sleep(50); // Sleep for 50 ticks
    }
}

// Demo task: Greeter task
void task_greeter(void) {
    const char* greetings[] = {
        "Hello from multitasking!",
        "Tasks are running!",
        "Scheduler working!",
        "Context switching active!"
    };
    int greeting_index = 0;
    
    while (1) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
        terminal_writestring("[");
        terminal_writestring(greetings[greeting_index]);
        terminal_writestring("] ");
        
        greeting_index = (greeting_index + 1) % 4;
        task_sleep(75); // Sleep for 75 ticks
    }
}
