# AI AGENT TEAM PEER REVIEW PROCESS
**Mandatory Quality Gate for AI-Generated Commits**

## Overview
All AI Agent Team commits must pass peer review before reaching user for final approval. This process enforces compliance with existing project standards and prevents policy violations.

## Workflow
```
Agent A (Creates Commit) → Agent B (Peer Review) → User (Final Approval) → Push
```

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
- **NO EXCEPTIONS**: All AI commits require peer approval before user review
- **BLOCKING**: Any non-compliance blocks approval until resolved
- **ACCOUNTABILITY**: Both creator and reviewer are responsible for quality

This process ensures all existing project standards are maintained without duplicating their content.
