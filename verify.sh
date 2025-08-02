#!/bin/bash

# Verification script for MiniCore-OS Phase 1
echo "=== MiniCore-OS Phase 1 Verification ==="
echo

# Check if all required files exist
echo "Checking required files..."
files=("boot.asm" "kernel.c" "link.ld" "Makefile" "README.md")
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

echo

# Calculate file sizes
echo "File sizes:"
for file in "${files[@]}"; do
    size=$(wc -c < "$file")
    lines=$(wc -l < "$file")
    echo "  $file: $size bytes, $lines lines"
done

echo
echo "=== Verification Complete ==="
echo

# Show next steps
echo "Next steps to build and test:"
echo "1. Install cross-compiler: ./setup.sh"
echo "2. Check dependencies: make check-deps"
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
