# Discipline: workspace-hygiene

No scratch files in the repo root or any tracked directory. Transient output (build logs, captures, experiments) goes to `Build/` only — `Build/` is gitignored and exists for exactly this. Scripts belong in `Scripts/` (tracked); never in `Build/`.

## How

### Generated data

`Data/Generated/` is derived runtime output. Do not track files there. Do not modify `.gitignore` to permit it. If a fresh clone needs an asset, source it from a tracked location (`Data/Engine/`, `Source/External/`) or a setup-script download — never by un-ignoring derived output. Enforced by `.claude/hooks/gates/data-generated.js`.

### External code

Do not touch `Source/External/` (git submodules) or `Source/Engine/ThirdParty/` (vendored) from AI-authored CLs — these are external code and fall outside every agent's ownership.

### Documentation files

Do not create new documentation files (`*.md`, `README`, design doc, roadmap, spec, architecture note, etc.) under the repo root or any tracked directory without explicit user request. Inferring usefulness is not authorization — the user must have typed something like "create a doc at …". The only AI-authored `*.md` files checked in are the ones that belong with tracked infrastructure: agent manifests under `.claude/agents/`, disciplines under `.claude/disciplines/`, backlog tasks under `.backlog/tasks/`, ownership / scope declarations in subtree `CLAUDE.md` files, and paper-alignment audits under `.alignments/`.

The backlog is the only AI-authored project-state medium. Cross-session context goes in backlog task `## Implementation Notes`, not a standalone doc.

## Recorded incident

TASK-133 (commit `652c7a29`, 2026-04-25) — a fresh clone could not run because `Bin/Config/`, `Bin/Shaders/`, and `Bin/Data/` were missing; an earlier CL had un-ignored derived output to "fix" the gap rather than adding a setup-script download path. The data-generated gate now blocks the un-ignore pattern at commit time.

## Cross-references

- `persistence-venue.md` — explains why backlog Implementation Notes are the right venue for cross-session context (the only one that reaches subagents).
- `backlog-workflow.md` — the no-rogue-implementations rule and the closure-staleness gate are the operational form of "backlog is the single AI-authored medium."
