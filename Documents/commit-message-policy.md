# COMMIT MESSAGE POLICY
**Version:** 1.1 - Updated AI Attribution Format

## 🤖 **AI-GENERATED COMMITS - MANDATORY ATTRIBUTION**

All commits written by AI must include proper attribution with this exact format:

### Required Format:
```
Subject line: Clear, imperative, under 72 characters

Brief description paragraph explaining what and why the changes were made,
written in 1-2 sentences for better readability and context.

- Bullet point list of specific changes
- Each change should be clear and actionable  
- Use imperative mood consistently
- Keep bullets concise but descriptive

AI-Generated-By: Claude (Anthropic AI Assistant)
```

### Key Requirements:
- **Subject line**: Imperative mood, under 72 chars, no period
- **Description**: 1-2 sentences explaining what/why
- **Bullet points**: Specific changes made
- **AI attribution**: `AI-Generated-By: [AI System Name]` - **MANDATORY**

### Examples:
```
Fix ObjectPool memory leak with proper destructor

Resolves critical memory leak where ObjectPool heap was never deallocated,
causing accumulating memory usage in long-running applications.

- Add destructor with proper heap deallocation
- Add null pointer safety check
- Use engine memory management system
- Add debug logging for allocation tracking

AI-Generated-By: Claude (Anthropic AI Assistant)
```

## 👤 **HUMAN COMMITS**
Human commits use the same format but WITHOUT the AI attribution line.

### Format:
```
Subject line: Clear, imperative, under 72 characters

Brief description paragraph explaining what and why the changes were made.

- Bullet point list of specific changes
- Each change should be clear and actionable  
- Use imperative mood consistently
```

---

## 🚨 **ENFORCEMENT**

- **All AI commits MUST include attribution** - commits will be rejected without it
- **Code review process checks attribution** - missing attribution blocks merge  
- **Git history transparency** - ensures clear distinction between human and AI work
- **Professional documentation** - maintains project integrity and legal clarity

This policy ensures transparent, professional commit history with proper AI attribution.
