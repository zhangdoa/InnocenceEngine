# InnocenceEngine — Claude Instructions

## Project
- 8-year solo C++ game engine, active refactoring toward GPU-driven rendering and modern C++ standards
- Source: `C:\GitRepo\InnocenceEngine\Source\` — never touch `Source\External\`
- Build: `C:\GitRepo\InnocenceEngine\Build\` (RelWithDebInfo only)
- Run: `C:\GitRepo\InnocenceEngine\Bin\` — always invoke executables from this directory

## Standards
- Code conventions: `Documents\code-standards.md` — reference before every code change
- Commit format: `Documents\commit-message-policy.md` — reference before every commit

## Build & Test Commands
```
# Build
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo" 2>&1

# Runtime test (headless)
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1

# GPU validation — autonomous test, exits 0=pass, 1=GPU error, 2=crash (preferred over Test.exe when rendering is touched)
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1

# Shader compilation
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\HLSL2DXIL.ps1" 2>&1
```

## Workflow
**Implementation → Build → Runtime test → Shader test (if shaders changed) → Peer review → User approval**

Every step is mandatory. Any build error, Test.exe crash, D3D12 validation error, or shader failure = stop and fix before proceeding.

## Skill Usage
| Situation | Skill |
|-----------|-------|
| Before any new feature or non-trivial change | `superpowers:brainstorming` |
| Planning multi-step implementation | `superpowers:writing-plans` → `superpowers:executing-plans` |
| Bug or unexpected behavior | `superpowers:systematic-debugging` |
| Any third-party SDK or API | `context7` MCP — never assume behavior |
| After implementation, before merge | `superpowers:requesting-code-review` |
| Before claiming work is done | `superpowers:verification-before-completion` |
| 2+ independent tasks that can run in parallel | `superpowers:dispatching-parallel-agents` |

## Core Principles
- Systemic — never patch locally; trace and fix root causes
- Expert quality — think before every change; no mediocre solutions
- No workarounds — low-quality patches are forbidden
- No assumptions — verify with tools; hallucination is a real risk
- Minimal cognitive complexity — code must be readable, not just correct
- No explanatory comments — only comment when the code itself is not obvious
- Validate everything — build and runtime test before any commit

## Forbidden
- Direct STL includes — use engine wrappers (`STL14.h`, `STL17.h`)
- Raw `malloc`/`free`/`new[]` — use the engine memory system
- Inline functions that call engine APIs (`Log`, `g_Engine`) in headers
- `std::cout` — use engine logging
- Committing without a full test pass
- Touching `Source\External\`
