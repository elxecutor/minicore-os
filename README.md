# MiniCore-OS

A minimal operating system kernel with bootloader written from scratch in x86 Assembly and C.

## Features

- **Multiboot-compliant bootloader** written in x86 Assembly
- **32-bit protected mode kernel** written in C
- **Memory management system** with heap allocation
- **Interactive CLI shell** with keyboard input
- **VGA text mode display** with colored output
- **Interrupt system** with IDT and ISR handlers
- **Cooperative multitasking** with task scheduling
- **Read-only file system** with preloaded demo files
- **Built-in commands** for system control and debugging
- **File system commands:** `ls` (list files) and `cat` (display file contents)
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

## File System Usage

The OS includes a read-only in-memory file system with preloaded demo files:

### Available Commands
- `ls` - List all files in the file system
- `cat <filename>` - Display the contents of a file

### Demo Files
- `welcome.txt` - Introduction to MiniCore-OS
- `system.txt` - System information and specifications  
- `readme.txt` - File system documentation
- `hello.c` - Sample C code
- `license.txt` - License information

### Example Usage
```
minicore> ls
=== File System Contents ===
Name                     Size   Type
------------------------ ------ --------
welcome.txt              387    TEXT
system.txt              245    TEXT
readme.txt              312    TEXT
hello.c                  89     TEXT
license.txt             156    TEXT

Total files: 5 / 16

minicore> cat welcome.txt
=== Contents of welcome.txt ===
Welcome to MiniCore-OS!
This is a simple read-only file system.
Try 'ls' to list files and 'cat <filename>' to read them.
...
=== End of file ===
```

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

### File System

The read-only file system implementation provides:
- **In-memory storage:** Files preloaded at boot time
- **Fixed allocation:** Maximum 16 files, 4KB each
- **Directory abstraction:** Flat namespace with filename lookup
- **Type support:** Text and binary file types
- **Integration:** Shell commands `ls` and `cat`

**Architecture:**
- `fs_t` structure holds filesystem metadata
- `fs_file_t` entries contain file information
- Static memory allocation for file data
- String-based filename matching
- Read-only permissions enforced

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

This project has completed Phase 5 of the OS development:

**Completed Phases:**
- ✅ Phase 1: Basic bootloader and kernel
- ✅ Phase 2: Memory management (heap allocation)  
- ✅ Phase 3: Interactive CLI shell
- ✅ Phase 4: Interrupt handling (IDT/ISR)
- ✅ Phase 5: Read-only file system

**Future phases might include:**
- Process management and user mode
- Writable file system
- Device drivers (keyboard, timer, disk)
- Network stack
- Graphics mode support
- System calls and user programs

## License

For guidelines on contributing, please see the [CONTRIBUTING.md](CONTRIBUTING.md) file. By participating in this project, you are expected to adhere to our [Code of Conduct](CODE_OF_CONDUCT.md).

## Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [Multiboot Specification](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
- [x86 Assembly Guide](https://en.wikibooks.org/wiki/X86_Assembly)
- [Intel Software Developer Manuals](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
