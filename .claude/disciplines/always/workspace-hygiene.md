# Discipline: workspace-hygiene

## Rules

- No scratch files in the repo root or any tracked directory. Transient output (build logs, captures, experiments) → `Build/` only (gitignored).
- Scripts → `Scripts/` (tracked). Never in `Build/`.
- `Data/Generated/` is derived runtime output. Do not track files there. Do not modify `.gitignore` to permit it. Enforced by `.claude/hooks/gates/data-generated.js`.
- If a fresh clone needs an asset → source it from a tracked location (`Data/Engine/`, `Source/External/`) or a setup-script download. Never by un-ignoring derived output.
- Do not touch `Source/External/` (git submodules) or `Source/Engine/ThirdParty/` (vendored) from AI-authored CLs.

## New documentation files

Do not create new `*.md`, `README`, design doc, roadmap, spec, or architecture note under any tracked directory without explicit user request. The only AI-authored `*.md` files allowed:

- `.claude/agents/`, `.claude/disciplines/` — agent manifests and disciplines.
- `.backlog/tasks/` — backlog tasks.
- Subtree `CLAUDE.md` — ownership / scope declarations.
- `.alignments/` — paper-alignment audits.

Cross-session context goes in backlog task `## Implementation Notes`, not a standalone doc.

## Cross-references

- `always/persistence-venue.md` — venue routing table.
- `always/backlog-workflow.md` — backlog is the single AI-authored project-state medium.
