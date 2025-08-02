#!/bin/bash

# Cross-compiler installation script for MiniCore-OS
# This script helps install the i686-elf-gcc cross-compiler

set -e

echo "=== MiniCore-OS Cross-Compiler Setup ==="
echo

# Detect OS
if [ -f /etc/debian_version ]; then
    OS="debian"
elif [ -f /etc/arch-release ]; then
    OS="arch"
elif [ -f /etc/fedora-release ]; then
    OS="fedora"
else
    OS="unknown"
fi

echo "Detected OS: $OS"
echo

# Function to install dependencies for Debian/Ubuntu
install_debian_deps() {
    echo "Installing dependencies for Debian/Ubuntu..."
    # sudo apt-get update
    # sudo apt-get install -y build-essential nasm qemu-system-x86 grub-pc-bin grub-common xorriso mtools
    # sudo apt-get install -y bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo wget
}

# Function to install dependencies for Arch Linux
install_arch_deps() {
    echo "Installing dependencies for Arch Linux..."
    sudo pacman -S --needed base-devel nasm qemu grub xorriso libisoburn mtools
    sudo pacman -S --needed bison flex gmp libmpc mpfr texinfo wget
    
    # Try to install from AUR if available
    if command -v yay &> /dev/null; then
        echo "Installing cross-compiler from AUR..."
        yay -S i686-elf-gcc i686-elf-binutils
        return $?
    fi
}

# Function to build cross-compiler from source
build_crosscompiler() {
    echo "Building cross-compiler from source..."
    echo "This will take a while (30-60 minutes)..."
    
    # Configuration
    export PREFIX="$HOME/opt/cross"
    export TARGET=i686-elf
    export PATH="$PREFIX/bin:$PATH"
    
    # Create directories
    mkdir -p ~/cross-compiler-build
    cd ~/cross-compiler-build
    
    # Download sources
    echo "Downloading binutils..."
    wget -nc https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.xz
    echo "Downloading GCC..."
    wget -nc https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz
    
    # Extract
    echo "Extracting sources..."
    tar -xf binutils-2.40.tar.xz
    tar -xf gcc-13.2.0.tar.xz
    
    # Build binutils
    echo "Building binutils..."
    mkdir -p build-binutils
    cd build-binutils
    ../binutils-2.40/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    make -j$(nproc)
    make install
    cd ..
    
    # Build GCC
    echo "Building GCC..."
    mkdir -p build-gcc
    cd build-gcc
    ../gcc-13.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    make all-gcc -j$(nproc)
    make all-target-libgcc -j$(nproc)
    make install-gcc
    make install-target-libgcc
    cd ..
    
    echo ""
    echo "Cross-compiler built successfully!"
    echo "Add this to your ~/.bashrc or ~/.zshrc:"
    echo "export PATH=\"$PREFIX/bin:\$PATH\""
    echo ""
    echo "Then run: source ~/.bashrc"
}

# Main installation logic
case $OS in
    "debian")
        install_debian_deps
        echo ""
        echo "Dependencies installed. Now building cross-compiler from source..."
        build_crosscompiler
        ;;
    "arch")
        install_arch_deps
        if [ $? -ne 0 ]; then
            echo "AUR installation failed. Building from source..."
            build_crosscompiler
        fi
        ;;
    "fedora")
        echo "Installing dependencies for Fedora..."
        sudo dnf install -y gcc gcc-c++ make nasm qemu-system-x86 grub2-tools xorriso
        sudo dnf install -y bison flex gmp-devel libmpc-devel mpfr-devel texinfo wget
        build_crosscompiler
        ;;
    *)
        echo "Unknown OS. Installing basic dependencies and building from source..."
        echo "Please install these packages manually:"
        echo "- build-essential or equivalent"
        echo "- nasm"
        echo "- qemu-system-x86"
        echo "- grub tools"
        echo "- bison, flex, gmp-devel, libmpc-devel, mpfr-devel, texinfo"
        echo ""
        read -p "Press Enter to continue with cross-compiler build..."
        build_crosscompiler
        ;;
esac

echo ""
echo "=== Setup Complete ==="
echo "Next steps:"
echo "1. If cross-compiler was built from source, add to PATH:"
echo "   export PATH=\"\$HOME/opt/cross/bin:\$PATH\""
echo "2. Test the setup:"
echo "   cd /home/atsuomi/Documents/projects/minicore-os"
echo "   make check-deps"
echo "3. Build the OS:"
echo "   make"
echo "4. Run in QEMU:"
echo "   make run"
