#!/bin/bash
# Build script for pick6 development testing on Linux

echo "Building pick6 - Development Test Build"
echo "======================================="

# Check for cmake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found. Please install cmake."
    echo "On Ubuntu/Debian: sudo apt-get install cmake build-essential"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure
echo "Configuring build..."
cmake ..
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed."
    exit 1
fi

# Build
echo "Building..."
make
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed."
    exit 1
fi

echo
echo "Build completed successfully!"
echo "Output: $(pwd)/bin/pick6"
echo
echo "Note: This is a stub build for development testing."
echo "The full application requires Windows and DirectX 11."