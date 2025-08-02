# Makefile for MiniCore-OS (with alternative compiler support)

# Compiler and assembler settings
# Try cross-compiler first, fall back to system compiler
CC_CROSS = i686-elf-gcc
CC_SYSTEM = gcc -m32
AS = nasm
LD_CROSS = i686-elf-gcc
LD_SYSTEM = gcc -m32

# Check which compiler to use
CC := $(shell which $(CC_CROSS) >/dev/null 2>&1 && echo $(CC_CROSS) || echo $(CC_SYSTEM))
LD := $(shell which $(LD_CROSS) >/dev/null 2>&1 && echo $(LD_CROSS) || echo $(LD_SYSTEM))

# Compiler flags
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
LDFLAGS = -ffreestanding -O2 -nostdlib -lgcc

# Assembly flags
ASFLAGS = -f elf32

# Target files
KERNEL = kernel.bin
ISO = os.iso
BOOT_OBJ = boot.o
KERNEL_OBJ = kernel.o
MM_OBJ = mm.o
SHELL_OBJ = shell.o
IDT_OBJ = idt.o
ISR_OBJ = isr.o
INTERRUPT_OBJ = interrupt.o
SCHEDULER_OBJ = scheduler.o
TASK_SWITCH_OBJ = task_switch.o
FS_OBJ = fs.o

OBJECTS = $(BOOT_OBJ) $(KERNEL_OBJ) $(MM_OBJ) $(SHELL_OBJ) $(IDT_OBJ) $(ISR_OBJ) $(INTERRUPT_OBJ) $(SCHEDULER_OBJ) $(TASK_SWITCH_OBJ) $(FS_OBJ)

# GRUB configuration
GRUB_CFG = grub.cfg
ISO_DIR = isodir
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub

.PHONY: all clean iso run check-deps

all: check-deps $(ISO)

# Check for required dependencies
check-deps:
	@echo "Checking for required dependencies..."
	@which $(AS) > /dev/null || (echo "Error: $(AS) not found. Please install NASM assembler." && exit 1)
	@which grub-mkrescue > /dev/null || (echo "Error: grub-mkrescue not found. Please install GRUB utilities." && exit 1)
	@which qemu-system-i386 > /dev/null || echo "Warning: qemu-system-i386 not found. Install QEMU to test the OS."
	@if which $(CC_CROSS) >/dev/null 2>&1; then \
		echo "Using cross-compiler: $(CC_CROSS)"; \
	else \
		echo "Cross-compiler not found, using system compiler: $(CC_SYSTEM)"; \
		echo "Note: Install i686-elf-gcc for proper cross-compilation"; \
		which gcc >/dev/null || (echo "Error: gcc not found" && exit 1); \
	fi
	@echo "Dependencies check complete."

# Assemble the bootloader
$(BOOT_OBJ): boot.asm
	$(AS) $(ASFLAGS) boot.asm -o $(BOOT_OBJ)

# Compile the kernel
$(KERNEL_OBJ): kernel.c mm.h shell.h
	$(CC) $(CFLAGS) -c kernel.c -o $(KERNEL_OBJ)

# Compile memory management
$(MM_OBJ): mm.c mm.h
	$(CC) $(CFLAGS) -c mm.c -o $(MM_OBJ)

# Compile shell
$(SHELL_OBJ): shell.c shell.h mm.h isr.h
	$(CC) $(CFLAGS) -c shell.c -o $(SHELL_OBJ)

# IDT object
$(IDT_OBJ): idt.c idt.h
	$(CC) $(CFLAGS) -c idt.c -o $(IDT_OBJ)

# ISR object
$(ISR_OBJ): isr.c isr.h idt.h
	$(CC) $(CFLAGS) -c isr.c -o $(ISR_OBJ)

# Interrupt handlers assembly
$(INTERRUPT_OBJ): interrupt.asm
	$(AS) $(ASFLAGS) interrupt.asm -o $(INTERRUPT_OBJ)

# Scheduler object
$(SCHEDULER_OBJ): scheduler.c scheduler.h isr.h
	$(CC) $(CFLAGS) -c scheduler.c -o $(SCHEDULER_OBJ)

# Task switching assembly
$(TASK_SWITCH_OBJ): task_switch.asm
	$(AS) $(ASFLAGS) task_switch.asm -o $(TASK_SWITCH_OBJ)

# File system object
$(FS_OBJ): fs.c fs.h
	$(CC) $(CFLAGS) -c fs.c -o $(FS_OBJ)

# Link the kernel
$(KERNEL): $(OBJECTS) link.ld
	$(LD) -T link.ld -o $(KERNEL) $(OBJECTS) $(LDFLAGS)

# Create GRUB configuration
$(GRUB_CFG):
	@mkdir -p $(GRUB_DIR)
	@echo "menuentry \"MiniCore-OS\" {" > $(GRUB_DIR)/$(GRUB_CFG)
	@echo "    multiboot /boot/$(KERNEL)" >> $(GRUB_DIR)/$(GRUB_CFG)
	@echo "}" >> $(GRUB_DIR)/$(GRUB_CFG)

# Create bootable ISO
$(ISO): $(KERNEL) $(GRUB_CFG)
	@mkdir -p $(BOOT_DIR)
	@cp $(KERNEL) $(BOOT_DIR)/
	grub-mkrescue -o $(ISO) $(ISO_DIR)
	@echo "ISO created: $(ISO)"

# Run the OS in QEMU
run: $(ISO)
	@echo "Starting MiniCore-OS in QEMU..."
	qemu-system-i386 -cdrom $(ISO)

# Run with additional debugging options
debug: $(ISO)
	@echo "Starting MiniCore-OS in QEMU with debugging..."
	qemu-system-i386 -cdrom $(ISO) -monitor stdio -d int

# Test the kernel file
test-kernel: $(KERNEL)
	@echo "Testing kernel file..."
	file $(KERNEL)
	objdump -h $(KERNEL)
	@echo "Kernel size: $$(stat -c%s $(KERNEL)) bytes"

# Clean build artifacts
clean:
	rm -f $(BOOT_OBJ) $(KERNEL_OBJ) $(MM_OBJ) $(SHELL_OBJ) $(KERNEL) $(ISO)
	rm -rf $(ISO_DIR)
	@echo "Cleaned build artifacts"

# Build cross-compiler
build-cross-compiler:
	@echo "Building cross-compiler from source..."
	@echo "This will take 30-60 minutes..."
	./setup.sh

# Show help
help:
	@echo "MiniCore-OS Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all               - Build the complete OS ISO (default)"
	@echo "  iso               - Build the bootable ISO file"
	@echo "  run               - Build and run the OS in QEMU"
	@echo "  debug             - Run the OS in QEMU with debugging"
	@echo "  test-kernel       - Test and analyze the kernel binary"
	@echo "  build-cross-compiler - Build i686-elf-gcc from source"
	@echo "  clean             - Clean all build artifacts"
	@echo "  check-deps        - Check for required dependencies"
	@echo "  help              - Show this help message"
