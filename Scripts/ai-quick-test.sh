#!/bin/bash

# AI Agent Team Quick Test Script
# One-stop build and test execution
# Equivalent to: VS Code Build Button + F5 Test Launch

set -e

echo "=== AI Agent Team Quick Test ==="
echo "Build + Test in one command..."
echo ""

# Step 1: Build
echo "🔨 Building project..."
bash Scripts/ai-build.sh

echo ""
echo "🚀 Launching tests..."

# Step 2: Test  
bash Scripts/ai-test.sh

echo ""
echo "=== Quick test complete ==="
