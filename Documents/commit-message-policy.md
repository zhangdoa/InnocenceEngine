# COMMIT MESSAGE POLICY
**Version:** 2.0 - Clarified AI Attribution Scope

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

Code-AI-Generated-By: Claude (Anthropic AI Assistant)
```

### Key Requirements:
- **Subject line**: Imperative mood, under 72 chars, no period
- **Description**: 1-2 sentences explaining what/why
- **Bullet points**: Specific changes made
- **AI attribution**: `Code-AI-Generated-By: [AI System Name]` - **MANDATORY**
  - **SCOPE**: This attribution indicates the entire changelist (code + commit message) was AI-generated
  - **FORMAT**: Must be the exact final line of the commit message
  - **CLARITY**: Distinguishes from human commits and specifies full AI authorship

### Examples:
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

## 👤 **HUMAN COMMITS**
Human commits use the same format but WITHOUT any AI attribution line.

### Format:
```
Subject line: Clear, imperative, under 72 characters

Brief description paragraph explaining what and why the changes were made.

- Bullet point list of specific changes
- Each change should be clear and actionable  
- Use imperative mood consistently
```

## 🤝 **HYBRID SCENARIOS**

### AI Code + Human Message:
```
Subject line: Human-written

Human-written description of the changes.

- Human-written bullet points
- Describing the changes

Message-Human-Written: [Human Name]
Code-AI-Generated-By: Claude (Anthropic AI Assistant)
```

### Human Code + AI Message:
```
Subject line: AI-written

AI-written description of the changes.

- AI-written bullet points
- Describing the changes

Message-AI-Generated-By: Claude (Anthropic AI Assistant)
Code-Human-Written: [Human Name]
```

---

## 🚨 **ENFORCEMENT**

- **All AI commits MUST include attribution** - commits will be rejected without proper `Code-AI-Generated-By` footer
- **Code review process checks attribution** - missing or incorrect attribution blocks merge  
- **Git history transparency** - ensures clear distinction between human and AI work
- **Professional documentation** - maintains project integrity and legal clarity
- **Scope clarity** - `Code-AI-Generated-By` indicates entire changelist is AI-authored (code + message)

This policy ensures transparent, professional commit history with unambiguous AI attribution.
