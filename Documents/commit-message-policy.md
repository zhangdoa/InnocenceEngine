# COMMIT MESSAGE POLICY
**Version:** 2.1 - Streamlined Format

## Standard Format
All commits follow this structure:

```
Subject line: Clear, imperative, under 72 characters

Brief description explaining what and why the changes were made.

- Bullet point list of specific changes
- Each change should be clear and actionable  
- Use imperative mood consistently

[Attribution Line - see below]
```

## Attribution Requirements

### 🤖 AI-Generated Commits
**Mandatory attribution for AI-authored code and/or commit message:**
```
Code-AI-Generated-By: Claude (Anthropic AI Assistant)
```

### 👤 Human Commits  
**No attribution line needed.**

### 🤝 Hybrid Scenarios
**AI Code + Human Message:**
```
Message-Human-Written: [Human Name]
Code-AI-Generated-By: Claude (Anthropic AI Assistant)
```

**Human Code + AI Message:**
```
Message-AI-Generated-By: Claude (Anthropic AI Assistant)
Code-Human-Written: [Human Name]
```

## AI Agent Team Process
AI agents use temp file workflow:

1. Create message in `AI Agent Team/commit-message.txt`
2. User reviews code changes and commit message
3. User approves and then AI agents execute commit with `git commit -c AI Agent Team/commit-message.txt`
4. Reuse temp file for next commit

## Example
```
Fix ObjectPool memory leak with proper destructor

Resolves critical memory leak where ObjectPool heap was never deallocated,
causing accumulating memory usage in long-running applications.

- Add destructor with proper heap deallocation
- Add null pointer safety check  
- Use engine memory management system
- Add debug logging for allocation tracking

Code-AI-Generated-By: Claude (Anthropic AI Assistant)
```

## Enforcement
- **AI commits require attribution** - missing attribution blocks commit
- **Attribution scope** - indicates who authored code and/or message
- **Git history transparency** - clear distinction between human and AI work
