#!/bin/bash

# AI Agent Team Test Launch Script  
# Replicates VS Code F5 launch for Test configuration
# Equivalent to: VS Code -> Run and Debug -> (Windows) Launch Test

set -e

echo "=== AI Agent Team Test Launch ==="
echo "Replicating VS Code F5 test launch..."

# Check if Test.exe exists
if [ ! -f "Bin/RelWithDebInfo/Test.exe" ]; then
    echo "❌ Test.exe not found at Bin/RelWithDebInfo/Test.exe"
    echo "Run ai-build.sh first to build the project"
    exit 1
fi

echo "✅ Test.exe found"
echo ""

# Change to correct working directory (matching VS Code launch.json cwd)
cd Bin

echo "Working directory: $(pwd)"
echo "Launching Test.exe with headless engine..."
echo ""

# Launch Test.exe (matching VS Code launch configuration)
# Note: Test.exe already configured with "headless" argument internally
./RelWithDebInfo/Test.exe

echo ""
echo "=== Test execution complete ==="
