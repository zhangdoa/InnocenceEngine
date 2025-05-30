#!/bin/bash

# AI Agent Team Build Script
# Replicates VS Code build button functionality
# Equivalent to: VS Code Build Button -> CMake Configure -> Build

set -e

echo "=== AI Agent Team Build Script ==="
echo "Replicating VS Code build workflow..."

# Step 1: CMake Configure (equivalent to VS Code CMake extension)
echo ""
echo "Step 1: CMake Configure..."

cmake -S . -B build \
    -DPREPARE_ENV=ON \
    -DBUILD_THIRD_PARTY=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DFORCE_REBUILD_THIRD_PARTY=OFF \
    -G "Visual Studio 17 2022" \
    -A x64

echo "✅ CMake configuration complete"

# Step 2: Build (equivalent to VS Code build task)
echo ""
echo "Step 2: Building engine..."

cmake --build build --config RelWithDebInfo --parallel 4

echo "✅ Build complete"

# Step 3: Check outputs
echo ""
echo "Step 3: Verifying build outputs..."

if [ -f "Bin/RelWithDebInfo/Main.exe" ]; then
    echo "✅ Main.exe found"
else
    echo "❌ Main.exe not found"
fi

if [ -f "Bin/RelWithDebInfo/Test.exe" ]; then
    echo "✅ Test.exe found"
else
    echo "❌ Test.exe not found"
fi

echo ""
echo "=== Build script complete ==="
echo "Ready for launch scripts!"
