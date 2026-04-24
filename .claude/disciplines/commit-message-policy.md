# Discipline: commit-message-policy

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
Code-AI-Generated-By: [AI Agent Model Name]
```

### 👤 Human Commits  

**No attribution line needed.**

### 🤝 Hybrid Scenarios

**AI Code + Human Message:**

```
Message-Human-Written: [Human Name]
Code-AI-Generated-By: [AI Agent Model Name]
```

**Human Code + AI Message:**

```
Message-AI-Generated-By: [AI Agent Model Name]
Code-Human-Written: [Human Name]
```

## AI Agent Scratch File

AI agents draft messages in `Build/commit-message.txt` (gitignored) and commit
with `git commit -F Build/commit-message.txt`.

## Harness enforcement

`.claude/hooks/commit-gate.js` runs as a `PreToolUse` hook on Bash and blocks
`git commit` unless the commit message contains `Code-AI-Generated-By:` or
`Message-AI-Generated-By:`. It reads messages from either `-m ...` or `-F path`.
See `CLAUDE.md` → Harness enforcement for the test-run gate that runs
alongside.

## Example

```
Fix ObjectPool memory leak with proper destructor

Resolves critical memory leak where ObjectPool heap was never deallocated,
causing accumulating memory usage in long-running applications.

- Add destructor with proper heap deallocation
- Add null pointer safety check  
- Use engine memory management system
- Add debug logging for allocation tracking

Code-AI-Generated-By: Claude Sonnet 4.6
```

## Enforcement

- **AI commits require attribution** - missing attribution blocks commit
- **Attribution scope** - indicates who authored code and/or message
- **Git history transparency** - clear distinction between human and AI work
