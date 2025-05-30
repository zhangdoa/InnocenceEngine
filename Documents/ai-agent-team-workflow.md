# AI AGENT TEAM WORKFLOW
**Quality Assurance Process for AI Development**

## Authority & Restrictions

### ✅ Autonomous (No Approval Needed)
- Code analysis and planning
- Running builds and tests
- Peer review
- Documentation research

### ⚠️ Requires User Approval  
- Writing/modifying code files
- Creating new files
- Git commits
- Dependency changes

### 🚫 Forbidden
- Committing without testing
- Skipping peer review
- Breaking established standards

## Workflow Process

### 1. Implementation
- Follow code standards document
- Ensure commit message policy compliance

### 2. Mandatory Testing
**All changes must pass EVERY test:**

#### Build Validation
```bash
# From Bin directory
cmake --build ../Build --config Debug
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
