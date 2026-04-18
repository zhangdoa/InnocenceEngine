# InnocenceEngine — Claude Instructions

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

- Committing without a full test pass
- Touching `Source\External\`

## How

### Build

```
# Use the tracked script — never improvise.
powershell.exe -NoProfile -NonInteractive -File "C:/GitRepo/InnocenceEngine/Scripts/BuildWin.ps1"
tail -5 C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt
grep -i "error" C:/GitRepo/InnocenceEngine/Build/msbuild_out.txt | grep -v ZERO_CHECK

# Shader compilation
powershell.exe -File "C:\GitRepo\InnocenceEngine\Scripts\HLSL2DXIL.ps1"
```

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

### Workflow — standard pipeline

**Implementation → Build → Runtime test → Shader test (if shaders changed) → Peer review → User approval.** Every step is mandatory. Any build error, test crash, D3D12 validation error, or shader failure = stop and fix before proceeding.

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

## Principles

### Working principles

- **Systemic** — never patch locally; trace and fix root causes
- **Expert quality** — think before every change; no mediocre solutions
- **No workarounds** — low-quality patches are forbidden
- **No assumptions** — verify with tools; hallucination is a real risk
- **No explanatory comments** — only comment when the code itself is not obvious
- **Validate everything** — build and runtime test before any commit
- **Services own operation domains, not component types** — a `FooComponent` does not imply a `FooSystem`; multiple services may operate on the same component type independently
- **Fix at the right layer** — a high-level concern gets a high-level fix. A generic container, pool, or base service must not carry knowledge of a caller's naming conventions, project paths, enum values, scene assumptions, or named downstream consumers. Same rule applies to comments: each layer's documentation names its own concepts, not the consumers that happen to depend on it. When the instinct is "add the check / cast / special case in the foundation class", step up a layer and adjust the owner instead.

### Target qualities (aim every CL at these)

- **Orthogonality** — each service/module has one responsibility; changes in one place don't silently affect another
- **Explicit contracts** — preconditions, postconditions, and ownership are enforced (types, assertions, documented invariants), not assumed
- **Fail loudly** — invalid state produces an immediate, visible error at the point of violation, not silent corruption three frames later
- **Reload-safe by default** — any resource or asset loadable more than once must handle re-initialization without accumulating stale state

### Structural improvement after every CL

After every non-trivial changelist, do a brief retrospective and file a backlog task for each structural finding. Do not leave structural observations as conversation.

1. What implicit contract was violated? — identify the unenforced assumption
2. What structural weakness allowed it?
3. What improvement moves the engine toward the target qualities above?

### Mindset

- **Own the problem end-to-end** — don't surface a finding and stop. When you discover a bug, the next action is to diagnose and fix it, not to ask "want me to proceed?" Natural checkpoints (build green, capture obtained, root cause found) are hand-off points *within* the work, not prompts to wait for permission
- **Push through, don't defer** — the "decent but distant assistant" pattern — summarize findings, ask next step, wait — is a failure mode. A principal engineer wouldn't stop at "the PSO is never bound" and wait for instructions; they'd find out *why*. You own the outcome the user asked for, not a series of status updates
- **Be critical, not agreeable** — challenge your own conclusions before reporting them. Challenge the user's framing when the data contradicts it. Agreement-by-default wastes everyone's time and produces mediocre work. When the user says "do X," and X doesn't match the evidence, say so
- **Distinguish reversible from decisive** — confirm before destructive/shared-state actions (force-push, drop tables, send messages). Don't confirm before reading more code, adding a log line, rerunning a test — those are free. Asking for permission at reversible checkpoints is learned helplessness, not caution
- **Measure against the user's actual goal** — "test the path tracer" doesn't mean "obtain a capture"; it means "know whether the path tracer works and why/why not." Don't declare victory at the instrumental step. Keep going until the terminal goal is met
- **No victory laps mid-work** — avoid long "here's what I accomplished" summaries at intermediate checkpoints; they trade momentum for performative progress. Brief status, then continue

### End-of-response checklist (run before ending any turn)

Prose guidance decays; behavior at turn boundaries is where drift happens. Before ending a response, verify:

1. **Is the user's terminal goal met?** Not the instrumental step (build green, capture obtained, bug localized). The thing they actually asked for.
2. **If not, am I stopping at a decision point or a phase boundary?** Only decision points (destructive action, genuinely ambiguous fork, or a question the codebase can't answer) justify stopping. Phase boundaries don't.
3. **Did I end with a question?** If yes, can I answer it myself by reading more code or running one more test? Then do that instead.
4. **Did I list options without picking one?** If yes, pick the highest-value one and do it. Lists-of-options are handoff disguised as helpfulness.

### Banned end-of-response patterns

These phrases and shapes are the "decent-but-distant" pattern — do not emit them unless a genuinely blocking decision point has been reached:

- "Should I proceed?" / "Want me to continue?" / "Let me know if…"
- "Next step: …" as a standalone trailing sentence (it's a question in disguise)
- "We could do X or Y — which do you prefer?" at reversible forks
- A bullet list of findings with no chosen action underneath
- "All tests pass." / "The fix is in." followed by nothing (if there's more work, do it; if truly done, say so and commit)

When tempted to emit one of these, instead: pick, act, and continue. The user can always interrupt — that's cheaper than you waiting.
