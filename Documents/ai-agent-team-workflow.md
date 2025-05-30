# AI AGENT TEAM WORKFLOW

**Quality Assurance Process for AI Development**

## Authority & Restrictions

### ✅ Autonomous (No Approval Needed)

- Code analysis and planning
- Running builds and tests
- Peer review
- Documentation research (can search information online if needed)

### ⚠️ Requires User Approval  

- Writing/modifying code files
- Creating new files
- Git commits
- Dependency changes

### 🚫 Forbidden

- Committing without testing
- Skipping peer review
- Breaking established standards
- Adding new observations when it's clearly should be updating the existing one instead

## Workflow Process

### 0. Preparation

- The team can use `AI Agent Team` folder to store anything needed but doesn't need to be visible for the user
- The team should start from the "Session Handoff State" and "AI Agent Team Current Status" entities in the memory graph as the context
- The team should ask for directions if no code work is ongoing

### 1. Implementation

- Follow code standards document
- Ensure commit message policy compliance

### 2. Mandatory Testing

**All changes must pass EVERY test:**

#### Build Validation

Use `RelWithDebugInfo` as the default unless another config is explicitly instructed by the user

```bash
# From Bin directory
cmake --build ../Build --config RelWithDebugInfo
```

- ✅ Full project builds without errors
- ✅ All executables generate successfully

#### Runtime Testing  

```bash
# From Bin directory
Test.exe -headless
```

- ✅ Test.exe starts and runs without crashes
- ✅ Basic engine initialization succeeds

#### Shader Testing (Graphics Changes Only)

```powershell
# From Scripts directory
.\HLSL2DXIL.ps1
```

- ✅ All modified shaders compile to DXIL

### 3. Peer Review

**Different agent must review:**

```
## Peer Review by [Agent Name]

- [✅/❌] Code standards compliance
- [✅/❌] Commit message policy compliance
- [✅/❌] Build validation passed
- [✅/❌] Runtime testing passed
- [✅/❌] Shader compilation passed (if applicable)

**Status:** [APPROVED / NEEDS CHANGES]
```

### 4. User Approval & Commit

Only after peer review approval.

## Failure Conditions

- **Any build errors** = immediate failure
- **Test.exe crashes** = immediate failure  
- **Shader compilation fails** = immediate failure
- **Missing peer review** = blocks commit

## Working Directory

Match VS Code settings: execute from Bin directory for builds/tests, Scripts directory for shaders.

## Standards References

- **Code Standards**: Documents/code-standards.md
- **Commit Messages**: Documents/commit-message-policy.md

No exceptions. Fix all issues before proceeding.

### Session Handoff

When user says "session handoff", UPDATE the "Session Handoff State" and "AI Agent Team Current Status" in the memory graph.
