#!/bin/bash

# AI Agent Team Main Launch Script
# Replicates VS Code F5 launch for Main configuration  
# Equivalent to: VS Code -> Run and Debug -> (Windows) Launch Main

set -e

echo "=== AI Agent Team Main Launch ==="
echo "Replicating VS Code F5 main launch..."

# Check if Main.exe exists
if [ ! -f "Bin/RelWithDebInfo/Main.exe" ]; then
    echo "❌ Main.exe not found at Bin/RelWithDebInfo/Main.exe"
    echo "Run ai-build.sh first to build the project"
    exit 1
fi

echo "✅ Main.exe found"
echo ""

# Change to correct working directory (matching VS Code launch.json cwd)
cd Bin

echo "Working directory: $(pwd)"
echo "Launching Main.exe with arguments from VS Code launch.json..."
echo ""

# Launch Main.exe with exact arguments from VS Code launch.json
./RelWithDebInfo/Main.exe -mode 0 -renderer 0 -loglevel 0 -headless

echo ""
echo "=== Main execution complete ==="
