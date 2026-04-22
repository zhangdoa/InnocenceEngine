# InnocenceEngine — Claude Instructions

Project-scoped only. Meta/behavioral rules (working principles, pipeline, communication, end-of-response checklist, skills table) live in the user-scope `CLAUDE.md`.

## What

- 8-year solo C++ game engine; active refactoring toward GPU-driven rendering and modern C++.
- Source: `C:\GitRepo\InnocenceEngine\Source\` — never touch `Source\External\`.
- Build: `C:\GitRepo\InnocenceEngine\Build\` (RelWithDebInfo only, gitignored).
- Run: `C:\GitRepo\InnocenceEngine\Bin\` — invoke executables from this directory.
- Standards: `Documents\code-standards.md` (before every coding task) and `Documents\commit-message-policy.md` (before every commit).

## Workspace hygiene

- No scratch files in the repo root or any tracked directory; transient output (build logs, captures) goes to `Build/` only.
- Scripts belong in `Scripts/` (tracked) — never in `Build/`.

## Project-level forbidden

- Committing without a real integration-test run (enforced by `.claude/hooks/commit-gate.js`; see Harness enforcement).
- Touching `Source\External\`.
- Creating any new documentation file (`*.md`, `README`, design doc, roadmap, spec, architecture note, etc.) under `Documents/`, the repo root, or any tracked directory without explicit user request. "Explicit request" = the user typed something like "create a doc at …"; inferring usefulness is not authorization. Reinforces the system-prompt default.

## AI authorship scope

The backlog is the only AI-authored project-state medium. Cross-session context goes in backlog task `## Implementation Notes`, not a standalone doc.

**AI may author:** `Source/` (except `External/`); `.backlog/**`; edits to `CLAUDE.md` / `.claude/**` / `.backlog/**` on explicit user request; scratch under `Build/**` (gitignored).

**AI may NOT author without explicit request:** any new `*.md` under `Documents/`, the repo root, or any tracked directory; `README`, design/architecture/roadmap/runbook docs; edits to existing `Documents/*.md` files (only `code-standards.md` and `commit-message-policy.md` are editable, on request).

Existing AI-created tracked docs are flagged for user decision, not unilaterally migrated.

## Harness enforcement

`.claude/settings.json` wires a PreToolUse hook (`.claude/hooks/commit-gate.js`) that blocks `git commit` unless all three gates pass.

**Test-run gate** — one of:
- A qualifying integration test ran this turn — `Main.exe` with frame flags, `RenderTest.exe -test`, `InteractiveTest.ps1`, or `Main.exe -serialize_test`.
- Staged set is docs-only (`.backlog/`, `Documents/`, `*.md`, `.claude/`).
- Message contains `[skip-test-gate]` — use only when the commit genuinely cannot be validated by a test (commit-message edit, hook fix, docs migration, etc.).

**Serialize-determinism gate** — when `JSONWrapper/`, `AssetService.*`, or `SceneService.*` are staged, `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` must have run this turn.

**Attribution gate** — commit message must contain `Code-AI-Generated-By:` or `Message-AI-Generated-By:` per `Documents/commit-message-policy.md`.

Drafts go in `Build/commit-message.txt` (gitignored). Hook fails open on internal errors.

## Build

```
# Use the tracked script — never improvise.
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK

# Shader compilation
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\HLSL2DXIL.ps1"

# clangd index — rerun when CMakeLists.txt / include layout changes, or
# diagnostics cite missing headers that MSBuild compiles cleanly.
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\RegenClangdIndex.ps1"
```

**Always launch with `run_in_background: true`** — Bash fires a completion notification; don't guess timeouts, don't poll, don't `TaskOutput(block=true)`. Same pattern for `HLSL2DXIL.ps1` and any long runtime test.

**Trust real diagnostics, fix false ones at source.** If clangd cites missing headers MSBuild resolves cleanly, the index is stale — regen, don't filter.

**Why `Scripts/BuildWin.ps1`:** `cmd.exe /c msbuild` from git bash swallows output; inline PowerShell `-Command` breaks on bash `$` expansion; `/t:Main` on the `.sln` targets a folder. Script lives in `Scripts/` so it survives `git clean`.

## Test tiers — pick the lightest that covers the change

All via `powershell.exe -NoProfile -NonInteractive`, from `C:\GitRepo\InnocenceEngine\Bin`.

1. **RenderTest** — single draw call regression. Use for any code change. Exit 0=pass, 1=GPU error, 2=crash.
   `RelWithDebInfo\RenderTest.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced`

2. **Main.exe integration** — GI scene, 10 frames, CPU ray tracer. Use for service refactors, resource management, render pipeline.
   `RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10`

3. **Scene reload** — reloads UnitTest at frame 10. Use for *any* change touching asset loading, scene lifecycle, GPU resource management, deferred init, or state that persists across scene boundaries. Tier 2 can green-light changes that only fail on the second-or-later load.
   `RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10`

4. **Interactive** — windowed, Win32 PostMessage keystrokes. Catches windowed-only crashes.
   `powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/InteractiveTest.ps1" -Scenario full`
   Scenarios: `toggle_pathtracer`, `scene_reload`, `camera_movement`, `pathtracer_reload`, `full`.

