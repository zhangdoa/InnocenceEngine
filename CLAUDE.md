# InnocenceEngine — Claude Instructions

Project-scoped orchestration. Meta / cross-project rules in user-scope `CLAUDE.md`.

## Project framing

Single user (zhangdoa). No team, no other contributors, no CI fleet, no fresh-checkout onboarding for anyone else. Closure notes / backlog rationales / commit messages: cost is borne by zhangdoa alone.

## Stages

| Stage | File | Scope |
|---|---|---|
| `task-mgmt` | `.claude/agents/task-mgmt.md` | Backlog. First agent at session start. |
| `code-impl` | `.claude/agents/code-impl.md` | C++ / TS source (engine, editor, services, tests). |
| `shader-impl` | `.claude/agents/shader-impl.md` | HLSL (`.hlsl`, `.comp`, `.frag`, `.vert`). |
| `harness-impl` | `.claude/agents/harness-impl.md` | `.claude/`, `CLAUDE.md`. |
| `ci-build-impl` | `.claude/agents/ci-build-impl.md` | CMake, `Scripts/`. |

## Gates (the load-bearing harness layer)

| Gate | Blocks when |
|---|---|
| `data-generated.js` | Files under `Data/Generated/` are staged or `.gitignore` mask loosened. |
| `no-images.js` | New / modified image files staged (allowlist: `Data/Engine/Icons/`, `Source/Editor-Next/tests/*-snapshots/`). |
| `no-new-md.js` | New `.md` outside `.backlog/tasks/`, `.claude/{agents,skills,commands,state}/`, or CLAUDE/README/LICENSE allowlist. |
| `commit-body-cap.js` | Commit body > 40 lines (trailers excluded). |
| `comment-essay-cap.js` | Staged code adds > 5 contiguous lines of `//` comments, OR a comment carrying a tracker id / design-doc section / phase-bin-primitive ref (WHY-only in comments). |
| `file-size.js` | Touched file > 300 lines AND growing past pre-image size. |
| `closure-staleness.js` | Commit cites `TASK-N` still open AND staged files include non-docs. Bypass: `[task-stays-open]` in subject. |
| `peer-review.js` | Missing `Reviewed-By:` / `Review-Skipped:` footer. |
| `visual-review.js` | Commit body mentions `Build/captures/` but missing `Reviewed-Visually:` / `Review-Skipped-Visual:`. |
| `test-run.js` | Code staged but no qualifying integration test ran in this turn. Closing a task = same requirement. Exemption: `Closure-Reason: <value>`. |
| `live-engine.js` | Editor-Next code staged but no live engine / Playwright run. |
| `serialize-test.js` | Serializer code staged but no serialize-determinism run. |
| `attribution.js` | Missing `Code-AI-Generated-By:` / `Message-AI-Generated-By:` (or Human-Written equivalent). |
| `agent-dispatch.js` (session-gate) | `Agent` call without `run_in_background: true` and without `[foreground-required]` in prompt. |
| `skill-evidence.js` (session-gate) | Sub-agent side-effecting tool call before its transcript shows `Skill` invocations for every name on its manifest's always-apply line. |
| `no-auto-memory.js` (session-gate) | Writes to `~/.claude/projects/<slug>/memory/`. |
| `task-mgmt-brief.js` (session-gate) | Substantive tool used before the `task-mgmt` subagent has run this session. Escape: `CLAUDE_SKIP_PRODUCER=1`. |
| `session-start-brief.js` (SessionStart) | Doesn't block — injects the "dispatch task-mgmt first" directive at session start; paired with `task-mgmt-brief`. |

## Session start

First action of every new session: invoke `task-mgmt` agent for briefing. Then, for any work continuing prior sessions, read the relevant resume note from the `InnocenceEngine` basic-memory project (see Memory).

## Memory (basic-memory MCP)

Cross-session continuity for this project lives in the **`InnocenceEngine`** basic-memory project (created via the basic-memory MCP; run `list_memory_projects` to resolve its local path). Address it explicitly in MCP calls (`project: "InnocenceEngine"`) — it is **not** the default project.

- This is the sanctioned memory venue. It is distinct from the disabled Claude auto-memory path (`~/.claude/projects/<slug>/memory/`, blocked by `no-auto-memory`) — basic-memory is not blocked.
- **Division of labour:** Backlog.md tasks remain the task tracker + per-task history. basic-memory holds the cross-cutting / continuity layer — resume points, durable decisions, load-bearing invariants, and gotchas that outlive a single task. Don't duplicate full backlog notes into it; link to the task.
- **Read** the relevant resume note at the start of continuing work. **Write/update** it when a session ends or lands a durable decision/invariant — not for ephemeral status.
- **Find** the resume note for the work in hand via the project's `search_notes` / `recent_activity` (by task id or topic) — don't hardcode note titles here; they get reorganized.

## Harness wiring

- `.claude/settings.json` registers three dispatchers: `session-gate.js` + `commit-gate.js` as PreToolUse hooks, and `session-start.js` on the SessionStart event.
- Per-gate logic: `.claude/hooks/gates/<name>.js`. Shared helpers: `.claude/hooks/lib/common.js`.
- All three dispatchers fail open on internal errors; a single gate that throws is skipped without disabling the rest of its tier.
- Commit-message drafts: `Build/commit-message.txt` (gitignored).
