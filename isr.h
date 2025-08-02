#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// Register structure passed to ISR
struct registers {
    uint32_t ds;                    // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;      // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by processor automatically
};

// IRQ handler function pointer type
typedef void (*isr_t)(struct registers*);

// Functions
void isr_init(void);
void isr_handler(struct registers* r);
void irq_handler(struct registers* r);
void register_interrupt_handler(uint8_t n, isr_t handler);

// Send End Of Interrupt to PIC
void irq_ack(uint8_t irq);

// Enable/disable specific IRQs
void irq_enable(uint8_t irq);
void irq_disable(uint8_t irq);

#endif
