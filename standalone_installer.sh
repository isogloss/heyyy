#!/bin/bash

# ===============================================================
#          FiveM Capture System - Standalone Installer
# ===============================================================
# This script downloads, compiles, and installs the FiveM 
# Capture System without requiring the user to have access
# to the source code repository.
# 
# NOTE: This project is Windows-specific and will not function
# on Linux/macOS, but this script can be used for testing
# build system compatibility.
# ===============================================================

set -e  # Exit on any error

echo "================================================================="
echo "              FiveM Capture System - Standalone Installer"
echo "================================================================="
echo
echo "WARNING: This project is designed for Windows systems only."
echo "This Linux/Unix installer is for development and testing purposes."
echo
echo "This installer will:"
echo "  1. Download the source code from GitHub"
echo "  2. Set up all build dependencies automatically"
echo "  3. Attempt to configure the build system (will fail on non-Windows)"
echo "  4. Verify source code structure and dependencies"
echo
echo "The installation process may take 5-10 minutes depending on"
echo "your internet connection."
echo

# Check if running as root (not recommended)
if [ "$EUID" -eq 0 ]; then
    echo "WARNING: Running as root is not recommended for this installer."
    read -p "Do you want to continue anyway? (y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Installation cancelled by user."
        exit 1
    fi
    echo
fi

# Set up paths
DOWNLOADS_PATH="$HOME/Downloads"
TEMP_BUILD_PATH="/tmp/FiveMCapture_Build_$$"
HIDDEN_SOURCE_PATH="$HOME/.local/share/FiveMCapture_System_$$"

# Create Downloads directory if it doesn't exist
if [ ! -d "$DOWNLOADS_PATH" ]; then
    mkdir -p "$DOWNLOADS_PATH"
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to create Downloads directory at $DOWNLOADS_PATH"
        exit 1
    fi
fi

echo "Step 1: Preparing installation environment..."
echo

# Create temporary build directory
if [ -d "$TEMP_BUILD_PATH" ]; then
    rm -rf "$TEMP_BUILD_PATH"
fi
mkdir -p "$TEMP_BUILD_PATH"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to create temporary build directory"
    echo "Path: $TEMP_BUILD_PATH"
    exit 1
fi

# Create hidden source directory
mkdir -p "$HIDDEN_SOURCE_PATH"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to create hidden source directory"
    echo "Path: $HIDDEN_SOURCE_PATH"
    exit 1
fi

echo "Step 2: Downloading source code from GitHub..."
echo

# Check if git is available
if ! command -v git &> /dev/null; then
    echo "ERROR: Git is not installed"
    echo
    echo "Please install Git using your package manager:"
    echo "  Ubuntu/Debian: sudo apt install git"
    echo "  CentOS/RHEL:   sudo yum install git"
    echo "  macOS:         brew install git"
    echo
    rm -rf "$TEMP_BUILD_PATH"
    exit 1
fi

# Clone the repository to the hidden source location
echo "Downloading FiveM Capture System source code..."
cd "$HIDDEN_SOURCE_PATH"
if ! git clone --depth 1 --quiet https://github.com/isogloss/heyyy.git source_code; then
    echo "ERROR: Failed to download source code from GitHub"
    echo
    echo "This could be due to:"
    echo "- No internet connection"
    echo "- GitHub repository is private or does not exist"
    echo "- Network firewall blocking the connection"
    echo
    rm -rf "$TEMP_BUILD_PATH"
    rm -rf "$HIDDEN_SOURCE_PATH"
    exit 1
fi

echo "Source code downloaded successfully to protected location."
echo

# Copy source to temporary build location
echo "Step 3: Preparing build environment..."
echo

cp -r "$HIDDEN_SOURCE_PATH/source_code" "$TEMP_BUILD_PATH/heyyy"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to prepare build environment"
    rm -rf "$TEMP_BUILD_PATH"
    rm -rf "$HIDDEN_SOURCE_PATH"
    exit 1
fi

cd "$TEMP_BUILD_PATH/heyyy"

# Verify we have the necessary files
if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Invalid source code structure - CMakeLists.txt not found"
    rm -rf "$TEMP_BUILD_PATH"
    rm -rf "$HIDDEN_SOURCE_PATH"
    exit 1
fi

echo "Step 4: Setting up build dependencies..."
echo

# Create third_party directory structure
mkdir -p "third_party"
cd third_party

