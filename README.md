# MiniCore-OS

A minimal operating system kernel with bootloader written from scratch in x86 Assembly and C.

## Features

- **Multiboot-compliant bootloader** written in x86 Assembly
- **32-bit protected mode kernel** written in C
- **Memory management system** with heap allocation
- **Interactive CLI shell** with keyboard input
- **VGA text mode display** with colored output
- **Built-in commands** for system control and debugging
- **Debug commands** for memory inspection
- **GRUB2 bootloader support** for easy testing
- **QEMU integration** for development and testing

## Prerequisites

### Required Tools

1. **Cross-compiler toolchain:**
   - `i686-elf-gcc` (C compiler for i686 target)
   - `i686-elf-binutils` (Linker and other tools)

2. **Assembler:**
   - `nasm` (Netwide Assembler)

3. **Bootloader utilities:**
   - `grub-mkrescue` (GRUB rescue disk creator)
   - `xorriso` (ISO 9660 filesystem utility)

4. **Testing (optional):**
   - `qemu-system-i386` (x86 emulator)

### Installation

#### Ubuntu/Debian:
```bash
# Install available packages
sudo apt-get update
sudo apt-get install build-essential nasm qemu-system-x86 grub-pc-bin grub-common xorriso mtools

# Note: i686-elf-gcc must be built from source or installed separately
```

#### Arch Linux:
```bash
# Install from official repos
sudo pacman -S base-devel nasm qemu grub xorriso libisoburn mtools

# Install cross-compiler from AUR
yay -S i686-elf-gcc i686-elf-binutils
```

#### Building Cross-Compiler (if needed):
Follow the guide at: https://wiki.osdev.org/GCC_Cross-Compiler

## Building

### Quick Start

```bash
# Check dependencies
make check-deps

# Build the OS
make

# Run in QEMU
make run
```

### Available Make Targets

- `make` or `make all` - Build the complete bootable ISO
- `make iso` - Build the ISO file only
- `make run` - Build and run the OS in QEMU
- `make debug` - Run with QEMU debugging enabled
- `make test-kernel` - Analyze the kernel binary
- `make clean` - Clean all build artifacts
- `make help` - Show all available targets

## Technical Details

### Boot Process

1. **GRUB Stage 1/2:** GRUB loads our kernel from the ISO
2. **Multiboot Header:** GRUB finds our multiboot header in `boot.asm`
3. **Protected Mode:** CPU is already in 32-bit protected mode
4. **Stack Setup:** Assembly code sets up a 16KB stack
5. **Kernel Entry:** Control transfers to `kernel_main()` in C

### Memory Layout

- **0x00100000 (1MB):** Kernel load address
- **Stack:** 16KB stack space in BSS section
- **VGA Buffer:** 0xB8000 (text mode video memory)

### Display System

The kernel includes a basic VGA text mode driver supporting:
- 80x25 character display
- 16 foreground colors
- 16 background colors
- Basic string output functions

## Testing

### QEMU
```bash
# Basic run
make run

# With debugging
make debug

# Manual QEMU command
qemu-system-i386 -cdrom os.iso
```

### Real Hardware

The generated `os.iso` can be written to a USB drive or CD and booted on real hardware:

```bash
# Write to USB (replace /dev/sdX with your USB device)
sudo dd if=os.iso of=/dev/sdX bs=4M status=progress
```

**⚠️ Warning:** This will erase all data on the target device!

## Expected Output

When the OS boots successfully, you should see:

```
Welcome to MiniCore-OS!
Kernel successfully loaded and running in protected mode.
Bootloader Phase 1 Complete!

System Information:
- Architecture: x86 (32-bit)
- Mode: Protected Mode
- Memory Management: Basic
- Display: VGA Text Mode (80x25)
```

## Troubleshooting

### Common Issues

1. **"i686-elf-gcc not found"**
   - Install the cross-compiler toolchain
   - Ensure it's in your PATH

2. **"grub-mkrescue not found"**
   - Install GRUB utilities for your distribution

3. **ISO won't boot**
   - Verify GRUB configuration
   - Check kernel binary with `make test-kernel`

4. **Black screen in QEMU**
   - Try different QEMU options
   - Check kernel for infinite loops

### Debug Tips

- Use `make debug` for QEMU debugging
- Check kernel size with `make test-kernel`
- Examine assembly output with `objdump -d kernel.bin`

## Next Steps

This is Phase 1 of the OS development. Future phases might include:

- Interrupt handling (IDT setup)
- Memory management (paging, heap)
- Process management
- File system
- Device drivers
- User mode programs

## License

This project is provided as educational material. Feel free to use and modify for learning purposes.

## Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [Multiboot Specification](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
- [x86 Assembly Guide](https://en.wikibooks.org/wiki/X86_Assembly)
- [Intel Software Developer Manuals](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
