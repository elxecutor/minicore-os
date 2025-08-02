#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "isr.h"

#define MAX_TASKS 8
#define TASK_STACK_SIZE 4096

// Task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_TERMINATED
} task_state_t;

// Task control block
typedef struct task {
    uint32_t id;
    char name[32];
    task_state_t state;
    
    // CPU state
    uint32_t esp;           // Stack pointer
    uint32_t ebp;           // Base pointer
    uint32_t ebx, esi, edi; // General purpose registers
    uint32_t eflags;        // Flags register
    uint32_t eip;           // Instruction pointer
    
    // Stack
    uint8_t stack[TASK_STACK_SIZE];
    
    // Scheduling info
    uint32_t time_slice;
    uint32_t time_remaining;
    uint32_t sleep_until;
    
    struct task* next;      // For task queue
} task_t;

// Scheduler functions
void scheduler_init(void);
uint32_t task_create(const char* name, void (*entry_point)(void));
void task_yield(void);
void task_sleep(uint32_t ticks);
void task_exit(void);
void scheduler_tick(struct registers* r);
void schedule(void);

// Task switching (implemented in assembly)
extern void task_switch(uint32_t* old_esp, uint32_t new_esp);

// Current task info
extern task_t* current_task;
extern uint32_t next_task_id;

// Demo tasks
void task_idle(void);
void task_counter(void);
void task_greeter(void);

#endif
