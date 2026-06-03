# InnocenceEngine — Agent Instructions

Project-scoped orchestration for omp. Meta / cross-project rules live in user-scope
(`~/.claude/CLAUDE.md`). This file is omp-native (read at priority 100 because `.omp/` is
non-empty); the Claude-era `CLAUDE.md` files were retired in the omp migration.

## Project framing

Single user (zhangdoa). No team, no other contributors, no CI fleet, no fresh-checkout
onboarding for anyone else. Closure notes / backlog rationales / commit messages: cost is
borne by zhangdoa alone.

## Stages (agents)

| Stage | File | Scope |
|---|---|---|
| `task-mgmt` | `.omp/agents/task-mgmt.md` | Backlog. First agent at session start. `spawns: "*"` (dispatcher). |
| `code-impl` | `.omp/agents/code-impl.md` | C++ / TS source (engine, editor, services, tests). `spawns: ""`. |
| `shader-impl` | `.omp/agents/shader-impl.md` | HLSL (`.hlsl`, `.comp`, `.frag`, `.vert`). `spawns: ""`. |
| `harness-impl` | `.omp/agents/harness-impl.md` | `.omp/`, `AGENTS.md`. `spawns: ""`. |
| `ci-build-impl` | `.omp/agents/ci-build-impl.md` | CMake, `Scripts/`. `spawns: ""`. |

Impl agents carry `spawns: ""` so they cannot dispatch their own peer-review; review is a
fresh dispatch from the main session (see `dispatch-briefs` skill).

## Harness layout

- `.omp/agents/` — stage manifests (above).
- `.omp/skills/<name>/SKILL.md` — project skills (priority 100; win over same-named
  `~/.claude/skills`). Enforcement-supporting content only.
- `.omp/extensions/commit-guard/` — the commit gates (TypeScript omp extension; a `tool_call`
  interceptor). `bun test` under `tests/` must pass for changes.
- `.omp/state/` — point-in-time fact snapshots (direction, remote-sync, engine invariants).
  Read by `task-mgmt` at session start; update in the same CL as a directional change.

## Enforcement (commit-guard extension)

`git commit` run through omp's bash tool is gated. A failing gate returns a block reason;
gates enforce footer/staged-file presence, not truthfulness. Escape hatches preserved:
`[task-stays-open]`, `Review-Skipped:`, `Review-Skipped-Visual:`, `Closure-Reason:`, and the
no-images / no-new-md allowlists.

| Gate | Blocks when |
|---|---|
| data-generated | `Data/Generated/` staged or `.gitignore` mask loosened. |
| no-images | New/modified image staged (allowlist: `Data/Engine/Icons/`, Editor `*-snapshots/`). |
| no-new-md | New `.md` outside `.backlog/tasks/`, `.omp/{agents,skills,commands,state,extensions}/`, `AGENTS`/`README`/`LICENSE`. |
| commit-body-cap | Body > 40 lines (trailers excluded). |
| comment-essay-cap | Staged code adds > 5 contiguous `//` lines, or a comment cites a tracker id / phase / RFC. |
| file-size | Touched source file > 300 lines AND growing (harness dirs `.omp/` exempt). |
| closure-staleness | Commit cites open `TASK-N` with non-docs staged. |
| peer-review / visual-review | Missing `Reviewed-By:`/`Review-Skipped:` (and `Reviewed-Visually:` when body cites `Build/captures/`). |
| test-run / live-engine / serialize-test | Code staged but no qualifying integration / live-engine / serialize run this turn. |
| attribution | Missing `Code-AI-Generated-By:` / `Message-AI-Generated-By:`. |

Not hard-enforced under omp (session-model differences): the always-apply-skill check and the
session-start briefing are convention via agent manifests; the old `agent-dispatch` gate is
now `spawns` config; `no-auto-memory` was dropped (its venue is unused).

## Session start

First action of every new session: invoke the `task-mgmt` agent for briefing. Then, for work
continuing a prior session, read the relevant resume note from the `InnocenceEngine`
basic-memory project.

## Memory (basic-memory MCP)

Cross-session continuity lives in the **`InnocenceEngine`** basic-memory project (run
`list_memory_projects` to resolve its path; address it explicitly with
`project: "InnocenceEngine"` — it is not the default).

- Backlog.md tasks (`.backlog/tasks/`) remain the task tracker + per-task history.
- basic-memory holds the cross-cutting layer — resume points, durable decisions, load-bearing
  invariants, gotchas that outlive a task. Link to the task; don't duplicate full notes.
- Read the resume note at the start of continuing work; write/update on a durable
  decision/invariant or session end — not for ephemeral status. Find notes via the project's
  `search_notes` / `recent_activity`, not hardcoded titles.

## Commit drafts

`Build/commit-message.txt` (gitignored). Commit with `git commit -F Build/commit-message.txt`
using the `write` tool, not a heredoc. See the `commit-message-policy` skill.
