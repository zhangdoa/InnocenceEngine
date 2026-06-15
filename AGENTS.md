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
- `.omp/rules/` — standalone rules. Each file has a YAML frontmatter (`name`,
  `description`, `condition`, `scope`) consumed by the harness's rule matcher.
  Read by the model on demand; not hard-gated. Examples:
  `no-speculative-debug-loop.md`, `no-revert-on-engine-gap.md`,
  `pre-commit-tracker-sync.md`, `verify-capture-path-not-stale.md`,
  `prefer-ast-rewrites.md`. New rules belong here when an observed failure
  pattern is worth encoding for future sessions; see the
  `harness-changes-are-empirical` line in the global user-scope CLAUDE.md.
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

## Logging discipline

Logs are diagnostics, not narration. Two failure modes to avoid:

- **Main-loop spam** — `Log()` in per-frame paths (render-graph `RecordPass`,
  per-frame update hooks, hot services) floods the log, hides real signal, and
  tanks log-search/grep performance. Logs at `Success`/`Info` level inside
  per-frame loops are forbidden; they belong at `Verbose` (off by default).
- **Silent one-shot failures** — init / setup / parse paths must surface
  *every* failure at `Error` or `Warning`. If an init step can fail, log
  the failure with the entity name. Defaulting to "return false" on the
  happy path with no message is acceptable; on a failure path it's a bug.

### Rules

- `Log(Error|Warning)` for any failure path in setup / init / parse / one-shot
  hooks — no silent `return false;` on a failure. Caller needs the entity
  name + reason to diagnose.
- `Log(Success|Info)` is reserved for non-repeating startup milestones
  (e.g. "DX12 device created", "graph loaded with N passes"). Do not
  log a success message every time a routine completes — that's
  per-frame in disguise.
- `Log(Verbose)` for per-pass or per-frame signals that are useful when
  actively debugging but not for normal operation. Default off.
- `Log(Success|Info)` inside a per-frame path (rendering, update,
  draw, simulation tick) is a harness smell. If you need to confirm
  flow during a debug session, gate it on a `-verbose` cmdline flag
  or a hot-reloadable config, not a hardcoded `Log(Success, …)`.
- **Diagnostic logs are not for committing.** A `Log(Success, "RecordPass start: …")`
  you added to track a crash gets removed before the commit. The commit
  message records the finding, the code carries the rule, the log stays
  clean. Exception: a log that catches a real defect class with no
  production-side cost (rare; document in the body).

### Pre-commit check

Before staging, grep your own diff for `Log(Success|Info)` and `Log(Verbose)` and
ask: is this in a per-frame path? Is it firing once or N times? If N, demote
to `Verbose` or delete. The harness will not block on log spam today, but a
future gate will.


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
