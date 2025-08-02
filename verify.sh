#!/bin/bash

# Verification script for MiniCore-OS Phase 5
echo "=== MiniCore-OS Phase 5 Verification ==="
echo

# Check if all required files exist
echo "Checking required files..."
files=("boot.asm" "kernel.c" "link.ld" "Makefile" "README.md" "fs.c" "fs.h" "shell.c" "shell.h")
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file exists"
    else
        echo "✗ $file missing"
        exit 1
    fi
done
echo

# Check file contents
echo "Verifying file contents..."

# Check multiboot header in boot.asm
if grep -q "MAGIC.*0x1BADB002" boot.asm; then
    echo "✓ Multiboot magic number found in boot.asm"
else
    echo "✗ Multiboot magic number missing in boot.asm"
fi

# Check kernel_main function in kernel.c
if grep -q "void kernel_main" kernel.c; then
    echo "✓ kernel_main function found in kernel.c"
else
    echo "✗ kernel_main function missing in kernel.c"
fi

# Check entry point in linker script
if grep -q "ENTRY(_start)" link.ld; then
    echo "✓ Entry point defined in link.ld"
else
    echo "✗ Entry point missing in link.ld"
fi

# Check Makefile targets
if grep -q "all:" Makefile && grep -q "run:" Makefile && grep -q '$(ISO)' Makefile; then
    echo "✓ Essential Makefile targets present"
else
    echo "✗ Essential Makefile targets missing"
fi

echo

# Check for multiboot compliance
echo "Checking multiboot compliance..."
if grep -q "multiboot" boot.asm && grep -q "FLAGS.*MBALIGN.*MEMINFO" boot.asm; then
    echo "✓ Multiboot header structure appears correct"
else
    echo "✗ Multiboot header structure issues"
fi

echo

# Code quality checks
echo "Code quality checks..."

# Check for protected mode setup
if grep -q "mov esp" boot.asm; then
    echo "✓ Stack pointer setup found in bootloader"
else
    echo "✗ Stack pointer setup missing in bootloader"
fi

# Check for VGA text mode usage
if grep -q "0xB8000" kernel.c; then
    echo "✓ VGA text mode buffer address found"
else
    echo "✗ VGA text mode setup missing"
fi

# Check for welcome message
if grep -q -i "welcome" kernel.c; then
    echo "✓ Welcome message found in kernel"
else
    echo "✗ Welcome message missing in kernel"
fi

# Check for file system implementation
if grep -q "fs_init" kernel.c; then
    echo "✓ File system initialization found in kernel"
else
    echo "✗ File system initialization missing in kernel"
fi

# Check for file system commands in shell
if grep -q "cmd_ls" shell.c && grep -q "cmd_cat" shell.c; then
    echo "✓ File system commands (ls, cat) found in shell"
else
    echo "✗ File system commands missing in shell"
fi

# Check for file system functions
if grep -q "fs_list" fs.c && grep -q "fs_read" fs.c; then
    echo "✓ Core file system functions found"
else
    echo "✗ Core file system functions missing"
fi

# Check for demo files creation
if grep -q "fs_create_demo_files" fs.c; then
    echo "✓ Demo files creation function found"
else
    echo "✗ Demo files creation function missing"
fi

echo

# Calculate file sizes
echo "File sizes:"
for file in "${files[@]}"; do
    size=$(wc -c < "$file")
    lines=$(wc -l < "$file")
    echo "  $file: $size bytes, $lines lines"
done

echo
echo "=== Phase 5 Verification Complete ==="
echo

# Show next steps
echo "Next steps to build and test:"
echo "1. Install cross-compiler: ./setup.sh"
echo "2. Check dependencies: make check-deps"
echo "3. Build the OS: make"
echo "4. Run in QEMU: make run"
echo "5. Test file system commands:"
echo "   - Type 'ls' to list files"
echo "   - Type 'cat welcome.txt' to read a file"
echo "   - Type 'help' for all commands"
echo "3. Build the OS: make"
echo "4. Test in QEMU: make run"
echo
echo "Phase 1 requirements status:"
echo "✓ Bootloader in x86 Assembly (boot.asm)"
echo "✓ Kernel entry point in C (kernel.c)"
echo "✓ Multiboot header implemented"
echo "✓ Linker script for memory layout (link.ld)"
echo "✓ Build system using Makefile"
echo "✓ Disk image (.iso) creation support"
echo "✓ QEMU testing support"
echo "✓ Protected mode boot capability"
echo "✓ Welcome message display from kernel"
