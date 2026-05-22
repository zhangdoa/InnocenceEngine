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
| `comment-essay-cap.js` | Staged code adds > 5 contiguous lines of `//` comments. |
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

## Session start

First action of every new session: invoke `task-mgmt` agent for briefing.

## Harness wiring

- `.claude/settings.json` registers `session-gate.js` (session-level) + `commit-gate.js` (commit-level) as PreToolUse hooks.
- Per-gate logic: `.claude/hooks/gates/<name>.js`. Shared helpers: `.claude/hooks/lib/common.js`.
- Both dispatchers fail open on internal errors.
- Commit-message drafts: `Build/commit-message.txt` (gitignored).
