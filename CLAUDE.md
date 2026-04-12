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

**Testing policy:** Four tiers of testing:
1. **RenderTest** — regression test (single draw call). Use for any code change.
2. **Main.exe integration** — loads GI scene, renders 10 frames. Use for service refactors, resource management, render pipeline changes.
3. **Scene reload test** — loads GI scene, reloads UnitTest scene at specified frame, renders remaining frames. Catches device-removed crashes from in-flight resource destruction. Use for changes that affect scene lifecycle, GPU resource teardown, or deferred init.
4. **Interactive test** — launches Main.exe windowed and sends keystrokes via Win32 PostMessage to exercise runtime toggles (path tracer on/off, scene reload, camera movement). Catches crashes that only occur in windowed mode with user interaction. Use for rendering pipeline changes, HID-triggered features, or any toggle/mode-switch logic.

```
# Scene reload test
powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10' -Wait -PassThru -NoNewWindow).ExitCode"

# Interactive test — full scenario (camera + path tracer toggle + scene reload)
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/InteractiveTest.ps1" -Scenario full

# Interactive test — specific scenarios: toggle_pathtracer, scene_reload, camera_movement, pathtracer_reload
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/InteractiveTest.ps1" -Scenario toggle_pathtracer
```

## RenderDoc
RenderDoc is at `C:/Program Files/RenderDoc/renderdoccmd.exe`. The engine has built-in in-process RenderDoc API (loaded via renderdoc.dll injection). Use `-capture_frame N` to trigger a capture; the .rdc file goes to `Build/captures/`.

The auto-test scene schedule in World.inl:
- Frame 5: GISponza loads (good frame range for capturing GI scene: 6–9)
- Frame 10: UnitTest reloads
- Frame 20: engine exits

**E2E visual verification workflow** — fully autonomous, no human needed:
```
# Step 1: Create captures dir, run capture (GISponza visible at frame 8)
mkdir -p C:/GitRepo/InnocenceEngine/Build/captures
"C:/Program Files/RenderDoc/renderdoccmd.exe" capture -w \
  -d "C:/GitRepo/InnocenceEngine/Bin" \
  -c "C:/GitRepo/InnocenceEngine/Build/captures/frame" \
  "C:/GitRepo/InnocenceEngine/Bin/RelWithDebInfo/Main.exe" \
  "-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -capture_frame 8"

# Step 2: Find the .rdc file (name is template + 8-digit frame number)
ls C:/GitRepo/InnocenceEngine/Build/captures/

# Step 3: Extract thumbnail to PNG
"C:/Program Files/RenderDoc/renderdoccmd.exe" thumb \
  --out="C:/GitRepo/InnocenceEngine/Build/captures/frame8.png" \
  "C:/GitRepo/InnocenceEngine/Build/captures/frame800000000.rdc"

# Step 4: View the thumbnail — use the Read tool on the .png file
# Claude can view images and assess whether textures/lighting look correct.
```

**SOP: When to use RenderDoc instead of asking the user**
- Any visual rendering change (textures, lighting, shadows, materials) — capture + thumb + Read to verify
- Shader changes — capture before and after, compare thumbnails
- Never ask "does it look correct?" — run the capture and check yourself

## Workflow
**Implementation → Build → Runtime test → Shader test (if shaders changed) → Peer review → User approval**

Every step is mandatory. Any build error, Test.exe crash, D3D12 validation error, or shader failure = stop and fix before proceeding.

## Regression Debugging Policy
When a regression is found, do NOT guess at the cause or add speculative fixes. Instead:
1. Reset to the last known working commit and verify the feature works there (e.g. `git stash && git checkout <known-good-sha>`)
2. Apply changes incrementally (commit-by-commit or file-by-file) from the working baseline toward HEAD
3. Build and test after each incremental step to isolate the exact change that introduced the regression
4. Only after identifying the offending change, analyze and fix the root cause

## Merge Policy
Never blindly merge branches or accept incoming changes. Always:
1. Review every incoming change for correctness and compatibility with the current codebase
2. Build and run the full test suite after merging — treat a merge like any other code change
3. If tests fail post-merge, bisect the merged commits to find the offending change before attempting fixes

## Bug Vigilance
Be vigilant about any unexpected behavior, warnings, or anomalies during testing. When you observe a potential bug — even if it seems minor or intermittent — backlog it immediately so it is tracked and not forgotten.

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
