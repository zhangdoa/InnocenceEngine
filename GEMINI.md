# InnocenceEngine — Gemini Instructions

## Project
- 8-year solo C++ game engine, active refactoring toward GPU-driven rendering and modern C++ standards
- Source: `Source/` — never touch `Source/External/`
- Build: `Build/` (RelWithDebInfo only)
- Run: `Bin/` — always invoke executables from this directory

## Standards
- Code conventions: `Documents/code-standards.md` — reference before every code change
- Commit format: `Documents/commit-message-policy.md` — reference before every commit

## Build & Test Commands
```bash
# Build — use the tracked script, never improvise a build command
powershell.exe -NoProfile -NonInteractive -File "./Scripts/BuildWin.ps1"

# Check build result
tail -5 Build/msbuild_out.txt
grep -i "error" Build/msbuild_out.txt | grep -v ZERO_CHECK

# Regression test — lightweight, single draw call, exits 0=pass, 1=GPU error, 2=crash
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'Bin'; (Start-Process -FilePath 'RelWithDebInfo/RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"

# Full integration test — loads GI scene, renders 10 frames, runs CPU ray tracer, exits 0=pass
# Use this for heavy changes (service refactors, resource management, render pipeline)
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'Bin'; (Start-Process -FilePath 'RelWithDebInfo/Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"

# Shader compilation
powershell.exe -File "./Scripts/HLSL2DXIL.ps1"

# CMake regeneration (needed after adding/removing source files)
cd Build && cmake .. && cd ..
```

**Why Scripts/BuildWin.ps1:** `cmd.exe /c msbuild` from git bash swallows output. Inline PowerShell `-Command` breaks on bash `$` expansion. The script lives in `Scripts/` (tracked) so it survives `git clean` and Build directory wipes.

**Testing policy:** Three tiers of testing:
1. **RenderTest** — regression test (single draw call). Use for any code change.
2. **Main.exe integration** — loads GI scene, renders 10 frames. Use for service refactors, resource management, render pipeline changes.
3. **Scene reload test** — loads GI scene, reloads UnitTest scene at specified frame, renders remaining frames. Catches device-removed crashes from in-flight resource destruction. Use for changes that affect scene lifecycle, GPU resource teardown, or deferred init.

```bash
# Scene reload test
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'Bin'; (Start-Process -FilePath 'RelWithDebInfo/Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10' -Wait -PassThru -NoNewWindow).ExitCode"
```

## Workflow
**Implementation -> Build -> Runtime test -> Shader test (if shaders changed) -> Peer review -> User approval**

Every step is mandatory. Any build error, RenderTest.exe crash, D3D12 validation error, or shader failure = stop and fix before proceeding.

## Skill Usage
| Situation | Skill |
|-----------|-------|
| Before any new feature or non-trivial change | `superpowers:brainstorming` |
| Planning multi-step implementation | `superpowers:writing-plans` -> `superpowers:executing-plans` |
| Bug or unexpected behavior | `superpowers:systematic-debugging` |
| After implementation, before merge | `superpowers:requesting-code-review` |
| Before claiming work is done | `superpowers:verification-before-completion` |

## Architecture Overview

### Service Pattern
The engine uses a service-oriented architecture. Services are registered as singletons accessed via `g_Engine->Get<ServiceType>()`. Each service implements `IService` (Setup/Initialize/Update/Terminate lifecycle).

### Per-Type Resource Services (recently refactored)
GPU resources are managed by 8 focused services, each with a base class and DX12 backend:

| Service | Owns | Deferred Init |
|---------|------|---------------|
| MeshResourceService | Mesh pool, GPUMeshResource slots | Yes |
| TextureResourceService | Texture pool, SRV/UAV, mipmaps | Yes |
| MaterialResourceService | Material pool | Yes |
| GPUBufferResourceService | GPU buffer pool, raytracing buffers | Yes |
| RenderPassResourceService | Render pass pool, PSO, semaphores, fences | Yes |
| ShaderProgramResourceService | Shader pool | No (sync) |
| SamplerResourceService | Sampler pool | No (sync) |
| CommandListResourceService | Command list pool | No (sync) |