5. **Serialize-determinism** — load → save → diff. Required when `JSONWrapper/`, `AssetService.*`, `SceneService.*` are staged (hook-enforced).
   `RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -serialize_test ExampleProject/Scenes/UnitTest.InnoScene`

## GPU validation

`-gpu_validation` enables D3D12 GPU-based validation + synchronized command queue validation (works in `-offscreen`). Use when chasing silent corruption or barrier/layout/resource-state bugs — errors surface at the violation point.

**Known imprecision:** Release shaders trigger "Shader Patch Mode NONE" in GBV — `Layout: UNKNOWN (N)` with N > `D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON` (30) are validator sentinels, not real states. Rebuild shaders with `/Zi /Od` or inspect the enum bound (`External/GitSubmodules/DirectX-Headers/include/directx/d3d12.h`) to confirm false-positive vs real. See TASK-37.

## RenderDoc (in-process)

`renderdoccmd.exe` at `C:/Program Files/RenderDoc/renderdoccmd.exe`. Engine loads `renderdoc.dll` via injection; `-capture_frame N` writes a `.rdc` to `Build/captures/`.

Auto-test scene schedule (`World.inl`): frame 5 GISponza load request, frame 10 UnitTest reload, frame 20 exit. Async streaming takes ~30–40 frames; use `-total_frames 100 -capture_frame 60` for a fully-loaded capture. Frames before ~50 render a black G-buffer.

**E2E visual verification (autonomous — no human needed):**

```
mkdir -p C:/GitRepo/InnocenceEngine/Build/captures
"C:/Program Files/RenderDoc/renderdoccmd.exe" capture -w \
  -d "C:/GitRepo/InnocenceEngine/Bin" \
  -c "C:/GitRepo/InnocenceEngine/Build/captures/frame" \
  "C:/GitRepo/InnocenceEngine/Bin/RelWithDebInfo/Main.exe" \
  "-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 100 -capture_frame 60"
"C:/Program Files/RenderDoc/renderdoccmd.exe" thumb \
  --out="C:/GitRepo/InnocenceEngine/Build/captures/<label>.png" \
  "C:/GitRepo/InnocenceEngine/Build/captures/frame<frame>.rdc"
# View the PNG with the Read tool — images are visible.
```

**SOP:** any visual rendering change — capture, thumb, Read. Shader changes — capture before and after, compare. Never ask "does it look correct?" — capture and check.

## Workflow — cross-session continuity

Conversation context does not survive session boundaries. Everything a future session needs to pick up work must live in a tracked file.

**The only AI-owned cross-session medium is the backlog task file.** No roadmap/design/plan docs under `Documents/`.

**End of every landing CL on a multi-session task:**
1. Update the owning task's `## Implementation Notes`: what landed, what's deferred, what's next, remaining-slice priority order. Leave `status: In Progress` if the umbrella has more.
2. Update sub-slice task statuses (Done, or In Progress if taking the next one now).
3. Commit the task changes. Backlog task changes must never be left uncommitted across sessions — commit as `docs(backlog)` (docs-only, skips the test-run gate) or bundle with the code CL.

**Start of every session:**
1. List `.backlog/tasks/` with `status: In Progress` (Backlog MCP `task_list`, or `rg '^status: In Progress' .backlog/tasks/*.md`). Read each.
2. Read referenced sub-task files and the parent task's Implementation Notes — that is the hand-off.
3. Never infer "what's next" from commit subjects or prior-conversation memory.

**Forbidden:** recording priority order / next-step only in commit messages, chat, or AI-authored `Documents/*.md`.

## Backlog workflow (Backlog.md MCP)

This project uses Backlog.md MCP for all task and project management. Read `backlog://workflow/overview` (or call `backlog.get_backlog_instructions()`) for the complete workflow — decision framework, search-first-to-avoid-duplicates, and detailed guides for creation/execution/finalization. Read it the first time in a new session and before creating tasks.

## Project-specific principles

- **Comments describe present state, never history** — the codebase is not a changelog. Banned phrases: "now via X / used to be Y / replaces / subsumes / retires / migrated from / deleted alongside / first consumer lands in next commit / added in <sha> / before this fix / fixed in TASK-NN". Why-comments are fine when documenting a present invariant ("setter must be lock-free; AllToggles holds the mutex"); not when explaining how the code got here.
- **Services own operation domains, not component types** — a `FooComponent` does not imply a `FooSystem`; multiple services may operate on the same component type independently.

## Target qualities

Aim every CL at these:

- **Orthogonality** — each service/module has one responsibility; changes in one place don't silently affect another.
- **Explicit contracts** — preconditions, postconditions, and ownership enforced (types, assertions, invariants), not assumed.
- **Fail loudly** — invalid state produces an immediate, visible error at the violation point, not silent corruption frames later.
- **Reload-safe by default** — any resource or asset loadable more than once handles re-initialization without accumulating stale state.

### Structural retrospective after every CL

After every non-trivial changelist, brief retrospective; file a backlog task for each structural finding. Never leave structural observations as conversation.

1. What implicit contract was violated?
2. What structural weakness allowed it?
3. What improvement moves the engine toward the target qualities above?
