#include "isr.h"
#include "idt.h"

// I/O functions
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ISR handler table
static isr_t interrupt_handlers[256];

// Exception messages
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// External functions from kernel
extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);

// VGA colors
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_WHITE         15
#define VGA_COLOR_BLACK         0

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

// Initialize ISR system
void isr_init(void) {
    // Clear interrupt handlers
    for (int i = 0; i < 256; i++) {
        interrupt_handlers[i] = 0;
    }
}

// ISR handler for exceptions
void isr_handler(struct registers* r) {
    if (interrupt_handlers[r->int_no] != 0) {
        isr_t handler = interrupt_handlers[r->int_no];
        handler(r);
        return;
    }

    // Unhandled exception - display error
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    terminal_writestring("Exception: ");
    
    if (r->int_no < 32) {
        terminal_writestring(exception_messages[r->int_no]);
    } else {
        terminal_writestring("Unknown Exception");
    }
    
    terminal_writestring("\nSystem Halted.\n");
    
    // Halt system
    while (1) {
        __asm__ volatile ("hlt");
    }
}

// IRQ handler
void irq_handler(struct registers* r) {
    // Send EOI (End Of Interrupt) to PIC
    irq_ack(r->int_no - 32);
    
    // Call registered handler if exists
    if (interrupt_handlers[r->int_no] != 0) {
        isr_t handler = interrupt_handlers[r->int_no];
        handler(r);
    }
}

// Register an interrupt handler
void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

// Send End Of Interrupt signal
void irq_ack(uint8_t irq) {
    // Send EOI to slave PIC if IRQ >= 8
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    
    // Send EOI to master PIC
    outb(0x20, 0x20);
}

// Enable specific IRQ
void irq_enable(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
    } else {
        port = 0xA1;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

// Disable specific IRQ
void irq_disable(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
    } else {
        port = 0xA1;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}
