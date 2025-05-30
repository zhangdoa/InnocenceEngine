# AI AGENT TEAM PEER REVIEW PROCESS
**Quality Gate for AI-Generated Commits**

## Overview
All AI Agent Team commits should pass peer review before user approval. This process enforces compliance with existing project standards and prevents policy violations.

## Workflow (Single Environment)
Since all AI agents run in the same environment (same Claude instance), the simplified workflow is:
```
Agent A (Creates Changes) → Agent B (Peer Review) → User (Final Approval) → Commit
```

**No Branching Required** - All agents work in the same environment, so feature branches are unnecessary overhead unless agents are running on different physical machines/terminals/processes.

## Working Directory Compliance
**CRITICAL**: Always match VS Code working directory settings from `.vscode/launch.json`:
- Test execution: `cd C:\GitRepo\InnocenceEngine\Bin && Test.exe [args]`
- Main execution: `cd C:\GitRepo\InnocenceEngine\Bin && Main.exe [args]`  
- Match `"cwd": "${workspaceFolder}/Bin"` from VS Code configuration

## Peer Review Validation

The reviewing agent must verify compliance with all **existing mandatory documents**:

### 1. **Commit Message Policy Compliance**
✅ **REFERENCE:** `Documents/commit-message-policy.md`
- Verify exact format compliance (subject, description, bullets, AI attribution)
- Ensure no deviations from established policy

### 2. **Code Standards Compliance** 
✅ **REFERENCE:** `Documents/code-standards.md`
- Verify naming conventions, header patterns, threading philosophy
- Check critical inline function rules and forbidden patterns

### 3. **Technical Validation**
- Code compiles successfully
- Changes achieve stated objectives  
- No security/privacy issues (absolute paths, etc.)
- Functionality verified where possible

## Review Documentation
Each peer review must document:
```
## Peer Review by [Reviewing Agent Name]

**Commit:** [description]

- [✅/❌] Commit message policy compliance (see Documents/commit-message-policy.md)
- [✅/❌] Code standards compliance (see Documents/code-standards.md)  
- [✅/❌] Technical validation passed
- [✅/❌] Security/privacy review passed

**Status:** [APPROVED / NEEDS CHANGES]
**Comments:** [specific issues if any]
```

## Enforcement
- **Quality Focus**: Peer review ensures compliance and prevents policy violations
- **Same Environment**: No complex git branching since all agents share the same workspace
- **User Authority**: Final approval always required from user before committing
- **Working Directory**: Must match VS Code settings to prevent log file inclusion

This process ensures all existing project standards are maintained without unnecessary distributed workflow complexity.
