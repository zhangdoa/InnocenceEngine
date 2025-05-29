#!/bin/bash
# InnocenceEngine AI Agent Team Launcher

# Set variables
MCP_CONFIG="~/.config/claude-desktop/claude_desktop_config.json"

echo "🚀 Launching InnocenceEngine AI Agent Team..."

# Code Architect (Lead coordinator)
echo "📋 Starting Code Architect..."
tmux new-session -d -s architect \
  "claude-code --name 'code-architect' --session 'innocence-architect' --mcp-config $MCP_CONFIG ."

# Memory & Performance Specialist
echo "🧠 Starting Memory & Performance Specialist..."
tmux new-session -d -s memory \
  "claude-code --name 'memory-perf-specialist' --session 'innocence-memory' --mcp-config $MCP_CONFIG ."

# Platform & Build Specialist
echo "🔧 Starting Platform & Build Specialist..."
tmux new-session -d -s build \
  "claude-code --name 'platform-build-specialist' --session 'innocence-build' --mcp-config $MCP_CONFIG ."

# Rendering Systems Expert
echo "🎨 Starting Rendering Systems Expert..."
tmux new-session -d -s rendering \
  "claude-code --name 'rendering-expert' --session 'innocence-rendering' --mcp-config $MCP_CONFIG ."

# Core Systems Developer
echo "⚙️ Starting Core Systems Developer..."
tmux new-session -d -s core \
  "claude-code --name 'core-systems-dev' --session 'innocence-core' --mcp-config $MCP_CONFIG ."

# Tools & Editor Specialist
echo "🛠️ Starting Tools & Editor Specialist..."
tmux new-session -d -s tools \
  "claude-code --name 'tools-editor-specialist' --session 'innocence-tools' --mcp-config $MCP_CONFIG ."

echo ""
echo "✅ All AI agents launched! Use these commands to connect:"
echo "  tmux attach -t architect    # Code Architect (Lead)"
echo "  tmux attach -t memory       # Memory & Performance"
echo "  tmux attach -t build        # Platform & Build"
echo "  tmux attach -t rendering    # Rendering Systems"
echo "  tmux attach -t core         # Core Systems"
echo "  tmux attach -t tools        # Tools & Editor"
echo ""
echo "📝 Team coordination through: ./AI Agent Team/"
echo "🔍 List all sessions: tmux list-sessions"
echo "🛑 Kill all sessions: tmux kill-server"