Each service uses `NamedObjectPool<T>` for pool allocation with name-indexed dedup and live-object tracking. DX12 implementations are in `Source/Engine/Services/DX12/` and inherit from the base service.

### Key Patterns
- `NamedObjectPool<T>`: Wraps `TObjectPool<T>` + `ThreadSafeUnorderedMap` + `ThreadSafeVector`
- `DX12Context`: Shared struct owned by `DX12GraphicsHardwareService`, passed to each DX12 service via `SetDX12Context()`
- Deferred init: Services with deferred init queue work via `Initialize()` and drain per frame via `InitializeComponents()`
- `FrameManagementService`: Coordinates per-frame initialization and command execution
- `INNO_CLASS_INTERFACE_NON_COPYABLE`: Used on base service classes
- `INNO_CLASS_CONCRETE_NON_COPYABLE`: Used on DX12 derived classes

### Editor
The editor is a Qt-based application in `Source/Editor/`. It uses Qt widgets (QTreeWidget, QDockWidget, etc.) and communicates with the engine through the service layer.

## Core Principles
- Systemic — never patch locally; trace and fix root causes
- Expert quality — think before every change; no mediocre solutions
- No workarounds — low-quality patches are forbidden
- No assumptions — verify with tools; hallucination is a real risk
- Minimal cognitive complexity — code must be readable, not just correct
- No explanatory comments — only comment when the code itself is not obvious
- Validate everything — build and runtime test before any commit
- **UX-First Design:** Every UI change must be validated with an E2E test from a designer's perspective. Audit for contrast, readability, spacing, and clear feedback (e.g., progress indicators).
- Services own operation domains, not component types — a FooComponent does not imply a FooSystem; multiple services may operate on the same component type independently
- IPC Synchronization — Data must only be transferred or accessed when both the owner and the receiver are fully ready. A clear handshake/ready-signal must occur to prevent race conditions (e.g., transferring shared GPU textures before the receiving UI canvas is mounted).

## Workspace Hygiene
- Never produce scratch files in the repo root or any tracked directory
- Transient output (build logs, test captures) goes to `Build/` (gitignored) only
- Scripts belong in `Scripts/` (tracked) — never in `Build/`
- **Documentation Persistence:** Always commit brainstorming specs (`docs/superpowers/specs/`) and implementation plans (`docs/superpowers/plans/`) to the repository. These are valuable technical artifacts and must not be discarded.
- "Go ahead" means implement — do not ask follow-up questions

## Forbidden
- Direct STL includes — use engine wrappers (`STL14.h`, `STL17.h`)
- Raw `malloc`/`free`/`new[]` — use the engine memory system
- Inline functions that call engine APIs (`Log`, `g_Engine`) in headers
- `std::cout` — use engine logging
- Committing without a full test pass
- Touching `Source/External/`

<!-- BACKLOG.MD MCP GUIDELINES START -->

<CRITICAL_INSTRUCTION>

## BACKLOG WORKFLOW INSTRUCTIONS

This project uses Backlog.md MCP for all task and project management activities.

**CRITICAL GUIDANCE**

- If your client supports MCP resources, read `backlog://workflow/overview` to understand when and how to use Backlog for this project.
- If your client only supports tools or the above request fails, call `backlog.get_backlog_instructions()` to load the tool-oriented overview. Use the `instruction` selector when you need `task-creation`, `task-execution`, or `task-finalization`.

- **First time working here?** Read the overview resource IMMEDIATELY to learn the workflow
- **Already familiar?** You should have the overview cached ("## Backlog.md Overview (MCP)")
- **When to read it**: BEFORE creating tasks, or when you're unsure whether to track work

These guides cover:
- Decision framework for when to create tasks
- Search-first workflow to avoid duplicates
- Links to detailed guides for task creation, execution, and finalization
- MCP tools reference

You MUST read the overview resource to understand the complete workflow. The information is NOT summarized here.

</CRITICAL_INSTRUCTION>

<!-- BACKLOG.MD MCP GUIDELINES END -->
