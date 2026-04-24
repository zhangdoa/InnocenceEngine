# Discipline: workspace-hygiene

No scratch files in the repo root or any tracked directory. Transient output (build logs, captures, experiments) goes to `Build/` only — `Build/` is gitignored and exists for exactly this. Scripts belong in `Scripts/` (tracked); never in `Build/`.

Do not touch `Source/External/` (git submodules) or `Source/Engine/ThirdParty/` (vendored) from AI-authored CLs — these are external code and fall outside every agent's ownership.

Do not create new documentation files (`*.md`, `README`, design doc, roadmap, spec, architecture note, etc.) under the repo root or any tracked directory without explicit user request. Inferring usefulness is not authorization — the user must have typed something like "create a doc at …". The only AI-authored `*.md` files checked in are the ones that belong with tracked infrastructure: agent manifests under `.claude/agents/`, disciplines under `.claude/disciplines/`, backlog tasks under `.backlog/tasks/`, ownership / scope declarations in subtree `CLAUDE.md` files, and paper-alignment audits under `.alignments/`.

The backlog is the only AI-authored project-state medium. Cross-session context goes in backlog task `## Implementation Notes`, not a standalone doc.
