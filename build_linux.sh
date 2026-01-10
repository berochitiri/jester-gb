#!/bin/bash
# jester-gb Linux Build Script

set -e

echo "Building jester-gb for Linux..."

# Check dependencies
if ! command -v cmake &> /dev/null; then
    echo "CMake not found. Please install cmake."
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo ""
echo "Build complete!"
echo "Executable: build/jester-gb"
echo ""

# Create release package
cd ..
mkdir -p release/jester-gb-linux-x64
cp build/jester-gb release/jester-gb-linux-x64/
cp README.md release/jester-gb-linux-x64/
mkdir -p release/jester-gb-linux-x64/roms

# Strip binary for smaller size
strip release/jester-gb-linux-x64/jester-gb

# Create tarball
cd release
tar -czvf jester-gb-v0.1.0-linux-x64.tar.gz jester-gb-linux-x64

echo ""
echo "Release package: release/jester-gb-v0.1.0-linux-x64.tar.gz"
