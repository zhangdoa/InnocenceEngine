# InnocenceEngine — Claude Instructions

Project-scoped only. Meta engineering/behavioral rules live in the user-scope `CLAUDE.md`.

## What

- 8-year solo C++ game engine, active refactoring toward GPU-driven rendering and modern C++
- Source: `C:\GitRepo\InnocenceEngine\Source\` — never touch `Source\External\`
- Build: `C:\GitRepo\InnocenceEngine\Build\` (RelWithDebInfo only, gitignored)
- Run: `C:\GitRepo\InnocenceEngine\Bin\` — always invoke executables from this directory
- Standards: `Documents\code-standards.md` (before every coding task) and `Documents\commit-message-policy.md` (before every commit)

### Workspace hygiene

- No scratch files in the repo root or any tracked directory; transient output (build logs, test captures) goes to `Build/` only
- Scripts belong in `Scripts/` (tracked) — never in `Build/`
- "Go ahead" means implement — do not ask follow-up questions

### Project-level forbidden (not covered by code standards)

- Committing without a real integration-test run (enforced by `.claude/hooks/commit-gate.js`; see Harness enforcement)
- Touching `Source\External\`
- Creating any new documentation file (`*.md`, `README`, design doc, roadmap, spec, architecture note, etc.) under `Documents/`, the repo root, or any tracked directory without explicit user request. "Explicit request" = the user typed something like "create a doc at …" or "write a spec for …"; inferring that a doc *would be useful* is not authorization. The system-prompt default ("NEVER create documentation files unless explicitly requested") is not overridden by this project — reinforced here.

### AI authorship scope — what the AI may and may not author

The backlog is the only AI-authored project-state medium. Anything longer-lived than conversation context goes into backlog task files. The AI does not invent new tracked docs.

**AI may author:**

- Source files under `Source/` (except `Source/External/`) as part of implementing a task
- Backlog task files under `.backlog/tasks/**` (creation, edit, move to `completed/`) per the backlog workflow
- Edits to `CLAUDE.md`, `.claude/**`, and `.backlog/**` when the user explicitly asks to codify a rule or update workflow
- Transient scratch files under `Build/**` (gitignored) — commit messages, logs, captures

**AI may NOT author, without explicit user request:**

- Any new `*.md` under `Documents/`, the repo root, or any tracked directory
- `README.md`, design specs, roadmaps, architecture notes, runbooks, migration guides, release notes
- Edits to *existing* `Documents/*.md` files not owned by the AI. The two AI-editable docs in `Documents/` are `code-standards.md` and `commit-message-policy.md`, and only when the user asks to update them.

If long-lived cross-session context is needed (priority order, next-up slice, design rationale, partial progress), it goes in the owning backlog task's `## Implementation Notes` section — the backlog already survives sessions and is the supported hand-off mechanism. Standalone roadmap/plan docs are not.

Existing `Documents/*.md` files that were AI-created without explicit authorization (e.g. `Documents/radiance-cache-roadmap.md`) are flagged for user decision: keep as user-owned, migrate content into the owning backlog task and delete, or split. The AI does not unilaterally delete or migrate them.

## Harness enforcement

`.claude/settings.json` wires a PreToolUse hook (`.claude/hooks/commit-gate.js`) that blocks `git commit` unless BOTH gates pass:

**Test-run gate** — one of:

- A qualifying integration test ran in the current turn — `npx playwright test` (editor), `Main.exe` with frame flags (engine), `RenderTest.exe -test`, `InteractiveTest.ps1`, or `Main.exe -serialize_test`
- The staged set is docs-only — `.backlog/`, `Documents/`, `*.md`, `.claude/`
- The commit message contains `[skip-test-gate]` — use this only when the commit genuinely cannot be validated by a test (commit-message edit, hook fix, etc.)

**Serialize-determinism gate** — additionally required when `JSONWrapper/`, `AssetService.*`, or `SceneService.*` are staged: `Main.exe -serialize_test ExampleProject/Scenes/UnitTest.InnoScene` must have run in the current turn.

**Attribution gate** — the commit message (from `-m` or `-F`) must contain `Code-AI-Generated-By:` or `Message-AI-Generated-By:` per `Documents/commit-message-policy.md`. No escape; every Claude-issued commit is AI-authored by definition.

Commit-message drafts go in `Build/commit-message.txt` (gitignored), not in a tracked scratch folder. The hook fails open on internal errors so a broken hook never bricks commits.

## How

### Build

```
# Use the tracked script — never improvise.
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK

# Shader compilation
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\HLSL2DXIL.ps1"

# clangd index — run when CMakeLists.txt or include layout changes, or when
# clangd diagnostics start citing missing headers / undeclared identifiers
# that the MSBuild build resolves fine. Emits compile_commands.json at the
# repo root; clangd auto-discovers it on next reload.
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\RegenClangdIndex.ps1"
```

**Trust real diagnostics, fix false ones at the layer that produces them.**
If clangd reports `<string_view> not found` or `nlohmann/json.hpp not found`
on a file MSBuild compiles cleanly, the index is stale — rerun the regen
script. Don't dismiss the diagnostic, don't filter it; the cost of learning
to ignore loud signals is missing the real one later.

**Always launch the build with `run_in_background: true`** — the Bash tool then auto-fires a completion notification instead of blocking with a timeout. Don't guess a timeout value, don't poll with sleep loops, don't use `TaskOutput(block=true)`. Do unrelated work while it runs (e.g. read code, draft the next edit). When the notification arrives, grep the log for errors. Same pattern applies to `HLSL2DXIL.ps1` and any long runtime test.

**Why `Scripts/BuildWin.ps1`:** `cmd.exe /c msbuild` from git bash swallows output; inline PowerShell `-Command` breaks on bash `$` expansion; `/t:Main` on the `.sln` targets a folder, not a project. The script lives in `Scripts/` (tracked) so it survives `git clean` and Build wipes.

### Test tiers — pick the lightest that covers the change

1. **RenderTest** — single draw call regression. Use for *any* code change. Exit 0=pass, 1=GPU error, 2=crash.

   ```
   powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode"
   ```

2. **Main.exe integration** — loads GI scene, 10 frames, runs CPU ray tracer. Use for service refactors, resource management, render pipeline.

   ```
   powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 10' -Wait -PassThru -NoNewWindow).ExitCode"
   ```

3. **Scene reload** — reloads UnitTest scene at a chosen frame; catches device-removed crashes from in-flight resource destruction, stale descriptors, and multi-load accumulation bugs. Use for **any** change touching asset loading, scene lifecycle, GPU resource management, deferred initialization, or any state that persists across scene boundaries (shared asset handles, component pools, descriptor heaps). The 10-frame single-load tier (tier 2) can silently green-light changes that only fail on the second or later load.

   ```
   powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -reload_at_frame 10' -Wait -PassThru -NoNewWindow).ExitCode"
   ```

4. **Interactive** — windowed Main.exe driven by Win32 PostMessage keystrokes. Catches crashes that only surface in windowed/user-interaction mode.

   ```
   powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/InteractiveTest.ps1" -Scenario full
   # Scenarios: toggle_pathtracer, scene_reload, camera_movement, pathtracer_reload, full
   ```

5. **Serialize-determinism** — loads a scene, saves it in-place, compares saved state against the pre-save snapshot, restores originals, exits 0 if idempotent or 1 if any file changed. **Required** whenever `JSONWrapper/`, `AssetService.*`, or `SceneService.*` are staged (enforced by `.claude/hooks/commit-gate.js`). Also run manually after any scene-structure or component-serializer change.

   ```
   powershell.exe -NoProfile -NonInteractive -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\Main.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -serialize_test ExampleProject/Scenes/UnitTest.InnoScene' -Wait -PassThru -NoNewWindow).ExitCode"
   ```

### GPU validation

Launch with `-gpu_validation` to enable D3D12 GPU-based validation + synchronized command queue validation (works even in `-offscreen`). Use when chasing silent corruption or suspected barrier/layout/resource-state bugs — validation errors surface at the point of violation instead of manifesting frames later.

**Known imprecision:** Release-compiled shaders trigger "Shader Patch Mode NONE" in GBV, producing imprecise tracking. Symptom: errors citing `Layout: UNKNOWN (N)` with `N` past `D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON` (30) — out-of-enum values are validator sentinels, not real states. To confirm real vs. false-positive, rebuild shaders with `/Zi /Od` or inspect the enum bound (`External/GitSubmodules/DirectX-Headers/include/directx/d3d12.h`). See TASK-37.

### RenderDoc (in-process)

`renderdoccmd.exe` is at `C:/Program Files/RenderDoc/renderdoccmd.exe`. The engine has an in-process RenderDoc API loaded via `renderdoc.dll` injection. Use `-capture_frame N` to trigger a capture; the `.rdc` goes to `Build/captures/`.

Auto-test scene schedule (`World.inl`):

- Frame 5: GISponza loads (good capture range: 6–9)
- Frame 10: UnitTest reloads
- Frame 20: engine exits

**E2E visual verification — fully autonomous, no human needed:**

```
mkdir -p C:/GitRepo/InnocenceEngine/Build/captures

# 1. Capture frame 8 (GISponza visible)
"C:/Program Files/RenderDoc/renderdoccmd.exe" capture -w \
  -d "C:/GitRepo/InnocenceEngine/Bin" \
  -c "C:/GitRepo/InnocenceEngine/Build/captures/frame" \
  "C:/GitRepo/InnocenceEngine/Bin/RelWithDebInfo/Main.exe" \
  "-mode 0 -renderer 0 -loglevel 0 -offscreen -total_frames 20 -capture_frame 8"

# 2. Locate .rdc (name = template + 8-digit frame number)
ls C:/GitRepo/InnocenceEngine/Build/captures/

# 3. Extract thumbnail
"C:/Program Files/RenderDoc/renderdoccmd.exe" thumb \
  --out="C:/GitRepo/InnocenceEngine/Build/captures/frame8.png" \
  "C:/GitRepo/InnocenceEngine/Build/captures/frame800000000.rdc"

# 4. View with the Read tool (images are visible to Claude)
```

**SOP — use RenderDoc instead of asking the user:**

- Any visual rendering change (textures, lighting, shadows, materials): capture + thumb + Read to verify
- Shader changes: capture before and after, compare thumbnails
- Never ask "does it look correct?" — run the capture and check yourself

### Workflow — task-first approach

Never rogue-fix. File a backlog task *before* starting the fix so the work is tracked, then follow the loop:

1. **Discover** — reproduce; identify the failing resource/path/assumption
2. **Test** — add or pick a test that exhibits the bug
3. **Narrow** — isolate the minimal change or code path that causes it
4. **Fix** — address the root cause; prefer structural fixes over local patches
5. **Validate** — rerun the relevant test tier(s); zero validation errors required
6. **Commit** — atomic commit per `Documents/commit-message-policy.md`

### Workflow — regression debugging

When a regression is found, do NOT guess. Instead:

1. Reset to the last known-good commit and verify the feature works there (`git stash && git checkout <sha>`)
2. Apply changes incrementally (commit-by-commit or file-by-file) toward HEAD
3. Build and test after each step to isolate the offending change
4. Only then analyze and fix the root cause

### Workflow — commit granularity

Commits must be atomic and logically self-contained:

- One commit per distinct concern: engine code / data / scripts-or-tooling
- No unrelated fixes bundled together
- Each commit must build and pass RenderTest independently (no broken-state commits)
- Typical splits: `feat: engine code` / `data: scene and component files` / `chore: scripts and tooling`
- Backlog task changes (creation, edit, move to `completed/`) must be committed — never leave task-file edits uncommitted across sessions. Commit them as their own `docs(backlog)` commit (docs-only, skips the test-run gate) or bundle with the code CL that triggered the status change.

### Workflow — sync with remote

Pull and rebase occasionally to stay current with the remote branch. Do this before starting a new task or feature, before committing if the session has been long, or after completing a logical unit of work.

```bash
git fetch origin
git stash  # if there are uncommitted changes
git rebase origin/ecs-overhaul
git stash pop  # restore changes
```

If rebase fails due to file locks (common when VS or other tools hold files open), abort and retry later — do not force or discard work.

### Workflow — merge policy

Never blindly merge or accept incoming changes:

1. Review every incoming change for correctness and compatibility
2. Build and run the full test suite after merging — treat a merge like any other code change
3. If tests fail post-merge, bisect the merged commits to find the offender before fixing

### Workflow — bug vigilance

Any unexpected behavior, warning, or anomaly observed during testing — even minor or intermittent — gets backlogged immediately so it is tracked and not forgotten.

### Workflow — cross-session continuity

Conversation context does not survive session boundaries. Anything a future session needs to pick up work must live in a tracked file — **not** in commit messages, not in recent-memory narrative, not in conversation scrollback.

**The only AI-owned cross-session medium is the backlog task file.** The AI does not create roadmap docs, design docs, or any other `Documents/*.md` to carry state (see "AI authorship scope" above).

**End of every landing CL (on a multi-session task):**

1. Update the owning task's `## Implementation Notes` with: what landed, what's deferred, what's next, and the ordered list of remaining slices. Leave `status: In Progress` if more of the umbrella remains.
2. If sub-slices exist as their own tasks, also update their status (to `Done`, or to `In Progress` if you're taking the next one now).
3. Commit the task changes (see commit-granularity rule).

**Start of every session:**

1. Before "continuing", list `.backlog/tasks/` entries with `status: In Progress` (Backlog MCP `task_list`, or `rg '^status: In Progress' .backlog/tasks/*.md`). Read each.
2. Read referenced sub-task files and the parent task's Implementation Notes. That is the session hand-off.
3. Never rely on prior-conversation memory; never infer "what's next" from the last commit subject. If a user-owned doc under `Documents/` is referenced, it is background, not the hand-off.

**Forbidden:** recording "next step", "deferred to next CL", or priority order only in commit messages, chat, or an AI-authored `Documents/*.md`. Commit messages describe what landed; the backlog task file describes what's next.

**Rule of thumb — systemic vs. local:** when a cross-cutting concern surfaces (cross-session state, silent failures, forbidden patterns, etc.), the fix belongs in a document that governs *every* future occurrence (this file, a shared policy doc, or a code-level invariant). Fixing only the instance the user just pointed at is the local-not-systemic antipattern.

### Skills & Tools

| Situation | Skill |
|-----------|-------|
| Before any new feature or non-trivial change | `superpowers:brainstorming` |
| Planning multi-step implementation | `superpowers:writing-plans` → `superpowers:executing-plans` |
| Bug or unexpected behavior | `superpowers:systematic-debugging` |
| Any third-party SDK or API | `context7` MCP — never assume behavior |
| After implementation, before merge | `superpowers:requesting-code-review` |
| Before claiming work is done | `superpowers:verification-before-completion` |
| 2+ independent tasks that can run in parallel | `superpowers:dispatching-parallel-agents` |

<!-- BACKLOG.MD MCP GUIDELINES START -->

<CRITICAL_INSTRUCTION>

### Backlog workflow (Backlog.md MCP)

This project uses Backlog.md MCP for all task and project management activities.

- If your client supports MCP resources, read `backlog://workflow/overview` to understand when and how to use Backlog for this project.
- If your client only supports tools or the above request fails, call `backlog.get_backlog_instructions()` to load the tool-oriented overview. Use the `instruction` selector when you need `task-creation`, `task-execution`, or `task-finalization`.

- **First time working here?** Read the overview resource IMMEDIATELY to learn the workflow
- **Already familiar?** You should have the overview cached ("## Backlog.md Overview (MCP)")
- **When to read it**: BEFORE creating tasks, or when you're unsure whether to track work

These guides cover: decision framework for when to create tasks, search-first workflow to avoid duplicates, links to detailed guides for task creation, execution, and finalization, and MCP tools reference.

You MUST read the overview resource to understand the complete workflow. The information is NOT summarized here.

</CRITICAL_INSTRUCTION>

<!-- BACKLOG.MD MCP GUIDELINES END -->

## Project-specific principles

- **Comments describe present state, never history** — the codebase is not a changelog. Banned: "now via X / used to be Y / replaces / subsumes / retires / migrated from / deleted alongside / first consumer lands in next commit / added in <sha> / before this fix / fixed in TASK-NN". Git log + commit messages + backlog tasks are the version control; the source file describes only what is true today. Why-comments are fine when they document a present invariant ("setter must be lock-free; AllToggles holds the mutex"); they are not fine when they explain how the code got here.
- **Services own operation domains, not component types** — a `FooComponent` does not imply a `FooSystem`; multiple services may operate on the same component type independently.
- **Fix at the right layer** — a high-level concern gets a high-level fix. A generic container, pool, or base service must not carry knowledge of a caller's naming conventions, project paths, enum values, scene assumptions, or named downstream consumers. Same rule applies to comments: each layer's documentation names its own concepts, not the consumers that happen to depend on it. When the instinct is "add the check / cast / special case in the foundation class", step up a layer and adjust the owner instead.

## Target qualities (aim every CL at these)

- **Orthogonality** — each service/module has one responsibility; changes in one place don't silently affect another
- **Explicit contracts** — preconditions, postconditions, and ownership are enforced (types, assertions, documented invariants), not assumed
- **Fail loudly** — invalid state produces an immediate, visible error at the point of violation, not silent corruption three frames later
- **Reload-safe by default** — any resource or asset loadable more than once must handle re-initialization without accumulating stale state

### Structural improvement after every CL

After every non-trivial changelist, do a brief retrospective and file a backlog task for each structural finding. Do not leave structural observations as conversation.

1. What implicit contract was violated? — identify the unenforced assumption
2. What structural weakness allowed it?
3. What improvement moves the engine toward the target qualities above?