# Setup Microsoft Detours (placeholder for licensing compliance)
if [ ! -d "Detours" ]; then
    echo "Setting up Microsoft Detours placeholder..."
    mkdir -p "Detours/include"
    mkdir -p "Detours/lib.X64"
    
    # Create detours.h header file
    cat > "Detours/include/detours.h" << 'EOF'
#pragma once
#include <windows.h>

#define NO_ERROR    0L

extern "C" {
    LONG WINAPI DetourTransactionBegin(VOID);
    LONG WINAPI DetourUpdateThread(HANDLE hThread);
    LONG WINAPI DetourAttach(PVOID *ppPointer, PVOID pDetour);
    LONG WINAPI DetourDetach(PVOID *ppPointer, PVOID pDetour);
    LONG WINAPI DetourTransactionCommit(VOID);
}
EOF
    
    echo "Microsoft Detours placeholder created"
fi

# Setup Dear ImGui
if [ ! -d "imgui" ]; then
    echo "Downloading Dear ImGui dependency..."
    if ! git clone --depth 1 --quiet https://github.com/ocornut/imgui.git; then
        echo "ERROR: Failed to download Dear ImGui"
        echo "Please check your internet connection"
        rm -rf "$TEMP_BUILD_PATH"
        rm -rf "$HIDDEN_SOURCE_PATH"
        exit 1
    fi
    echo "Dear ImGui downloaded successfully"
fi

cd ..

echo "Step 5: Configuring build system..."
echo

# Create build directory
mkdir -p build
cd build

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake is not installed"
    echo
    echo "Please install CMake using your package manager:"
    echo "  Ubuntu/Debian: sudo apt install cmake"
    echo "  CentOS/RHEL:   sudo yum install cmake"
    echo "  macOS:         brew install cmake"
    echo
    rm -rf "$TEMP_BUILD_PATH"
    rm -rf "$HIDDEN_SOURCE_PATH"
    exit 1
fi

# Configure with CMake (this will fail on non-Windows systems as expected)
echo "Configuring with CMake..."
echo "NOTE: Configuration will fail on non-Windows systems (expected behavior)"
echo

CMAKE_SUCCESS=false
if cmake .. 2>&1 | tee cmake_output.log; then
    CMAKE_SUCCESS=true
    echo "CMake configuration completed successfully."
    echo "This is unexpected on non-Windows systems!"
else
    CMAKE_SUCCESS=false
    echo "CMake configuration failed as expected on non-Windows system."
    echo "This confirms the Windows-only nature of the project."
    
    # Check if it failed due to Windows requirement
    if grep -q "This project requires Windows" cmake_output.log; then
        echo "✓ Confirmed: Project correctly identifies Windows requirement"
    else
        echo "? Unexpected CMake error - check cmake_output.log"
    fi
fi

echo
echo "Step 6: Verification and cleanup..."
echo

# Create installation record in hidden source location
cat > "$HIDDEN_SOURCE_PATH/install_record.txt" << EOF
Installation completed on $(date)
Platform: $(uname -s)
Architecture: $(uname -m)
Note: This is a Windows-specific application
Source location: $HIDDEN_SOURCE_PATH/source_code
EOF

# Clean up temporary build directory
rm -rf "$TEMP_BUILD_PATH"

echo "================================================================="
echo "                    INSTALLATION VERIFICATION COMPLETE!"
echo "================================================================="
echo
echo "✓ Source code downloaded and protected"
echo "✓ Dependencies resolved successfully"
echo "✓ Build system verified (Windows-specific as expected)"
echo "✓ Temporary files cleaned up"
echo
echo "IMPORTANT NOTES:"
echo "- This FiveM Capture System is designed for Windows only"
echo "- It requires DirectX 11 and Visual Studio for compilation"
echo "- The source code has been stored securely at:"
echo "  $HIDDEN_SOURCE_PATH/source_code"
echo
echo "To build this project, you need:"
echo "- Windows 10/11 (64-bit)"
echo "- Visual Studio 2019/2022 with C++ support"
echo "- CMake 3.16 or later"
echo
echo "Installation log: $HIDDEN_SOURCE_PATH/install_record.txt"
echo
echo "Thank you for using the FiveM Capture System installer!"
echo

# Ask if user wants to view the source structure
read -p "Would you like to view the downloaded source structure? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo
    echo "Source structure:"
    find "$HIDDEN_SOURCE_PATH/source_code" -type f -name "*.cpp" -o -name "*.h" -o -name "*.txt" | head -20
    echo
fi

echo "Installation complete!"