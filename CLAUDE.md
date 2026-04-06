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
# Build — use the tracked script, never improvise a build command
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"

# Check build result
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK

# Regression test — lightweight, single draw call, exits 0=pass, 1=GPU error, 2=crash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"

# Full integration test — loads GI scene, renders 10 frames, runs CPU ray tracer, exits 0=pass
# Use this for heavy changes (service refactors, resource management, render pipeline)
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"

# Shader compilation
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\HLSL2DXIL.ps1"
```

**Why Scripts/BuildWin.ps1:** `cmd.exe /c msbuild` from git bash swallows output. Inline PowerShell `-Command` breaks on bash `$` expansion. `/t:Main` on the `.sln` targets a folder, not a project. The script lives in `Scripts/` (tracked) so it survives `git clean` and Build directory wipes.

**Testing policy:** Three tiers of testing:
1. **RenderTest** — regression test (single draw call). Use for any code change.
2. **Main.exe integration** — loads GI scene, renders 10 frames. Use for service refactors, resource management, render pipeline changes.
3. **Scene reload test** — loads GI scene, reloads UnitTest scene at specified frame, renders remaining frames. Catches device-removed crashes from in-flight resource destruction. Use for changes that affect scene lifecycle, GPU resource teardown, or deferred init.

```
# Scene reload test
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10' -Wait -PassThru -NoNewWindow).ExitCode"
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
- Services own operation domains, not component types — a FooComponent does not imply a FooSystem; multiple services may operate on the same component type independently

## Workspace Hygiene
- Never produce scratch files in the repo root or any tracked directory
- Transient output (build logs, test captures) goes to `Build/` (gitignored) only
- Scripts belong in `Scripts/` (tracked) — never in `Build/`
- "Go ahead" means implement — do not ask follow-up questions

## Forbidden
- Direct STL includes — use engine wrappers (`STL14.h`, `STL17.h`)
- Raw `malloc`/`free`/`new[]` — use the engine memory system
- Inline functions that call engine APIs (`Log`, `g_Engine`) in headers
- `std::cout` — use engine logging
- Committing without a full test pass
- Touching `Source\External\`
